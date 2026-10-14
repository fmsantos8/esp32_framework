# Contratos e integração do porte ESP32

Esta migração usa os contratos públicos definidos no projeto TCC, com a revisão de timeout aprovada durante a integração. O escopo é HAL, memória, porte ESP32, BSP e adaptação do teste existente de `main.c`.

| Contrato | Versão |
| --- | --- |
| Ciclo de vida e timeout comum | 2 |
| GPIO | 1 |
| I2C | 2 |
| UART | 2 |
| Timer | 1 |
| Informações do porte | 1 |
| Memória | 1 |

`hal_port_get_info()` fornece versões e capacidades antes de qualquer inicialização. A integração compara esses dados com os cabeçalhos antes de construir recursos. Novas versões incompatíveis exigem uma revisão explícita do porte.

## Inclusão dos contratos

Inclua explicitamente os cabeçalhos utilizados, por exemplo `gpio/hal_gpio.h`, `i2c/hal_i2c.h` e `common/hal_port.h`. Configure `hal/interface` como raiz de inclusão e `memory` ao usar `framework_memory.h`. Os componentes ESP-IDF já exportam esses diretórios na configuração de compilação.

## Construção e memória

A ordem de inicialização é: consultar o porte; chamar `framework_memory_init()`; construir os recursos no BSP; chamar `framework_memory_seal()`; executar o teste. Falhas interrompem a inicialização e são devolvidas à integração, sem `assert` com efeitos colaterais e sem repetição automática.

Os construtores recebem uma configuração completa e devolvem um handle somente em caso de sucesso. Um destino válido recebe `NULL` em falhas. O BSP não inclui representações privadas nem declara objetos concretos. GPIO, I2C e UART compartilham o controle de propriedade dos pinos: um GPIO não pode ocupar um pino já atribuído a um barramento. Controladores e a vinculação do timer também não podem ser construídos duas vezes.

A arena é um array estático de bytes com alinhamento de `max_align_t`. O porte copia estruturas locais tipadas para esse armazenamento e recupera seus dados por `memcpy`, sem desreferenciar os handles como estruturas dentro do array. Não exige `-fno-strict-aliasing`. A implementação usa C11; os cabeçalhos públicos continuam utilizáveis em C99, C11 e C++11.

`CONFIG_FRAMEWORK_MEMORY_CAPACITY`, em `menuconfig → TCC framework`, define a capacidade: o padrão é **1024 bytes**. O limite inclui os objetos e o ajuste de alinhamento. `framework_memory_get_usage()` informa capacidade, consumo, espaço restante e encerramento das alocações. Não há desalocação nem reset público; inicializar novamente não apaga os objetos. Uma reserva recusada não avança o cursor; uma construção que falhe depois de reservar memória pode consumir espaço.

Os drivers do ESP-IDF mantêm suas próprias alocações internas, fora da arena. Por isso, o consumo da arena não representa toda a RAM usada pelo firmware. Tanto falta de espaço na arena quanto falta de memória informada pelo SDK resultam em `HAL_STATUS_NO_MEMORY`.

## Timeout definido na configuração

I2C e UART recebem `timeout` no construtor, em milissegundos, e mantêm esse valor durante a vida do objeto. As transferências não recebem esse argumento. Zero e valores acima de `HAL_TIMEOUT_MAXIMUM` são inválidos.

O contrato comum não garante um prazo rígido para toda a chamada. Ele exige um mecanismo de espera finito, documentação das restrições e liberação do uso dos buffers antes do retorno. Portes com timeout fixo em compilação devem conferir a correspondência na construção e retornar `HAL_STATUS_NOT_SUPPORTED` para valores incompatíveis. As capacidades `HAL_I2C_CAPABILITY_CONFIGURABLE_TIMEOUT` e `HAL_UART_CAPABILITY_CONFIGURABLE_TIMEOUT` indicam suporte a valores ajustáveis pelo construtor; não prometem qualquer valor em qualquer recurso.

No ESP32, ambos são ajustáveis e o BSP usa **1000 ms**:

- **I2C:** o valor armazenado é passado a uma única chamada síncrona de `i2c_master_execute_defined_operations()`, inclusive na transação combinada. O driver pode aplicar esse valor em várias esperas internas; portanto, o tempo total pode ultrapassá-lo. A conversão é em ticks do FreeRTOS. O porte recusa valores menores que um tick ou maiores que `UINT32_MAX / configTICK_RATE_HZ`, evitando espera nula e overflow em versões do SDK que multiplicam em 32 bits. Com a configuração atual de 100 Hz, a faixa aceita é de 10 a 42 949 672 ms, sujeita à resolução dos ticks.
- **UART:** o porte usa um único prazo monotônico calculado a partir do timeout armazenado, envia os dados por FIFO e espera a conclusão usando apenas o tempo restante. A conversão para ticks arredonda para cima; escalonamento e o trabalho do SDK podem causar excedentes. Os bytes aceitos são informados mesmo quando a operação termina com erro ou timeout; bytes já copiados ao FIFO podem terminar de sair pelo fio após o retorno.

`ESP_ERR_TIMEOUT` é traduzido para `HAL_STATUS_TIMEOUT`. NACK e `ESP_ERR_INVALID_RESPONSE` resultam em `HAL_STATUS_ERROR`. O código do ESP-IDF 6.1 instalado também pode reportar determinados timeouts internos como `ESP_ERR_INVALID_RESPONSE`; nesses casos a HAL preserva o erro geral informado pelo SDK, sem inventar um diagnóstico. A distinção completa dessas falhas permanece uma limitação a verificar em hardware e na versão do SDK usada.

## Recursos e capacidades

O mapeamento físico está em [pinout.md](pinout.md). O BSP preserva I2C0 em GPIO21/GPIO22 a 100 kHz, UART0 em GPIO1/GPIO3 a 115200 baud 8N1, botão em GPIO18 e LED em GPIO19. O LED é construído primeiro, inicialmente apagado. As polaridades e os endereços 0x44/0x3C são fornecidos pelo BSP.

| Interface | Suporte e restrições deste porte |
| --- | --- |
| GPIO | Banco 0; entrada ou saída push-pull; níveis elétricos `bool`. GPIO6–GPIO11 são reservados para a flash do WROOM-32. GPIO34–GPIO39 não aceitam saída nem resistores internos. A operação incompatível com a direção retorna `HAL_STATUS_NOT_SUPPORTED`. |
| I2C | Instâncias 0 e 1; pinos distintos com capacidade de saída; 10–400 kHz; endereços de 7 bits entre 0x08 e 0x77; 1–65535 bytes por fase. Há um único dispositivo SDK sem endereço fixo, criado na inicialização. As transações enviam explicitamente o endereço; a combinada usa START repetido e um único STOP final. A leitura termina com NACK no último byte. Não há criação de dispositivos pelo porte durante cada transferência. |
| UART | Instâncias 0, 1 e 2; 7 ou 8 bits de dados; paridade nenhuma/par/ímpar; 1 ou 2 bits de parada; sem controle de fluxo. Baudrate limitado a `SOC_UART_BITRATE_MAX` e à possibilidade de configuração pelo SDK. Em 7 bits, todo o buffer é validado antes do envio. Tamanho zero é uma operação vazia válida; o contador de progresso é obrigatório. |
| Timer | Vinculação 0 ao relógio monotônico `esp_timer_get_time()`. Captura uma origem sem reiniciar o relógio compartilhado e retorna milissegundos inteiros módulo 2³². Não reserva um GPTimer físico. |

A arena e a propriedade dos recursos pressupõem inicialização sequencial. A integração serializa o acesso aos handles; não há garantia comum para concorrência ou interrupções. O console do SDK e o teste usam UART0, como na montagem original; isso não estabelece exclusividade do fio serial contra logs da plataforma.

## Verificação e limites do escopo

Execute os testes de contrato no computador com:

```sh
python3 tests/run_host_tests.py
```

Eles compilam todos os cabeçalhos em C99/C11/C++11, verificam ligação C/C++, e exercitam arena, esgotamento, falhas de construção, conflitos entre periféricos, direções GPIO, wrap do timer, endereçamento I2C, START repetido, contagem parcial UART e o BSP com um SDK simulado. O porte é compilado com otimização e análise de aliasing habilitada. Esses testes não substituem a validação elétrica nem demonstram os tempos do driver real.

Com o ambiente ESP-IDF 6.1 ativado, compile o firmware com `idf.py build`. O binário também foi gerado durante a migração. Não foi gravado na placa e não houve ensaio físico nesta sessão.

O teste de `main.c` mantém a aquisição bloqueante do SHT31-D, uma tela de temperatura/umidade e o LED seguindo o botão. Ainda não implementa a aplicação modular completa, as duas telas com debounce, a preservação da última amostra, a indicação de falha e a telemetria comum previstas para a prova de conceito. STM32F4 e nRF52840 também não foram implementados nem validados nesta migração.

`hal/interface` permanece um submódulo Git com alterações locais. Para revisar os contratos, use `git -C hal/interface diff` e `git -C hal/interface status --short`; a revisão no repositório principal deve ser acompanhada pela revisão interna do submódulo. Não houve commit nem push.
