---
name: framework-architecture
description: Aplicar a arquitetura e os padrões de código e documentação deste framework embarcado ao projetar, implementar ou revisar aplicações, componentes, HAL, memória, portes, BSP e integração. Não usar para tarefas alheias ao firmware deste projeto.
---

# Arquitetura e padrões do framework

Use estas decisões ao desenvolver o firmware. Confira os arquivos reais e o Doxygen dos cabeçalhos antes de alterar contratos: o retrato da implementação abaixo pode evoluir. Quando disponíveis, consulte `TCC.pdf` ou as fontes da proposta para a arquitetura pretendida; diferencie intenção do TCC de funcionalidade implementada. Se o documento estiver ausente, baseie o trabalho na árvore e explicite a limitação quando ela afetar a análise.

## Nomes, tipos e organização

- Use `cmp_<componente>_*` nas funções dos componentes, inclusive auxiliares privadas, e `cmp_<componente>` nos nomes dos arquivos. Exemplos: `cmp_button_read`, `cmp_ssd1306_init`, `cmp_sht31d_start_measurement`. Preserve a grafia `sht31d` nesse componente; revise símbolos, includes e documentação ao renomear.
- Parâmetros de saída não recebem o prefixo `out_`: use `pressed`, `sample`, `resources`, `ticks`, etc. Atualize declaração, definição, documentação e chamadas em conjunto.
- Separe tipos públicos em `*_types.h`, operações em `*.h` e configuração/armazenamento de composição em `*_config.h`, conforme o módulo. Preserve os nomes de tipos já adotados, como `button_t` e `ssd1306_status_t`; o prefixo de função `cmp_` não exige renomeá-los.
- Em `hal/memory`, use `uint32_t` para capacidades, tamanhos, consumo e cálculos internos, em vez de `size_t`. Preserve esse padrão nas APIs do framework que já o adotam e verifique conversões/overflow ao interagir com bibliotecas/SDKs.
- Use `#define` para constantes como dimensões e tamanho do framebuffer do SSD1306. Isso não elimina enums de estados, fontes ou resultados.
- O nome adotado é `hal_esp32_convert_timeout_to_ticks(uint32_t timeout, TickType_t *ticks)`. Preserve `timeout`, sem `_ms`, nessa função; documente unidade, arredondamento e limites reais da conversão.
- Respeite o formatador do projeto. Para sequências de bytes do SSD1306, use matrizes de bytes com linhas curtas e coerentes; não recorra a strings `"\x.."`, linhas extensas ou `clang-format off/on` para contornar a formatação.
- Não crie nem escreva READMEs sem pedido explícito. Não recrie `hal/memory/tests/`, removido pelo usuário; essa decisão não é uma proibição geral de validar mudanças em outros módulos.

## Comentários e Doxygen

- Dentro de funções em arquivos `.c`, use sempre `//` para comentários de implementação. A regra não substitui Doxygen de declarações de funções, tipos, enums ou variáveis, inclusive em `.c`.
- Documente campos e valores de enums no padrão `campo; /** Descrição. */`, começando com `/**`, sem `/**<`, como em `hal_memory_info_t`.
- Mantenha Doxygen conciso. Em funções públicas, documente todos os parâmetros com a direção adequada e os retornos possíveis com `@retval`; use `@return` quando apropriado ao tipo de retorno. Registre unidades, efeitos e condições relevantes sem repetir a implementação. Use `@details` somente quando necessário.
- Separe mudanças de grupo de tags por uma linha vazia Doxygen (` *`): `@brief` → `@details`, `@brief`/`@details` → `@param`, `@param` → `@retval`/`@return`. Parâmetros consecutivos e retornos consecutivos ficam no mesmo grupo.
- **Única exceção de espaçamento: o bloco de topo que documenta o arquivo.** Nele, mantenha `@file`, `@author` e `@brief` consecutivos e não use `@details`. Complete blocos `@file` sem autor com `Fabricio Santos (fmsantos8@ucs.br)`.

Exemplo de cabeçalho de arquivo:

```c
/**
 * @file cmp_button.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Logical button input.
 */
```

Exemplo de agrupamento nas funções:

```c
/**
 * @brief Read the debounced button state.
 *
 * @details The output changes only on success.
 *
 * @param[in,out] button Initialized context.
 * @param[out] pressed Required logical-state output.
 *
 * @retval BUTTON_STATUS_OK State written.
 * @retval BUTTON_STATUS_INVALID_ARGUMENT A required pointer is NULL.
 * @retval BUTTON_STATUS_NOT_INITIALIZED Context is unavailable.
 * @retval BUTTON_STATUS_NOT_SUPPORTED Input operation is unavailable.
 * @retval BUTTON_STATUS_GPIO_ERROR Input read failed.
 */
```

## Responsabilidades e dependências

| Parte | Responsabilidade | Dependências permitidas |
| --- | --- | --- |
| `apps/` | Política de aquisição, telas, seleção por botão, última amostra válida, repetição após falhas, indicação e eventos de telemetria. | Interfaces públicas das aplicações e dos componentes; sem HAL, BSP, porte ou SDK. |
| `components/` | Mecanismos reutilizáveis de dispositivos/serviços, com contexto, configuração e dependências explícitas. | Interfaces de componentes e HAL pública; sem BSP, porte ou SDK. |
| `hal/interface/` | Contratos portáveis e handles opacos de GPIO, I2C, UART, timer e sistema. | Tipos C e contratos públicos da HAL; sem tipos do fabricante. |
| `hal/memory/` | Arena linear C11 compartilhada pelos portes, independente da HAL. | Biblioteca padrão C11; sem SDK, RTOS ou `hal/interface`. |
| `hal/ports/<plataforma>/` | Implementação dos contratos, estruturas concretas, integração com drivers e tradução de erros. | HAL pública, `hal/memory` e SDK; sem escolhas de placa ou política de aplicação. |
| `bsp/` | Pinos, instâncias, polaridades, endereços e construção dos recursos de uma placa concreta. | HAL e SDK quando necessário; sem componentes ou aplicações. |
| `main/` e integração | Escolher BSP/porte, criar armazenamento e contextos, conectar dependências e definir a execução na plataforma. | Pode conhecer todas as camadas; a política do produto permanece em `apps/`. |

- O BSP escolhe **qual recurso físico** atende cada função; o porte implementa **como operá-lo**. A composição converte `bsp_resources_t` nas configurações dos componentes e injeta seus handles nas aplicações.
- A aplicação não deve incluir BSP ou headers de configuração de componentes que exponham HAL. Configurações necessárias à recuperação podem chegar como ponteiros opacos, construídos e mantidos válidos pela composição.
- Retornos de componentes são próprios do componente, como `button_status_t` e `ssd1306_status_t`; não exponha `hal_status_t` ou códigos do SDK à aplicação. Traduza erros na fronteira correspondente.
- Contextos de componentes e aplicações pertencem à composição e têm vida útil explícita; não use a arena da HAL para eles. Funções de inicialização devem preencher o próprio contexto sem exigir que o chamador o zere antes; respeite os contratos de falha e reinicialização de cada módulo.
- As tarefas portáveis são módulos de contexto e processamento cooperativo, sem pressupor RTOS. Não espalhe chamadas FreeRTOS em `apps/` ou `components/`.
- Serialize o acesso ao I2C compartilhado entre SHT31-D e SSD1306 em contexto normal. Não suponha segurança concorrente nem uso de HAL em ISR sem autorização do contrato. Projetos convencionais de comparação, sem framework, podem usar o SDK diretamente.

## Inicialização da HAL e arena persistente

Ordem de composição: `hal_system_init()` → construção dos recursos pelo BSP → `hal_system_seal()` → componentes/aplicação e processamento. Prepare os dados do callback antes de iniciar o timer; inicializações de sensor/display podem ser coordenadas pela aplicação para permitir recuperação.

- `hal_system_init` padroniza infraestrutura e clocks do sistema, respeitando o startup já realizado pelo SDK, e inicializa a arena com armazenamento fornecido pela integração.
- Cada `hal_*_config` é um construtor completo do periférico, executado com a arena inicializada e aberta: valida capacidades, configura clocks periféricos, pinos, funções alternativas/roteamento e drivers necessários e entrega um handle válido. Não dependa de inicializações periféricas externas ocultas. A inicialização prévia documentada do sistema continua necessária.
- Os objetos concretos, como `hal_gpio_t`, ficam privados no porte e são alocados durante esses construtores. Não presuma reconfiguração em execução, destruição ou desalocação. Após uma reserva, uma falha do SDK pode consumir espaço e exigir reinício, conforme o contrato.

Decisões do módulo `hal/memory/`:

- Mantenha `hal_memory.c` e `hal_memory.h` na raiz de `memory/`, sem restaurar `src/` ou `include/`. O nome continua `memory`, com prefixo `hal_memory_*`.
- É um submódulo Git reutilizável, visando ESP32, nRF52840 e STM32F411/STM32F407; o porte STM32F4 comum às duas STM32 permanece uma intenção, não suporte já implementado.
- Use C11 ou posterior para `max_align_t`, `_Alignas` e `_Alignof`. A integração fornece um buffer externo persistente alinhado para `max_align_t` e escolhe sua capacidade; o módulo não define esse buffer.
- Há **um singleton privado no módulo de memória**, compartilhado entre todos os periféricos. `hal_memory_t` não é público nem pertence a `hal_system.c`; os portes chamam a API `hal_memory_*` diretamente. Não restaure uma arena por periférico nem os wrappers `hal_esp32_memory_require_open` e `hal_esp32_memory_alloc`.
- API: `hal_memory_init`, `hal_memory_alloc`, `hal_memory_seal`, `hal_memory_get_info`, com `hal_memory_status_t` independente de `hal_status_t`. Uma segunda inicialização após sucesso retorna `HAL_MEMORY_INVALID_STATE`, inclusive depois da selagem, sem alterar objetos vivos.
- Não restaure `HAL_MEMORY_INITIALIZER`, a exigência de zerar um contexto externo ou `heap_3_free`. Não há `free`, reset ou reabertura; selar é idempotente e impede novas reservas.
- A reserva preserva alinhamento para `max_align_t`, inclui preenchimento no consumo e não zera o bloco. Alinhamentos estendidos não são atendidos. Tamanho zero é inválido; falhas limpam a saída válida para `NULL` e não consomem espaço. Preserve as verificações contra overflow.
- Receba a reserva em uma variável `void *` e depois atribua ao ponteiro tipado; não converta `hal_gpio_t **` em `void **`. `used` inclui preenchimento e `remaining` não garante que uma reserva desse tamanho caiba após alinhamento.
- A arena é usada sequencialmente durante construção, sem mutex ou dependência de RTOS. A memória interna usada pelos drivers do SDK é separada dela.

## Temporização e contratos observáveis

- A HAL permite configurar um timer com período de **1 ms**. A integração pode contar notificações e fornecer essa contagem aos componentes/aplicações; não invente `hal_timer_get_millis`, ausente do contrato atual.
- O callback executa em interrupção: mantenha-o curto e não bloqueante. Sincronize a contagem compartilhada; no ESP32, respeite as exigências de IRAM e de operações atômicas sem bloqueio quando aplicáveis. A contagem representa notificações atendidas e pode subestimar tempo físico por latência/aglutinação.
- Intervalos e deadlines de aplicação devem considerar o wrap do contador de `uint32_t`, usando diferenças sem sinal. O mecanismo de tempo chega por injeção de dependência, como `get_ticks`, sem acesso direto ao SDK.
- I2C e UART têm timeout positivo em milissegundos definido na construção. Esse limite governa a espera do driver, não um prazo rígido para a chamada inteira. Processamento cooperativo não torna transferências síncronas assíncronas.

## Estado de referência da implementação

Este retrato contextualiza as decisões; confira os arquivos antes de assumir APIs ou funcionalidades.

- Há implementações de GPIO, I2C, UART, timer e sistema em `hal/ports/esp32-wroom-32/`; o BSP atual está em `bsp/esp32_devkit_v1/`, com interface `bsp/bsp.h`.
- Componentes presentes: `button`, `led`, `sht31d`, `ssd1306` e `telemetry`. O **próprio botão** já faz debounce, com padrão de 30 ms e fonte de tempo injetada; a aplicação apenas detecta um acionamento por pressão. Não adicione outro filtro baseado na descrição antiga do TCC.
- SHT31-D: aquisição single-shot e validação CRC; a aplicação espera ao menos `SHT31D_MEASUREMENT_TIME` após o retorno do comando de início, preserva a última amostra válida e decide quando repetir. Leituras usam 0,001 °C e 0,001 %RH.
- SSD1306: framebuffer 128×64, fontes e primitivas gráficas. A política visual fica na aplicação; a atualização normal do componente transfere no máximo uma página por chamada. A inicialização do controlador pode realizar várias transferências.
- `apps/` contém coordenação (`app_init`, `app_process`, `app_get_state`), aquisição, botão, interface, indicação e telemetria. A tela principal destaca temperatura e umidade (opção B); a segunda mostra última aquisição e tempo ativo. O LED fica apagado no startup, aceso quando saudável e pisca em falhas. A aplicação mantém uma fila limitada de erros e não repete automaticamente um registro UART parcialmente transmitido.
- `main/main.c` é a raiz de composição e execução cooperativa. Sensor e display compartilham I2C; UART0 em GPIO1/3 foi reservada à HAL, com console do SDK desativado em `sdkconfig.defaults`.

## Compilação e manutenção

- O SDK escolhido para este projeto é **ESP-IDF 6.1.x**, substituindo a escolha anterior de 5.3.6/`latest`. Não altere essa versão por inferência.
- Centralize fontes, includes e dependências no **`CMakeLists.txt` da raiz**. Não crie `CMakeLists.txt` em cada subpasta. Registre novos módulos no arquivo central.
- Os comandos do projeto estão em `scripts/build.py`, `clean.py`, `upload.py` e `monitor_serial.py`, com utilitários em `_common.py`. Use o ambiente ESP-IDF ativado por `export.sh`/`export.ps1`; a ativação depende das ferramentas e do ambiente Python do SDK estarem instalados.
- HAL pública, memória, porte ESP32 e componentes são submódulos Git: preserve alterações locais e confira o repositório afetado antes de modificar contratos. Não transforme uma mudança de código em atualização/publicação de submódulos sem escopo para isso.
