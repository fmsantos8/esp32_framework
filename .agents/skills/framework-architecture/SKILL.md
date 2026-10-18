---
name: framework-architecture
description: Explicar e aplicar as responsabilidades dos módulos do framework embarcado deste repositório ao projetar, implementar ou revisar app, componentes, HAL, arena de memória, portes, BSP e integração. Não usar para tarefas alheias à arquitetura do firmware.
---

# Arquitetura do framework

Use este contexto ao decidir **onde colocar código**, quais dependências um módulo pode ter e se uma mudança é portável. A fonte da arquitetura pretendida é `68d497929590b37ccdc69df0/2-textuais/3-proposta.tex`, principalmente “Arquitetura geral e regra de dependências”, “Interfaces e contratos portáveis”, “Organização dos artefatos” e “Prova de conceito proposta”. Consulte os arquivos reais e a documentação Doxygen dos cabeçalhos para a API vigente. Se a proposta e o código divergirem, explicite a diferença; não apresente uma intenção do TCC como funcionalidade já implementada.

O documento do TCC pode estar ausente neste checkout. Nesse caso, registre a limitação e use os contratos e artefatos disponíveis para descrever o estado implementado.

## Partes e responsabilidades

| Parte | Responsabilidade | Dependências permitidas |
| --- | --- | --- |
| `app/` (planejado) | Coordenar o comportamento da prova de conceito: aquisição periódica, estado da última amostra, seleção das duas telas, política de repetição após falhas e estados de indicação. As “tarefas” são módulos de contexto e processamento, sem pressupor um RTOS. | Interfaces públicas da aplicação e dos componentes. |
| `components/` (planejado) | Encapsular mecanismos reutilizáveis dos dispositivos e serviços; cada componente tem interface pública, contexto, inicialização e dependências explícitas. Recebe recursos válidos da integração, sem escolher a instância física. | Interfaces de outros componentes e contratos públicos da HAL. |
| `hal/interface/` | Definir em C os contratos portáveis de GPIO, I2C, UART, temporizador e códigos de estado. Expõe tipos de recurso opacos e semântica observável; não conhece estruturas do SDK. | Tipos C e outros contratos públicos da HAL. |
| `hal/memory/` | Fornecer uma arena linear C11 compartilhada pelos portes para armazenar objetos persistentes da HAL. Administra alinhamento, capacidade, consumo e selagem sobre um buffer externo. | Biblioteca padrão C11; não depende de `hal/interface`, SDK, BSP ou RTOS. |
| `hal/ports/<plataforma>/` | Implementar as funções públicas da HAL usando o SDK selecionado; manter tipos concretos, estado do driver e tradução de erros. O mesmo porte STM32F4 deve atender às duas placas STM32 previstas. | HAL pública, `hal/memory` e SDK da plataforma; não incorpora pinos nem política de produto próprios de uma placa. |
| `bsp/` ou `boards/` | Descrever uma placa concreta: pinos, controladores, barramentos, polaridades, endereços e demais recursos físicos; realizar ou apoiar a inicialização de baixo nível. | HAL e SDK quando necessários; não conhece módulos de `app/` ou `components/`. |
| `main/` e arquivos de integração/compilação | Servir de raiz de composição: escolher porte e BSP, fornecer e dimensionar o buffer da arena, criar contextos, injetar dependências, inicializar os recursos e selar a arena antes do processamento da aplicação. Define como executar os módulos com o ambiente da plataforma. | Pode conhecer interfaces públicas, memória, porte, BSP e SDK; não deve concentrar a política de produto da aplicação final. |
| SDK e hardware | Fornecer drivers, serviços de plataforma e dispositivos físicos. | Acesso concentrado no porte, no BSP e na integração. |
| `docs/` e testes | Documentar contratos, capacidades e versões; verificar comportamento equivalente dos portes e o isolamento das dependências. | Consultar os contratos e artefatos correspondentes. |

Os projetos convencionais de comparação, sem framework, ficam fora desta arquitetura; neles o uso direto do SDK é intencional.

## Componentes reutilizáveis previstos

- **Botão:** interpreta o GPIO de entrada e a polaridade fornecida pela placa, oferecendo o estado de acionamento sem expor o pino. Na proposta atual, a aplicação usa a contagem de tempo para fazer o debounce por software de aproximadamente 30 ms.
- **LED:** traduz comandos lógicos de indicação para o GPIO de saída, respeitando a polaridade configurada pela placa. A aplicação decide quando manter aceso, piscar após falha ou apagar.
- **SHT31-D:** emite comandos I2C, obtém temperatura e umidade, valida a resposta e devolve dados ou erro. Recebe o barramento I2C e o endereço; não decide repetição de leitura nem preservação da última amostra.
- **SSD1306:** encapsula a inicialização e os comandos/dados do controlador OLED via I2C. Recebe o mesmo barramento do sensor e seu próprio endereço; a aplicação decide qual tela e conteúdo apresentar.
- **Telemetria:** produz o formato comum das leituras e dos erros e os envia pela UART recebida. A aplicação decide quais eventos comunicar. A configuração é estabelecida na construção do recurso; o contrato atual não oferece reconfiguração da UART em execução.
- **Temporização:** apoia prazos e intervalos sem bloqueio; a política de quando medir, atualizar tela ou piscar pertence à aplicação. O contrato atual fornece notificações periódicas de um temporizador físico, por callback em interrupção, e não uma função de consulta de milissegundos. Uma contagem mantida a partir dessas notificações exige sincronização e está sujeita às condições de latência e aglutinação documentadas pelo porte.

O TCC cita os componentes como artefatos previstos. Os detalhes de suas APIs ainda precisam ser definidos a partir dos contratos atuais e dos requisitos da prova de conceito; não inferir nomes de funções a partir desta lista.

## Regras de fronteira

1. `app/` e `components/` não incluem headers do BSP, dos portes ou do SDK. Seus dados de placa chegam por parâmetros e handles; recursos compartilhados, como o barramento I2C do SHT31-D e do SSD1306, são injetados pela integração.
2. Cabeçalhos de `hal/interface/` não expõem tipos do fabricante. O porte implementa os símbolos `hal_*`, mantém as estruturas concretas e aloca os objetos nos construtores. BSP/integração fornecem configurações e recebem ponteiros opacos; não devem precisar declarar objetos concretos da HAL por valor.
3. O BSP escolhe **qual** recurso físico representa uma função da placa; o porte define **como** executar uma operação comum nesse recurso. Decisões sobre clocks, pinos e compilação permanecem na região específica da placa/plataforma.
4. A composição pode conhecer todas as partes para conectá-las, mas as decisões de produto ficam na aplicação. A execução das tarefas pode ser um laço principal ou um mecanismo da plataforma; não há abstração comum de RTOS.
5. As funções `hal_gpio_config`, `hal_i2c_config`, `hal_uart_config` e `hal_timer_config` são construtores de inicialização: recebem a configuração e um ponteiro de saída para o handle, exigindo arena inicializada e não selada. Não presumir reconfiguração em execução, destruição ou liberação dos objetos. O porte valida os campos e capacidades antes de alocar, e entrega um handle válido após o sucesso da construção.
6. Use o Doxygen do cabeçalho público para assinaturas e semântica vigentes. I2C e UART recebem um timeout positivo em milissegundos na configuração, mantido durante a vida do objeto; as transferências não recebem timeout por chamada. Esse limite controla a espera do driver e não garante um prazo estrito para a chamada inteira. O TCC previa consulta de tempo transcorrido, mas a API atual do temporizador oferece `hal_timer_config` e `hal_timer_start`; não inferir `hal_timer_get_millis` a partir do teste antigo.
7. Não suponha concorrência segura ou chamadas de HAL em ISR sem autorização explícita do contrato. O callback do temporizador ocorre em interrupção e deve ser curto e não bloqueante. A integração coordena o acesso concorrente; a proposta atribui à aplicação a serialização do I2C compartilhado. A construção dos objetos e o acesso à arena são sequenciais em contexto normal de execução.

## Arena compartilhada pelos portes

`hal/memory/` é um submódulo Git reutilizável entre ESP32, nRF52840 e o porte STM32F4 comum a STM32F411/STM32F407. O nome vigente continua sendo `memory`, com prefixo `hal_memory_*`; os arquivos `hal_memory.c` e `hal_memory.h` ficam diretamente na raiz desse diretório. Consulte o cabeçalho para os contratos completos.

- **Padrão e representação:** foi adotado C11 ou posterior para usar `max_align_t`, `_Alignas` e `_Alignof`. Capacidades, tamanhos, consumo e cálculos internos usam `uint32_t`, por decisão do usuário, em vez de `size_t`. A seleção do padrão nos diferentes sistemas de compilação pertence à integração.
- **Instâncias e armazenamento:** a integração fornece um `hal_memory_t` e um buffer externo alinhado para `max_align_t`, com duração suficiente para todos os objetos em uso. O módulo não define buffer global nem escolhe sua capacidade. Os tipos concretos dos periféricos ficam no porte; aplicação e componentes recebem somente handles.
- **API pública:** `hal_memory_init`, `hal_memory_alloc`, `hal_memory_seal` e `hal_memory_get_info`. Os retornos usam `hal_memory_status_t`, independente de `hal_status_t`; a tradução para o resultado da HAL cabe ao porte.
- **Inicialização única:** `hal_memory_init` preenche todos os campos sem ler o conteúdo anterior do contexto. Não exigir `{0}` antes dessa chamada e não restaurar `HAL_MEMORY_INITIALIZER`, removido por solicitação do usuário. A integração garante que cada contexto seja inicializado com sucesso somente uma vez. Uma segunda chamada não é detectada: reinicia o consumo e remove a selagem, podendo sobrescrever objetos vivos. Depois de uma falha inicial, obter sucesso em `init` antes de consultar ou operar um contexto de conteúdo indeterminado.
- **Reserva:** `hal_memory_alloc` insere preenchimento antes dos blocos e preserva o alinhamento para `max_align_t`; não atende tipos com alinhamento estendido. Não zera o conteúdo reservado. Tamanho zero é inválido, falhas deixam a saída em `NULL` quando válida e não consomem espaço. As verificações de capacidade devem continuar protegidas contra overflow. Receber a saída em uma variável `void *`, atribuindo-a depois ao ponteiro tipado; não converter `hal_gpio_t **` em `void **`.
- **Vida útil:** não há `free`, reset ou reabertura no contrato. A selagem encerra as reservas, é idempotente e preserva os objetos existentes; reinicializar depois disso viola o contrato. Não adicionar mutexes ou dependência de RTOS para o cenário atual de construção sequencial.
- **Consumo e falhas:** `hal_memory_info_t.used` inclui preenchimentos consumidos; `remaining` é `capacity - used`, e a próxima reserva ainda pode exigir alinhamento. Falha do construtor depois de uma reserva bem-sucedida pode consumir espaço, conforme a HAL, mesmo que o SDK falhe. Isso difere de uma reserva que falha, a qual preserva o consumo. O armazenamento interno dos drivers do SDK permanece separado dessa arena.
- **Organização e documentação:** o módulo atual contém somente os arquivos `.c` e `.h`; não pressupor CMake, README ou suíte de testes presentes. A pasta `tests/` foi removida por solicitação do usuário. Nos enums e campos das estruturas deste módulo, usar comentários na própria linha começando com `/**`, sem `<`, como em `hal_memory_info_t`.

## Estado desta árvore

O projeto atual contém os submódulos `hal/interface/`, `hal/memory/` e `hal/ports/esp32-wroom-32/`, além de `bsp/bsp_esp32.c` e `main/main.c`. A arena já está implementada, mas o porte ESP32 contém apenas um README: sua integração com os construtores ainda precisa ser feita. O BSP e `main` usam formas de uma API anterior, incluindo objetos por valor e consulta de milissegundos, e precisam ser adaptados aos contratos atuais. `main/main.c` concentra a prova inicial com SHT31-D/SSD1306 e chamadas diretas do FreeRTOS. `app/` e `components/` ainda não existem nesta árvore. Redistribuir essas responsabilidades ao evoluir para a arquitetura proposta, sem tratar o teste atual como desenho final.
