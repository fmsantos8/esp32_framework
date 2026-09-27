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
2. Cabeçalhos de `hal/interface/` não expõem tipos do fabricante. O porte implementa os símbolos `hal_*` declarados nesses cabeçalhos; cabeçalhos específicos do porte definem os tipos concretos dos handles para BSP/integração.
3. O BSP escolhe **qual** recurso físico representa uma função da placa; o porte define **como** executar uma operação comum nesse recurso. Decisões sobre clocks, pinos e compilação permanecem na região específica da placa/plataforma.
4. A composição pode conhecer todas as partes para conectá-las, mas as decisões de produto ficam na aplicação. A execução das tarefas pode ser um laço principal ou um mecanismo da plataforma; não há abstração comum de RTOS.
5. A configuração inicial de baixo nível cabe ao BSP/integração. Os cabeçalhos atuais também expõem configuração portável de GPIO, I2C e UART: eventual reconfiguração em execução usa esses contratos, sem acesso do componente ao BSP ou SDK. O dono da política e a coordenação do recurso compartilhado devem ser definidos no caso concreto.
6. Use o Doxygen do cabeçalho público para assinaturas e semântica vigentes. A proposta do TCC menciona, por exemplo, limites de tempo explícitos em I2C/UART, mas os cabeçalhos atuais não recebem esse parâmetro; trate isso como diferença entre proposta e implementação, não como contrato já existente.
7. Não suponha concorrência segura ou chamadas de ISR nos contratos comuns. Quando necessárias, a integração da plataforma deve coordenar o acesso; a proposta atribui à aplicação a serialização do I2C compartilhado.

## Estado desta árvore

O projeto atual contém `hal/interface/` (submódulo), o porte `hal/ports/esp32-wroom-32/`, `bsp/bsp_esp32.c` e `main/main.c`. O BSP ainda é um esqueleto. `main/main.c` concentra uma prova inicial do ESP32, incluindo mapeamento de pinos, comunicação com SHT31-D/SSD1306 e chamadas diretas do FreeRTOS. `app/` e `components/` ainda não existem nesta árvore. Ao evoluir para a arquitetura proposta, redistribuir essas responsabilidades conforme as fronteiras acima; não tratar o teste atual como desenho final.
