---
name: framework-architecture
description: Explicar e aplicar as responsabilidades dos módulos do framework embarcado deste repositório ao projetar, implementar ou revisar app, componentes, HAL, portes, BSP e integração. Não usar para tarefas alheias à arquitetura do firmware.
---

# Arquitetura do framework

Use este contexto ao decidir **onde colocar código**, quais dependências um módulo pode ter e se uma mudança é portável. A fonte da arquitetura pretendida é `68d497929590b37ccdc69df0/2-textuais/3-proposta.tex`, principalmente “Arquitetura geral e regra de dependências”, “Interfaces e contratos portáveis”, “Organização dos artefatos” e “Prova de conceito proposta”. Consulte os arquivos reais e a documentação Doxygen dos cabeçalhos para a API vigente. Se a proposta e o código divergirem, explicite a diferença; não apresente uma intenção do TCC como funcionalidade já implementada.

## Partes e responsabilidades

| Parte | Responsabilidade | Dependências permitidas |
| --- | --- | --- |
| `app/` (planejado) | Coordenar o comportamento da prova de conceito: aquisição periódica, estado da última amostra, seleção das duas telas, política de repetição após falhas e estados de indicação. As “tarefas” são módulos de contexto e processamento, sem pressupor um RTOS. | Interfaces públicas da aplicação e dos componentes. |
| `components/` (planejado) | Encapsular mecanismos reutilizáveis dos dispositivos e serviços; cada componente tem interface pública, contexto, inicialização e dependências explícitas. Recebe recursos válidos da integração, sem escolher a instância física. | Interfaces de outros componentes e contratos públicos da HAL. |
| `hal/interface/` | Definir em C os contratos portáveis de GPIO, I2C, UART, temporizador e códigos de estado. Expõe tipos de recurso opacos e semântica observável; não conhece estruturas do SDK. | Tipos C e outros contratos públicos da HAL. |
| `hal/ports/<plataforma>/` | Implementar as funções públicas da HAL usando o SDK selecionado; manter tipos concretos, estado do driver e tradução de erros. O mesmo porte STM32F4 deve atender às duas placas STM32 previstas. | HAL pública e SDK da plataforma; não incorpora pinos nem política de produto próprios de uma placa. |
| `bsp/` ou `boards/` | Descrever uma placa concreta: pinos, controladores, barramentos, polaridades, endereços e demais recursos físicos; realizar ou apoiar a inicialização de baixo nível. | HAL e SDK quando necessários; não conhece módulos de `app/` ou `components/`. |
| `main/` e arquivos de integração/compilação | Servir de raiz de composição: escolher porte e BSP, obter os recursos da placa, criar contextos, injetar dependências, inicializar e chamar o processamento da aplicação. Define como executar os módulos com o ambiente da plataforma. | Pode conhecer interfaces públicas, porte, BSP e SDK; não deve concentrar a política de produto da aplicação final. |
| SDK e hardware | Fornecer drivers, serviços de plataforma e dispositivos físicos. | Acesso concentrado no porte, no BSP e na integração. |
| `docs/` e testes | Documentar contratos, capacidades e versões; verificar comportamento equivalente dos portes e o isolamento das dependências. | Consultar os contratos e artefatos correspondentes. |

Os projetos convencionais de comparação, sem framework, ficam fora desta arquitetura; neles o uso direto do SDK é intencional.

## Componentes reutilizáveis previstos

- **Botão:** interpreta o GPIO de entrada e a polaridade fornecida pela placa, oferecendo o estado de acionamento sem expor o pino. Na proposta atual, a aplicação usa a contagem de tempo para fazer o debounce por software de aproximadamente 30 ms.
- **LED:** traduz comandos lógicos de indicação para o GPIO de saída, respeitando a polaridade configurada pela placa. A aplicação decide quando manter aceso, piscar após falha ou apagar.
- **SHT31-D:** emite comandos I2C, obtém temperatura e umidade, valida a resposta e devolve dados ou erro. Recebe o barramento I2C e o endereço; não decide repetição de leitura nem preservação da última amostra.
- **SSD1306:** encapsula a inicialização e os comandos/dados do controlador OLED via I2C. Recebe o mesmo barramento do sensor e seu próprio endereço; a aplicação decide qual tela e conteúdo apresentar.
- **Telemetria:** produz o formato comum das leituras e dos erros e os envia pela UART recebida. A aplicação decide quais eventos comunicar. Uma necessidade de reconfiguração em execução deve usar um contrato público da HAL e ser coordenada com quem compartilha a UART.
- **Temporização:** usa a contagem da HAL para apoiar prazos e intervalos sem bloqueio. A política de quando medir, atualizar tela ou piscar pertence à aplicação; a fonte física de tempo pertence ao porte/BSP.

O TCC cita os componentes como artefatos previstos. Os detalhes de suas APIs ainda precisam ser definidos a partir dos contratos atuais e dos requisitos da prova de conceito; não inferir nomes de funções a partir desta lista.

## Regras de fronteira

1. `app/` e `components/` não incluem headers do BSP, dos portes ou do SDK. Seus dados de placa chegam por parâmetros e handles; recursos compartilhados, como o barramento I2C do SHT31-D e do SSD1306, são injetados pela integração.
2. Cabeçalhos de `hal/interface/` não expõem tipos do fabricante. O porte implementa os símbolos `hal_*` declarados nesses cabeçalhos; as representações concretas ficam privadas ao porte. Os construtores recebem a configuração completa, incluindo pinos, e devolvem handles; BSP e integração não instanciam essas estruturas.
3. O BSP escolhe **qual** recurso físico representa uma função da placa; o porte define **como** executar uma operação comum nesse recurso. Decisões sobre clocks, pinos e compilação permanecem na região específica da placa/plataforma.
4. A composição pode conhecer todas as partes para conectá-las, mas as decisões de produto ficam na aplicação. A execução das tarefas pode ser um laço principal ou um mecanismo da plataforma; não há abstração comum de RTOS.
5. A configuração inicial de baixo nível cabe ao BSP/integração. Os construtores portáveis de GPIO, I2C, UART e timer são usados apenas durante a inicialização sequencial, entre `framework_memory_init()` e `framework_memory_seal()`. Os handles permanecem válidos durante toda a execução; não há destruição nem reconfiguração em runtime.
6. Use o Doxygen do cabeçalho público para assinaturas e semântica vigentes. I2C e UART recebem `timeout`, em milissegundos, na configuração do objeto; as transferências não recebem esse parâmetro. O contrato não garante prazo total rígido por chamada. Portes com limite fixo verificam na construção a correspondência com a configuração de compilação. Consulte `docs/contracts.md` para versões, capacidades e limites do ESP32.
7. Não suponha concorrência segura ou chamadas de ISR nos contratos comuns. Quando necessárias, a integração da plataforma deve coordenar o acesso; a proposta atribui à aplicação a serialização do I2C compartilhado.

## Estado desta árvore

O projeto contém `hal/interface/` (submódulo mantido com alterações locais), o porte `hal/ports/esp32-wroom-32/`, a arena compartilhada `memory/`, o BSP e o teste de integração em `main/main.c`. O BSP constrói os recursos usando apenas interfaces públicas. A integração verifica versões e capacidades, abre a arena, inicializa o BSP e sela as alocações. Os objetos são armazenados em bytes alinhados e acessados por `memcpy` através de estruturas locais tipadas, sem acesso direto às estruturas na arena.

`app/` e `components/` ainda não existem: o escopo desta migração preserva o teste atual, incluindo aquisição bloqueante, uma tela e o LED seguindo o botão. Isso não constitui a prova de conceito completa da proposta. Os testes de host em `tests/` verificam contratos com um SDK simulado; a montagem física e os outros portes ainda precisam de validação.

## Convenções de documentação

Escreva os comentários Doxygen em inglês. Use blocos multilinha antes de tipos, funções, macros e outras declarações, com `@brief` quando houver uma descrição. Valores de enumerações e campos de estruturas recebem comentários alinhados depois do membro, na mesma linha, no formato literal `/** Description */`, sem `@brief` e sem `<`, conforme a preferência explícita do usuário. Documente as unidades no texto dos comentários.

O cabeçalho Doxygen no topo de cada arquivo C ou H usa `@file`, `@author Fabricio Santos (fmsantos8@ucs.br)`, `@brief` e uma linha vazia antes de `*/`. Não use agrupamentos Doxygen (`@defgroup`, `@ingroup`, `@{`, `@}`). Preserve as regras técnicas dos contratos na documentação das declarações correspondentes. Separe os grupos de descrição, parâmetros (`@param`) e resultados (`@retval` ou `@return`) com uma linha vazia no bloco.

Não há cabeçalho agregador da HAL. Cada consumidor inclui explicitamente os contratos utilizados. Resolva includes a partir das raízes públicas ou privadas configuradas na compilação, como `common/hal_contract.h` e `gpio/hal_gpio.h`; não use `../` ou `./` em diretivas `#include`.
