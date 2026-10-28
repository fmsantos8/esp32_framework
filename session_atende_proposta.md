# Codex conversation

## User

preciso que voce analise proposta.pdf e verifique se o projeto atual atende a proposta de solução descrita.

## Assistant

Vou ler a proposta e usar a skill de arquitetura do repositório para comparar a solução descrita com o código atual, apontando o que está atendido, parcial ou ausente.

## Activity

    $ pwd && rg --files -g 'AGENTS.md' -g '*proposta*' -g '*Proposta*' -g 'CMakeLists.txt' -g 'README*' -g 'SKILL.md' -g 'sdkconfig*'
    /home/julia/Documentos/fabricio/esp32_framework
    components/led/README.md
    components/button/README.md
    components/telemetry/README.md
    components/ssd1306/README.md
    components/sht31d/README.md
    AGENTS.md
    proposta.pdf
    CMakeLists.txt
    hal/ports/esp32-wroom-32/README.md
    ✓ • 0ms

## Activity

    $ cat .agents/skills/framework-architecture/SKILL.md
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
    ✓ • 0ms

## Activity

    $ git status --short && rg --files -g '!managed_components/**' -g '!build/**' -g '!sdkconfig*' -g '!dependencies.lock' -g '!proposta.pdf' -g '!.git/**' -g '!.agents/**'
    ?? proposta.pdf
    app/button/app_button.h
    app/button/app_button.c
    app/status_led/app_status_led.c
    app/status_led/app_status_led.h
    app/telemetry/app_telemetry.h
    app/telemetry/app_telemetry.c
    app/telemetry/app_telemetry_private.h
    app/acquisition/app_acquisition.h
    app/acquisition/app_acquisition.c
    app/gui/bitmaps.h
    app/gui/render.h
    app/gui/app_gui.h
    app/gui/app_gui.c
    app/gui/bitmaps.c
    app/gui/render.c
    components/led/cmp_led_config.h
    components/led/cmp_led.h
    components/led/README.md
    components/led/cmp_led_types.h
    components/led/cmp_led.c
    components/button/cmp_button_config.h
    components/button/cmp_button.h
    components/button/cmp_button_types.h
    components/button/cmp_button.c
    components/button/README.md
    components/telemetry/cmp_telemetry.h
    components/telemetry/cmp_telemetry_types.h
    components/telemetry/cmp_telemetry_config.h
    components/telemetry/README.md
    components/telemetry/cmp_telemetry.c
    components/ssd1306/cmp_ssd1306_types.h
    components/ssd1306/cmp_ssd1306_fonts.c
    components/ssd1306/cmp_ssd1306_fonts_private.h
    components/ssd1306/cmp_ssd1306.h
    components/ssd1306/tools/generate_fonts.py
    components/ssd1306/README.md
    components/ssd1306/cmp_ssd1306_config.h
    components/ssd1306/cmp_ssd1306.c
    components/sht31d/cmp_sht31d.h
    components/sht31d/cmp_sht31d.c
    components/sht31d/README.md
    components/sht31d/cmp_sht31d_config.h
    bsp/bsp.h
    bsp/esp32_devkit_v1/bsp.c
    scripts/monitor_serial.py
    scripts/clean.py
    scripts/upload.py
    scripts/build.py
    scripts/_common.py
    AGENTS.md
    main/main.c
    hal/interface/gpio/hal_gpio.h
    hal/interface/system/hal_system.h
    hal/interface/timer/hal_timer.h
    hal/interface/common/hal_status.h
    hal/interface/common/hal_pin.h
    hal/interface/i2c/hal_i2c.h
    hal/interface/uart/hal_uart.h
    docs/pinout.md
    CMakeLists.txt
    hal/ports/esp32-wroom-32/hal_esp32_private.h
    hal/ports/esp32-wroom-32/gpio/hal_gpio.c
    hal/ports/esp32-wroom-32/hal_esp32.c
    hal/memory/hal_memory.h
    hal/ports/esp32-wroom-32/system/hal_system.c
    hal/memory/hal_memory.c
    hal/ports/esp32-wroom-32/timer/hal_timer.c
    hal/ports/esp32-wroom-32/README.md
    hal/ports/esp32-wroom-32/uart/hal_uart.c
    hal/ports/esp32-wroom-32/i2c/hal_i2c.c
    ✓ • 0ms

## Activity

    $ pdftotext -layout proposta.pdf /tmp/esp32_proposta.txt && cat /tmp/esp32_proposta.txt
                                                                                                   58
    
    
    3 PROPOSTA DE SOLUÇÃO
    
    
    
            O referencial teórico evidenciou que a portabilidade de firmware não depende apenas
    da existência de uma camada de abstração de hardware. Para que o reúso ocorra de forma
    sistemática, também é necessário impedir que tipos e convenções dos fabricantes alcancem a
    lógica comum, separar a configuração da placa das operações dos periféricos e definir contratos
    que possam ser implementados e verificados em plataformas distintas. As soluções analisadas
    atendem partes desse problema, mas deixam lacunas na combinação entre reúso da aplicação,
    reúso de componentes intermediários e reaproveitamento das adaptações de baixo nível entre
    projetos que utilizam a mesma plataforma.
            Diante dessas lacunas, propõe-se um framework modular, escrito em C, baseado em
    uma arquitetura em camadas e destinado a microcontroladores de 32 bits. A expectativa técnica
    é que interfaces comuns de acesso ao hardware, combinadas com portes de plataforma e BSPs
    separados, permitam manter inalterados a aplicação e os componentes reutilizáveis quando o
    firmware for executado em microcontroladores de fabricantes distintos.
            Este capítulo delimita o escopo da solução, deriva seus requisitos, descreve a arquitetura
    e os contratos previstos, define os artefatos a serem implementados e estabelece o procedimento
    de validação. A proposta não possui intuito de abstrair e implementar todos os periféricos e
    funcionalidades dos microcontroladores, limitando-se aos recursos necessários para implemen-
    tação da prova de conceito.
    
    
    3.1   DELIMITAÇÃO E REQUISITOS DA PROPOSTA
    
             O recorte da proposta compreende quatro placas baseadas em microcontroladores de 32
    bits: uma ESP32 DevKit V1 com ESP-WROOM-32 DevKit, uma Black Pill STM32F411CEU6,
    uma DevEBox com STM32F407VGT6 e uma Pro Micro com nRF52840. Apesar de haver qua-
    tro configurações de hardware, serão utilizados três ambientes de desenvolvimento: ESP-IDF,
    STM32CubeIDE e nRF Connect SDK. As duas placas STM32 foram incluídas deliberadamente
    para verificar se uma mesma implementação do porte baseado na STM32CubeF4 HAL pode ser
    reutilizada entre microcontroladores da família STM32F4, mantendo em BSPs, inicializações e
    arquivos de compilação as diferenças entre as placas (Espressif Systems, 2025; STMicroelectronics,
    2026; Nordic Semiconductor, 2026).
            As interfaces portáveis mínimas serão GPIO, I2C, UART e temporizador. A interface
    de temporização fornecerá o tempo decorrido em milissegundos a partir de um timer disponi-
    bilizado pela plataforma. Esse conjunto foi escolhido porque permite construir uma prova de
    conceito que reúne entrada digital, saída digital, dois dispositivos no mesmo barramento, comu-
    nicação serial e temporização, sem ampliar o trabalho para periféricos que não participam do
    cenário avaliado. SPI, ADC, conectividade sem fio, armazenamento persistente, DMA avançado
    e abstrações de RTOS não integram o escopo. O framework também não se propõe a oferecer
    suporte universal a microcontroladores de 8, 16 e 32 bits; as conclusões deverão permanecer
    limitadas às quatro placas e aos três ambientes selecionados.
            Para constituir uma linha de base, também serão desenvolvidas implementações equi-
    valentes a prova de conceito sem a utilização do framework conceito, uma em cada ambiente
    selecionado, acopladas diretamente às APIs e aos tipos do ESP-IDF, da STM32CubeF4 HAL e
    do nRF Connect SDK. A implementação baseada na STM32CubeF4 HAL será construída para
    as duas placas STM32, resultando em quatro versões convencionais executáveis. Essas versões
    não utilizarão as interfaces, os portes, os BSPs nem os componentes do framework, mas pode-
    rão adotar a decomposição em funções e arquivos normalmente empregada em cada ambiente.
    Assim, a linha de base não será deliberadamente monolítica; sua característica distintiva será a
    ausência da camada comum proposta entre a aplicação e os SDKs.
           O Quadro 2 apresenta os requisitos derivados das lacunas discutidas no referencial. Eles
    serão usados tanto para orientar a implementação futura quanto para impedir que decisões de
    baixo nível sejam acrescentadas sem relação com o problema investigado.
    
                                Quadro 2 – Requisitos da solução proposta
     Código      Requisito
                 Manter a aplicação e os componentes comuns independentes de cabeçalhos, tipos e chamadas
     RF01
                 dos SDKs selecionados.
                 Confinar as diferenças de microcontrolador e de placa aos portes da HAL, aos BSPs, à
     RF02
                 inicialização e à configuração de compilação.
                 Permitir que um mesmo porte da HAL seja empregado por diferentes aplicações que utilizem a
     RF03
                 mesma plataforma, sem incorporar política de produto.
                 Definir contratos observáveis para GPIO, I2C, UART e temporizador em milissegundos,
     RF04        incluindo erros, timeout, bloqueio, inicialização, propriedade de recursos, concorrência e
                 operações não suportadas.
                 Manter tarefas, filas, semáforos, escalonamento e demais primitivas de RTOS fora do contrato
     RF05
                 central do framework.
                 Fornecer componentes reutilizáveis que recebam suas dependências por interfaces comuns e
     RF06
                 não configurem diretamente o hardware.
                 Identificar a versão dos contratos implementados por cada porte e verificar sua conformidade
     RF07
                 antes da integração.
                 Executar a mesma prova de conceito nas quatro placas, preservando os arquivos-fonte comuns
     RF08
                 e o comportamento externo definido.
                 Reutilizar sem alteração o mesmo porte STM32CubeF4 nas construções para
     RF09
                 STM32F411CEU6 e STM32F407VGT6, mantendo as diferenças de placa fora desse porte.
                 Implementar versões equivalentes sem utilização do framework , acopladas diretamente aos
     RF10        três SDKs, para comparar de forma pareada o código compartilhado, o código mantido, a
                 dispersão das dependências plataforma e o uso de memória.
    Fonte: O Autor (2026).
    
    
    
             A proposta resultará em um conjunto de artefatos, e não apenas em um diagrama arqui-
    tetural. Estão previstos os cabeçalhos das interfaces comuns, as implementações dessas interfa-
    ces para cada plataforma, os BSPs, os componentes reutilizáveis, uma aplicação demonstradora
    
    
                                                        59
    e os projetos sem a utilziação do framework. A separação desses artefatos permite avaliar indi-
    vidualmente a estabilidade das interfaces, o isolamento do código específico, a preservação da
    lógica comum e as diferenças entre as duas formas de implementação.
    
    
    3.2   ARQUITETURA GERAL E REGRA DE DEPENDÊNCIAS
    
            A arquitetura proposta seguirá a divisão de responsabilidades apresentada na Figura 4,
    na qual a camada de aplicação reunirá as tarefas responsáveis pela execução da prova de con-
    ceito. Cada tarefa coordenará os componentes relacionados à sua responsabilidade, enquanto o
    acesso aos periféricos permanecerá encapsulado nos componentes reutilizáveis de botão, LED,
    SHT31-D, SSD1306, telemetria e temporização. Desse modo, nem as tarefas da aplicação nem
    esses componentes acessarão diretamente BSPs ou serviços dos fabricantes, pois toda interação
    com o hardware ocorrerá por meio das interfaces da HAL.
            A tradução dos contratos comuns para cada ambiente de desenvolvimento ficará a cargo
    dos portes da HAL, ao passo que o BSP identificará pinos, instâncias de barramentos, periféri-
    cos e os demais recursos concretos de cada placa. Como essas responsabilidades permanecerão
    separadas, placas atendidas pelo mesmo ambiente e por contratos compatíveis poderão selecio-
    nar um único porte, ainda que necessitem de BSPs distintos.
    
                             Figura 4 – Arquitetura do framework proposto
                                              Aplicação (app)
                               Tarefas que irão controlar todos os dispositivos
    
                          Componentes e serviços reutilizáveis (components)
                   Botão, LED, SHT31-D, display SSD1306, telemetria e temporização
    
                                        Interfaces da HAL (hal)
                            GPIO, I2C, UART, temporizador em milissegundos
                                  implementa
    
                     Portes da HAL (ports)                 Board Support Package (bsp)
                  ESP32, STM32F4 e nRF52840                Pinos, barramentos e instâncias
    
    
                                     SDK e suporte de plataforma
                            ESP-IDF, STM32CubeF4 HAL e nRF Connect SDK
    
                                                Hardware
                             Microcontrolador, placa e dispositivos conectados
               Fonte: elaborada pelo autor.
    
    
    
           A direção das dependências será preservada também nas inclusões de cabeçalhos. Os
    arquivos da aplicação dependerão somente das interfaces públicas da própria aplicação e dos
    componentes reutilizáveis, enquanto estes poderão utilizar interfaces de outros componentes e
    
                                                      60
    os contratos comuns da HAL, sem incluir cabeçalhos de portes, BSPs ou SDKs. As interfaces
    da HAL permanecerão independentes de suas implementações concretas, cabendo a cada porte
    incluir apenas os contratos que implementa e os cabeçalhos do respectivo SDK. De forma se-
    melhante, o BSP poderá utilizar as interfaces da HAL e o SDK para descrever e inicializar os
    recursos da placa, mas não conhecerá módulos da aplicação ou dos componentes.
            Embora portes e BSPs ocupem a mesma região inferior da arquitetura, suas responsabi-
    lidades não se confundem: a HAL e seus portes expressarão operações portáveis, como escrever
    em um GPIO ou realizar uma transferência I2C, enquanto o BSP indicará quais instâncias con-
    cretas representam o botão, o LED, o sensor, o display e a UART de telemetria em determinada
    placa. Permanecerão no BSP e nos elementos de integração as decisões sobre clocks, pinos,
    barramentos e demais particularidades da compilação, de modo que a proposta não elimine as
    diferenças entre plataformas, mas determine onde elas deverão residir.
            Nas duas placas STM32, essa separação será exercitada pela utilização dos mesmos
    arquivos-fonte do porte STM32F4 em ambas as construções, combinados com um BSP, um
    código de inicialização e uma configuração de compilação próprios para cada placa. Pinos, ins-
    tâncias de periféricos, clocks e demais características dos microcontroladores deverão perma-
    necer nesses elementos específicos; caso a integração exija modificar o porte ou introduzir nele
    condicionais destinadas a apenas um dos dispositivos, a alteração será tratada como evidência
    de insuficiência da fronteira proposta.
            A divisão da aplicação em tarefas será mantida independente da presença de um RTOS,
    pois, no código comum, cada tarefa corresponderá a um módulo com contexto próprio e ope-
    rações de inicialização e processamento. A forma de executar essas operações ficará no código
    específico de cada plataforma: nas configurações bare-metal, elas poderão ser chamadas pelo
    laço principal, enquanto no nRF Connect SDK poderão ser associadas a contextos fornecidos
    pelo Zephyr. Essa variação permanecerá fora do núcleo do framework, que não criará uma ca-
    mada de abstração para sistema operacional e nem buscará uniformizar tarefas, filas, mutexes
    ou escalonadores.
            Como os módulos comuns não conhecerão o BSP, os portes ou os SDKs, a ligação entre
    essas partes ficará concentrada no código de integração específico de cada placa. Esse código
    reunirá os arquivos comuns, o porte da HAL, o BSP, a inicialização e a configuração de compi-
    lação necessárias à geração do firmware. Seu ponto de entrada, representado, por exemplo, por
    um arquivo main.c, funcionará como raiz de composição ao obter os recursos concretos da
    placa, criar os contextos das tarefas e dos componentes, fornecer suas dependências e iniciar o
    processamento da aplicação.
            Para realizar essa montagem, a raiz de composição poderá conhecer simultaneamente
    as interfaces públicas da aplicação e dos componentes, as interfaces e o porte selecionado da
    HAL, o BSP e os elementos necessários do SDK. Esse conhecimento ficará restrito à integração
    do executável, sem incorporar políticas que são responsabilidades da aplicação. A composição,
    
                                                  61
    portanto, não constitui uma camada adicional da arquitetura mostrada na Figura 4, mas o ponto
    em que as implementações concretas serão conectadas ao código comum sem criar dependência
    direta entre a aplicação e o BSP.
            As implementações sem a utilização do framework permanecerão fora da arquitetura
    mostrada na Figura 4. Em cada uma delas, a aplicação e os módulos funcionais poderão utilizar
    diretamente os recursos do respectivo SDK, sem reproduzir sob outros nomes os contratos, os
    portes ou os BSPs propostos. Essa delimitação evita atribuir à linha de base os mecanismos
    cuja contribuição se pretende avaliar, mas não impede o uso de funções, arquivos separados e
    recursos fornecidos pelo próprio ambiente de desenvolvimento.
    
    
    3.3   INTERFACES E CONTRATOS PORTÁVEIS
    
            As interfaces da HAL serão definidas em cabeçalhos públicos escritos em C. Os re-
    cursos concretos serão representados por handles opacos, evitando que estruturas ou tipos dos
    fabricantes sejam expostos aos componentes. A inicialização de baixo nível ficará sob respon-
    sabilidade do BSP e do código de composição; os componentes receberão referências já válidas
    para os recursos que utilizam e não assumirão sua propriedade.
            Todas as operações retornarão códigos de estado pertencentes ao contrato comum. O
    conjunto mínimo distinguirá sucesso, erro geral, timeout, argumento inválido e operação não
    suportada. Operações de I2C e UART receberão um limite de tempo explícito e terão comporta-
    mento síncrono no recorte inicial. Chamadas a partir de rotinas de interrupção e uso concorrente
    do mesmo handle não serão garantidos pela interface comum; quando necessários, deverão ser
    tratados pela integração específica da aplicação. Essa restrição evita prometer reentrância ou
    segurança entre tarefas sem que a semântica possa ser demonstrada de forma equivalente nos
    três ambientes.
            O Quadro 3 relaciona cada interface às operações necessárias para a prova de conceito
    e aos aspectos que deverão ser especificados antes da implementação.
    
                                 Quadro 3 – Interfaces portáveis previstas
     Interface        Operações mínimas                   Elementos do contrato
                                                          Estado de inicialização, polaridade definida pelo
     hal_gpio         Leitura e escrita de nível lógico
                                                          BSP, argumentos inválidos e propriedade do recurso.
                                                          Endereço do dispositivo, buffers, comprimento,
                      Escrita, leitura e transação
     hal_i2c                                              timeout, erros de barramento e serialização do acesso
                      combinada
                                                          compartilhado.
                                                          Instância utilizada, validade do buffer, tamanho do
     hal_uart         Transmissão de bloco de dados       buffer, comportamento bloqueante, timeout e
                                                          transmissão parcial.
                       Leitura do tempo decorrido em      Fonte de temporização da plataforma, resolução de 1
     hal_timer
                       milissegundos                      ms.
    Fonte: O Autor (2026).
    
    
    
    
                                                          62
            O compartilhamento do barramento I2C entre o SHT31-D e o display exigirá que am-
    bos recebam a mesma instância de hal_i2c, mantendo endereços e estado próprios em seus
    contextos. A aplicação será responsável por não iniciar operações concorrentes sobre o mesmo
    barramento. Essa decisão mantém o mecanismo de comunicação reutilizável sem impor mute-
    xes ou uma política de escalonamento ao framework.
    
    
    3.4   ORGANIZAÇÃO DOS ARTEFATOS
    
           Os artefatos serão organizados de acordo com sua estabilidade e responsabilidade. O
    Quadro 4 apresenta a estrutura lógica prevista dos arquivos. Os nomes indicam fronteiras de
    responsabilidade, mas não fixam antecipadamente todos os detalhes do sistema de compilação,
    que deverão acompanhar as ferramentas de cada plataforma.
    
                              Quadro 4 – Artefatos previstos para o framework
       Grupo                        Conteúdo e responsabilidade
                                    Cabeçalhos públicos, códigos de estado, tipos opacos, versões e
       hal/interface
                                    documentação dos contratos.
                                    Tradução das interfaces para ESP-IDF, STM32CubeF4 HAL, nRF
       hal/ports
                                    Connect SDK.
                                    Mapeamento de pinos, instâncias de periféricos, endereços dos
       boards
                                    dispositivos e composição específica de cada placa.
                                    Componentes de botão, LED, SHT31-D, SSD1306 e telemetria,
       components
                                    dependentes apenas das interfaces comuns.
                                    Estado e fluxo da aplicação, seleção da tela e tratamento dos eventos
       app
                                    produzidos pelos componentes.
      docs                          Contratos, capacidades suportadas, versões.
     Fonte: O Autor (2026).
    
    
    
            Os componentes não serão apenas arquivos de implementação. Cada um deverá possuir
    interface pública, contexto próprio, parâmetros de inicialização e documentação de dependên-
    cias. O componente do SHT31-D, por exemplo, receberá uma referência para I2C e o endereço
    do dispositivo; o componente de display receberá a mesma interface de barramento e manterá
    seu próprio contexto.
            Além dos diretórios de primeiro nível apresentados na Quadro 4, a estrutura interna
    seguirá a mesma separação de responsabilidades. Em hal/interface, cada contrato por-
    tável será mantido em um subdiretório próprio, como GPIO, I2C, UART e temporizador. Em
    hal/ports, haverá um subdiretório para cada porte selecionado, e cada porte conterá sua
    própria divisão por periférico implementado. O mesmo critério será aplicado aos demais gru-
    pos: placas, componentes, tarefas da aplicação, testes e documentação deverão ser separados
    por unidade de responsabilidade, evitando que arquivos de naturezas distintas sejam agrupados
    apenas por conveniência.
    
    
    
    
                                                        63
    3.5   PROVA DE CONCEITO PROPOSTA
    
             A prova de conceito será uma aplicação que irá medir temperatura e umidade do am-
    biente, exibindo as informações em um display. Um sensor SHT31-D fornecerá temperatura
    e umidade por I2C, enquanto um display Organic Light-Emitting Diode (OLED) de 0,96 po-
    legada, resolução de 128 por 64 pontos, controlador SSD1306 e interface I2C apresentará as
    informações ao usuário. O sensor e o controlador selecionados oferecem as interfaces necessá-
    rias a essa composição (Sensirion, 2026; Solomon Systech, 2026).
            Os módulos de sensor e display previstos são apresentados na Figura 5. Ambos com-
    partilharão o mesmo barramento I2C, o que permitirá exercitar a composição de mais de um
    componente sobre uma única interface portável.
    
                    Figura 5 – Dispositivos I2C previstos para a prova de conceito
    
    
    
    
            (a) Módulo sensor SHT31-D                            (b) Módulo de display OLED SSD1306
                            Fonte: (a) Mercado Livre (s.d.c); (b) MakerHero (s.d.).
    
    
            Os dispositivos externos de GPIO previstos são apresentados na Figura 6. O botão for-
    necerá a entrada digital usada para alternância das telas, enquanto o LED será empregado como
    indicação visual do estado da aquisição.
    
                   Figura 6 – Dispositivos GPIO previstos para a prova de conceito
    
    
    
    
       (a) Botão tátil momentâneo de 6 x 6 mm                       (b) LED amarelo difuso de 5 mm
                          Fonte: (a) Mercado Livre (s.d.a); (b) Mercado Livre (s.d.b).
    
    
            Para realizar as operações de entrada e saída digital, a montagem utilizará também um
    botão tátil momentâneo, normalmente aberto, e um LED, ambos externos às placas. Serão em-
    pregados os mesmos componentes e circuitos nas quatro configurações microcontroladores,
    
                                                      64
    mesmo quando a placa disponibilizar algum recurso embarcado equivalente, evitando que dife-
    renças de polaridade, ligação elétrica ou finalidade desses recursos alterem a prova de conceito.
    O botão será conectado entre o GPIO e o terra, com um resistor externo de 10 kΩ entre a en-
    trada e 3.3 V, resultando em acionamento ativo em nível baixo. O LED será conectado a uma
    saída GPIO por meio de um resistor em série de 1 kΩ, com acionamento ativo em nível alto.
    Os pinos e as polaridades serão descritos pelo BSP de cada placa, sem alterar os componentes
    reutilizáveis.
             A aplicação realizará leituras periódicas do sensor em um intervalo configurado pre-
    viamente. A primeira tela mostrará os valores atuais de temperatura e umidade. A segunda
    apresentará informações de diagnóstico reduzidas ao estado da última aquisição e ao tempo de
    operação. Um único acionamento do botão alternará entre as duas telas, com debounce de apro-
    ximadamente 30 ms realizado por software; não haverá pressionamento longo, menus, alteração
    de parâmetros ou persistência de configuração. O LED permanecerá aceso durante a operação
    normal, piscará quando houver falha de comunicação e ficará apagado antes da conclusão da
    inicialização. A UART transmitirá as leituras e os códigos de erro em um formato comum às
    quatro placas.
            Quando uma operação I2C não for concluída, a aplicação preservará o último valor
    válido, apresentará o estado de erro, atualizará o LED e registrará a ocorrência pela UART.
    A política de repetição permanecerá na aplicação, enquanto o componente do sensor apenas
    devolverá o resultado da operação. Essa separação permite verificar se os contratos de erro são
    mantidos pelos três portes de plataforma e pelas quatro configurações de placa sem transferir
    decisões do produto para a biblioteca reutilizável.
             A temporização será não bloqueante. Em cada plataforma, um timer fornecerá à in-
    terface comum a contagem do tempo decorrido em milissegundos. A aplicação utilizará essa
    contagem para determinar o instante da próxima aquisição, realizar o debounce do botão e con-
    trolar a indicação visual de falha. A aplicação comum será processada por chamadas sucessivas,
    sem criar tarefas próprias. Com isso, o mesmo fluxo poderá ser integrado ao laço principal ou a
    uma tarefa fornecida pelo ambiente de plataforma sem que componentes intermediários passem
    a depender de uma API de RTOS.
             O Quadro 5 resume as quatro configurações previstas. Em todas elas serão mantidos
    o mesmo sensor, display, botão, LED, circuitos externos e comportamento da aplicação. Nas
    mudanças entre ambientes de desenvolvimento serão substituídos o porte, o BSP, a inicialização
    e o sistema de compilação. Entre as duas placas STM32, o porte deverá permanecer inalterado,
    e somente os demais elementos específicos da placa serão substituídos.
            Cada placa receberá duas versões funcionalmente equivalentes do firmware: uma cons-
    truída com o framework e outra derivada da implementação convencional correspondente ao seu
    ambiente. As duas versões utilizarão os mesmos dispositivos externos, mapeamento de pinos,
    intervalo de aquisição, formato de telemetria, políticas de erro e cenários de teste.
    
                                                   65
                         Quadro 5 – Plataformas previstas para a prova de conceito
     Placa                                 Microcontrolador               Ambiente e adaptação
     ESP32 ESP-WROOM-32 DevKit
                                           ESP32                          ESP-IDF e porte específico da HAL.
     V1
                                                                          STM32CubeF4 HAL, porte STM32F4
     Black Pill STM32F411CEU6              STM32F411CEU6
                                                                          compartilhado e BSP próprio.
                                                                          STM32CubeF4 HAL, mesmo porte da
     DevEBox STM32F407VGT6                 STM32F407VGT6
                                                                          STM32F411CEU6 e BSP próprio.
                                                                          nRF Connect SDK, configuração da
     Pro Micro nRF52840                    nRF52840
                                                                          placa e porte específico da HAL.
    Fonte: O Autor (2026).
    
    
    
            As placas de desenvolvimento selecionadas são apresentadas na Figura 7. A identifica-
    ção visual complementa o Quadro 5 e delimita as configurações físicas que serão utilizadas na
    implementação futura.
    
                Figura 7 – Placas de desenvolvimento previstas para a prova de conceito
    
    
    
    
       (a) ESP32 ESP-WROOM-32 DevKit V1                                    (b) Black Pill STM32F411CEU6
    
    
    
    
           (c) DevEBox STM32F407VGT6                                           (d) Pro Micro nRF52840
            Fonte: (a) Voltaat (s.d.); (b) Mercado Livre (s.d.e); (c) Mercado Livre (s.d.d); (d) Elecrow (s.d.).
    
    
    
    
    3.6    PLANO DE VALIDAÇÃO
    
            A proposta será considerada funcionalmente demonstrada somente se os mesmos cená-
    rios forem executados nas duas versões de cada uma das quatro placas: inicialização e aquisição
    válida; apresentação das duas telas; alternância pelo botão; transmissão pela UART; repetição
    periódica; e tratamento observável de falha na comunicação com o sensor. Os valores medidos
    poderão variar com o ambiente, mas a sequência de estados e as respostas a cada evento deverão
    permanecer equivalentes.
    
    
                                                            66
            A evidência estrutural verificará se a portabilidade foi obtida pela arquitetura proposta,
    e não por cópias adaptadas da aplicação. Para isso, os arquivos da aplicação, dos componentes
    e das interfaces comuns deverão ser iguais entre as quatro construções baseadas no framework.
    Também será verificada a ausência de cabeçalhos e símbolos dos fabricantes nesses arquivos.
    Cada porte deverá executar os mesmos contratos e declarar explicitamente as capacidades que
    oferece. Adicionalmente, as versões STM32F411CEU6 e STM32F407VGT6 deverão referen-
    ciar exatamente os mesmos arquivos do porte STM32CubeF4, podendo apenas alterar as macros
    de configuração do SDK do fabricante.
            O percentual de código compartilhado será calculado para cada placa, separadamente
    nas versões baseadas no framework e nas implementações convencionais, conforme a Equação
    3.1. Serão consideradas apenas linhas de código-fonte e cabeçalhos desenvolvidos no trabalho.
    Código dos fabricantes, arquivos gerados, produtos de compilação e testes não participarão do
    cálculo.
    
                                                 Lcomum
                                  Pcomp =                        × 100                           (3.1)
                                            Lcomum + Lespecífico
    
             Nessa expressão, Lcomum corresponde às linhas dos mesmos arquivos que são incluí-
    dos, sem alteração e sem condicionais de compilação específicas de placa ou SDK, nas quatro
    construções da forma de implementação avaliada. Por sua vez, Lespecífico corresponde às linhas
    necessárias à placa considerada que não integram esse conjunto comum, incluindo porte, BSP,
    inicialização e integração. Cada placa e forma de implementação terão o percentual de código
    compartilhado calculados individualmente.
            Assim, a métrica indicará qual parcela do código de cada versão provém de artefatos
    comuns mantidos em uma única origem. Ela não será interpretada isoladamente como medida
    de qualidade ou de esforço de desenvolvimento, pois um percentual elevado também poderia
    resultar de interfaces excessivamente limitadas. Sua análise será combinada com os cenários
    funcionais, o volume total de código mantido e a dispersão das dependências de plataforma.
            Para esse cálculo, o porte STM32CubeF4 permanecerá classificado como código espe-
    cífico do ambiente STM32, pois não é compartilhado com ESP32 e nRF52840. Seu reapro-
    veitamento entre as duas placas STM32 será contabilizado separadamente pela identidade dos
    arquivos e de seus hashes. Essa separação evita somar o mesmo reúso simultaneamente ao per-
    centual global e à evidência intrafamília.
            A conformidade dos portes será verificada por um roteiro comum associado aos con-
    tratos da HAL. Esse roteiro não terá como objetivo demonstrar cobertura exaustiva dos perifé-
    ricos, mas confirmar que cada porte implementa o mesmo comportamento observável exigido
    pela prova de conceito. Para cada interface, serão avaliadas a inicialização, a validade dos argu-
    mentos, os códigos de retorno, o tratamento de timeout, a indicação explícita de operações não
    suportadas e a ausência de tipos específicos do SDK nas interfaces públicas. Quando um com-
    
                                                   67
    portamento não puder ser exercitado diretamente em todas as placas, a limitação será registrada
    como ameaça à validade, sem ser tratada como conformidade plena.
            O Quadro 6 apresenta as métricas e os respectivos procedimentos de coleta.
    
                               Quadro 6 – Métricas previstas para a validação
     Métrica ou evidência           Procedimento de coleta                         Finalidade
                                    Executar o mesmo roteiro nas duas versões
     Cenários funcionais                                                           Verificar equivalência de
                                    das quatro placas e registrar saídas do
     aprovados                                                                     comportamento.
                                    display, LED e UART.
                                    Calcular Pcomp,p para cada plataforma e
                                                                                   Comparar o reúso efetivo
     Código compartilhado           forma de implementação com as exclusões
                                                                                   de arquivos-fonte.
                                    definidas.
                                                                                   Detectar mudanças
                                    Comparar caminhos e hashes ao integrar
     Arquivos comuns alterados                                                     distribuídas no núcleo
                                    cada plataforma.
                                                                                   comum.
                                    Comparar caminhos e hashes do porte nas        Verificar o reúso integral
     Reúso do porte STM32F4         construções STM32F411CEU6 e                    dentro da família
                                    STM32F407VGT6.                                 selecionada.
                                    Contar linhas do porte, BSP, inicialização e   Dimensionar a adaptação
     Código específico
                                    integração por plataforma.                     necessária.
                                    Aplicar um roteiro comum de verificação dos    Verificar se os portes
                                    contratos da HAL, cobrindo inicialização,      apresentam
                                    argumentos inválidos, erros, timeouts,         comportamento
     Conformidade dos portes
                                    operações não suportadas, capacidades          compatível com o contrato
                                    declaradas e ausência de tipos do SDK nas      comum dentro do recorte
                                    interfaces públicas.                           da prova de conceito.
                                    Registrar os relatórios de compilação das      Comparar de forma
     Uso de flash e RAM             duas versões de cada placa sob configurações   pareada o custo de
                                    equivalentes.                                  memória.
    Fonte: O Autor (2026).
    
    
    
            Não serão estabelecidas antecipadamente metas numéricas de percentual compartilhado
    ou consumo de memória, pois não há medição preliminar que sustente limites arbitrários. Os
    critérios de aceitação serão: execução dos cenários nas quatro placas; identidade dos arquivos-
    fonte comuns; ausência de dependências de fabricante nesses arquivos; aprovação dos testes
    de conformidade; identidade do porte STM32CubeF4 nas duas placas STM32; e concentração
    das demais adaptações nos portes, BSPs, inicializações e arquivos de compilação. Caso um
    desses critérios não seja atendido, a arquitetura deverá ser revisada ou a limitação deverá ser
    explicitada nos resultados.
    
    
    3.7    CRONOGRAMA DE IMPLEMENTAÇÃO
    
           O desenvolvimento será distribuído ao longo de 14 semanas, conforme o Quadro 7. A
    sequência parte da estabilização dos contratos e dos componentes comuns, avança pelas plata-
    formas e reserva as semanas finais para validação integrada, coleta das métricas e correções.
    Para reduzir diferenças acidentais de comportamento, cada implementação convencional será
    construída logo após a versão correspondente baseada no framework.
    
    
                                                       68
                      Quadro 7 – Cronograma de implementação em 14 semanas
     Semana       Atividade principal
                  Criar os contratos de GPIO, I2C, UART e temporização e os mapeamentos das
     1
                  placas
     2            Preparar os ambientes de desenvolvimento com cada um dos SDK.
     3            Implementar o porte para ESP-IDF e o BSP da ESP32.
                  Implementar o porte STM32CubeF4 e os BSPs das placas STM32F411CEU6 e
     4
                  STM32F407VGT6.
     5            Implementar o porte para nRF Connect SDK e o BSP da nRF52840.
                  Aplicar o roteiro comum de conformidade aos três portes e corrigir divergências em
     6
                  relação aos contratos.
                  Implementar e testar os componentes de botão, LED, temporização e telemetria
     7
                  sobre as interfaces comuns.
     8            Implementar os componentes SHT31-D e SSD1306 e compor a aplicação comum.
                  Integrar a aplicação baseada no framework à ESP32, implementar a versão
     9
                  convencional para ESP-IDF e executar a verificação funcional pareada.
                  Integrar a aplicação baseada no framework à STM32F411CEU6, implementar a
     10
                  versão convencional correspondente e executar a verificação funcional pareada.
                  Integrar a STM32F407VGT6 nas duas formas de implementação, preservando o
     11
                  porte STM32CubeF4, e executar a verificação funcional pareada.
                  Integrar a aplicação baseada no framework à nRF52840, implementar a versão
     12
                  convencional para nRF Connect SDK e executar a verificação funcional pareada.
                  Executar os cenários funcionais completos e os casos de falha nas oito versões e
     13           coletar código compartilhado, código mantido, dispersão das dependências de
                  plataforma, uso de flash e RAM.
                  Corrigir não conformidades, repetir as medições afetadas e consolidar a
     14
                  documentação e as evidências da implementação.
    Fonte: O Autor (2026).
    
    
    
    
                                                    69
    
    ✓ • 28ms

## Activity

    $ nl -ba main/main.c && cat CMakeLists.txt && cat .gitmodules
         1    #include <assert.h>
         2    #include <stdatomic.h>
         3    #include <stddef.h>
         4    #include <stdint.h>
         5
         6    #include "esp_attr.h"
         7    #include "freertos/FreeRTOS.h"
         8    #include "freertos/task.h"
         9
        10    #include "acquisition/app_acquisition.h"
        11    #include "bsp.h"
        12    #include "button/app_button.h"
        13    #include "cmp_button_config.h"
        14    #include "cmp_led_config.h"
        15    #include "cmp_sht31d_config.h"
        16    #include "cmp_ssd1306_config.h"
        17    #include "cmp_telemetry_config.h"
        18    #include "gui/app_gui.h"
        19    #include "status_led/app_status_led.h"
        20    #include "system/hal_system.h"
        21    #include "telemetry/app_telemetry.h"
        22
        23    #ifdef NDEBUG
        24    #error "This firmware requires assertions to be enabled"
        25    #endif
        26
        27    #define HAL_STORAGE_CAPACITY 1024U /** HAL Memory Arena Storage (bytes) */
        28
        29    static _Alignas(max_align_t) unsigned char hal_storage[HAL_STORAGE_CAPACITY]; /** Shared HAL arena storage. */
        30    static _Atomic uint32_t timer_notifications = 0;                              /** Serviced one-millisecond timer notifications. */
        31
        32    static bsp_resources_t board; /** Resources constructed by the selected board. */
        33    static led_t led;             /** Status LED component context. */
        34    static telemetry_t telemetry; /** UART telemetry component context. */
        35    static button_t button;       /** Button component context. */
        36    static sht31d_t sensor;       /** Sensor component context. */
        37    static ssd1306_t display;     /** Display component context and framebuffer. */
        38
        39    _Static_assert(ATOMIC_INT_LOCK_FREE == 2, "Timer callback requires a lock-free counter");
        40
        41    static void IRAM_ATTR timer_tick(void) {
        42        atomic_fetch_add_explicit(&timer_notifications, 1, memory_order_relaxed);
        43    }
        44
        45    static uint32_t get_ticks(void) {
        46        return atomic_load_explicit(&timer_notifications, memory_order_relaxed);
        47    }
        48
        49    static void init_platform(void) {
        50        const hal_system_config_t system_config = {
        51            .memory_buffer = hal_storage,
        52            .memory_capacity = (uint32_t)sizeof(hal_storage),
        53        };
        54        assert(hal_system_init(&system_config) == HAL_STATUS_OK);
        55
        56        assert(bsp_init(timer_tick, &board) == HAL_STATUS_OK);
        57        assert(hal_system_seal() == HAL_STATUS_OK);
        58    }
        59
        60    static void init_components(void) {
        61        const led_config_t led_config = {
        62            .gpio = board.led,
        63            .active_level = board.led_active_level,
        64        };
        65        assert(cmp_led_init(&led, &led_config) == LED_STATUS_OK);
        66
        67        const telemetry_config_t telemetry_config = {
        68            .uart = board.telemetry_uart,
        69        };
        70        assert(cmp_telemetry_init(&telemetry, &telemetry_config) == TELEMETRY_STATUS_OK);
        71
        72        const button_config_t button_config = {
        73            .gpio = board.button,
        74            .active_level = board.button_active_level,
        75            .get_ticks = get_ticks,
        76        };
        77        assert(cmp_button_init(&button, &button_config) == BUTTON_STATUS_OK);
        78
        79        const sht31d_config_t sensor_config = {
        80            .i2c = board.devices_i2c,
        81            .address = board.sht31_address,
        82        };
        83        assert(cmp_sht31d_init(&sensor, &sensor_config) == SHT31D_STATUS_OK);
        84
        85        const ssd1306_config_t display_config = {
        86            .i2c = board.devices_i2c,
        87            .address = board.ssd1306_address,
        88        };
        89        assert(cmp_ssd1306_init(&display, &display_config) == SSD1306_STATUS_OK);
        90    }
        91
        92    static void init_application(void) {
        93        assert(app_status_led_init(get_ticks));
        94        assert(app_telemetry_init(get_ticks));
        95        assert(app_acquisition_init(get_ticks));
        96        assert(app_gui_init(get_ticks));
        97        app_button_init();
        98    }
        99
       100    void app_main(void) {
       101        init_platform();
       102        init_components();
       103        init_application();
       104
       105        assert(hal_timer_start(board.timebase_timer) == HAL_STATUS_OK);
       106
       107        app_status_led_set_ready(true);
       108
       109        while (true) {
       110            app_button_run(&button);
       111            app_acquisition_run(&sensor);
       112            app_gui_run(&display);
       113            app_status_led_run(&led);
       114            app_telemetry_run(&telemetry);
       115            vTaskDelay(1);
       116        }
       117    }
    cmake_minimum_required(VERSION 3.22)
    
    set(HAL_ESP32_INTERFACE_DIR "${CMAKE_CURRENT_LIST_DIR}/hal/interface" CACHE PATH "Portable HAL interface directory")
    set(HAL_ESP32_MEMORY_DIR "${CMAKE_CURRENT_LIST_DIR}/hal/memory" CACHE PATH "HAL arena directory")
    
    # ESP-IDF reads this file again to register the firmware as one component.
    if(DEFINED COMPONENT_NAME AND "${COMPONENT_DIR}" STREQUAL "${CMAKE_CURRENT_LIST_DIR}")
        if(NOT IDF_VERSION_MAJOR EQUAL 6 OR NOT IDF_VERSION_MINOR EQUAL 1)
            message(FATAL_ERROR "esp32_framework requires ESP-IDF 6.1.x")
        endif()
    
        idf_component_register(
            SRCS
                "main/main.c"
                "bsp/esp32_devkit_v1/bsp.c"
                "app/button/app_button.c"
                "app/status_led/app_status_led.c"
                "app/acquisition/app_acquisition.c"
                "app/telemetry/app_telemetry.c"
                "app/gui/app_gui.c"
                "app/gui/render.c"
                "app/gui/bitmaps.c"
                "components/button/cmp_button.c"
                "components/led/cmp_led.c"
                "components/sht31d/cmp_sht31d.c"
                "components/ssd1306/cmp_ssd1306.c"
                "components/ssd1306/cmp_ssd1306_fonts.c"
                "components/telemetry/cmp_telemetry.c"
                "hal/ports/esp32-wroom-32/hal_esp32.c"
                "hal/ports/esp32-wroom-32/system/hal_system.c"
                "hal/ports/esp32-wroom-32/gpio/hal_gpio.c"
                "hal/ports/esp32-wroom-32/i2c/hal_i2c.c"
                "hal/ports/esp32-wroom-32/uart/hal_uart.c"
                "hal/ports/esp32-wroom-32/timer/hal_timer.c"
                "${HAL_ESP32_MEMORY_DIR}/hal_memory.c"
            INCLUDE_DIRS "app" "bsp" "components/button" "components/led" "components/sht31d" "components/ssd1306" "components/telemetry" "${HAL_ESP32_INTERFACE_DIR}"
            PRIV_INCLUDE_DIRS "hal/ports/esp32-wroom-32" "${HAL_ESP32_MEMORY_DIR}"
            PRIV_REQUIRES
                esp_driver_gpio esp_driver_i2c esp_driver_uart esp_driver_gptimer
                esp_hw_support esp_pm freertos esp_common
            REQUIRED_IDF_TARGETS esp32
        )
    
        target_compile_features(${COMPONENT_LIB} PUBLIC c_std_11)
        return()
    endif()
    
    set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}")
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(esp32_framework)
    [submodule "hal_interface"]
        path = hal/interface
        url = git@github.com:fmsantos8/hal_interface.git
    [submodule "hal/ports/esp32-wroom-32"]
        path = hal/ports/esp32-wroom-32
        url = git@github.com:fmsantos8/esp32-wroom-32.git
    [submodule "hal/memory"]
        path = hal/memory
        url = git@github.com:fmsantos8/memory.git
    [submodule "components/ssd1306"]
        path = components/ssd1306
        url = git@github.com:fmsantos8/ssd1306.git
    [submodule "components/sht31d"]
        path = components/sht31d
        url = git@github.com:fmsantos8/sht31d.git
    [submodule "components/button"]
        path = components/button
        url = git@github.com:fmsantos8/button.git
    [submodule "components/led"]
        path = components/led
        url = git@github.com:fmsantos8/led.git
    [submodule "components/telemetry"]
        path = components/telemetry
        url = git@github.com:fmsantos8/telemetry.git
    ✓ • 0ms

## Activity

    $ for f in bsp/bsp.h bsp/esp32_devkit_v1/bsp.c hal/ports/esp32-wroom-32/hal_esp32.c hal/ports/esp32-wroom-32/hal_esp32_private.h docs/pinout.md; do echo "FILE: $f"; nl -ba "$f"; done
    FILE: bsp/bsp.h
         1    /**
         2     * @file bsp.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Hardware resources provided by the selected board.
         5     */
         6    #ifndef BSP_H
         7    #define BSP_H
         8
         9    #include <stdbool.h>
        10    #include <stdint.h>
        11
        12    #include "common/hal_status.h"
        13    #include "gpio/hal_gpio.h"
        14    #include "i2c/hal_i2c.h"
        15    #include "timer/hal_timer.h"
        16    #include "uart/hal_uart.h"
        17
        18    #ifdef __cplusplus
        19    extern "C" {
        20    #endif
        21
        22    /**
        23     * @brief Constructed HAL resources and electrical characteristics of the board.
        24     *
        25     * @details Persistent handles belong to the HAL arena; composition lends them
        26     * to components. Sensor and display share devices_i2c with distinct addresses.
        27     */
        28    typedef struct {
        29        hal_gpio_t *button;          /** Button input GPIO. */
        30        hal_gpio_t *led;             /** Status LED output GPIO. */
        31        hal_i2c_t *devices_i2c;      /** Bus shared by the sensor and display. */
        32        hal_uart_t *telemetry_uart;  /** UART used for telemetry. */
        33        hal_timer_t *timebase_timer; /** Stopped timer configured with a 1 ms period. */
        34
        35        bool button_active_level; /** Electrical level representing a pressed button. */
        36        bool led_active_level;    /** Electrical level that illuminates the LED. */
        37
        38        uint8_t sht31_address;   /** Unshifted seven-bit SHT31-D address. */
        39        uint8_t ssd1306_address; /** Unshifted seven-bit SSD1306 address. */
        40    } bsp_resources_t;
        41
        42    /**
        43     * @brief Construct the selected board's hardware resources once.
        44     *
        45     * @details Call sequentially in normal context between hal_system_init() and
        46     * hal_system_seal(). Configures the LED off first and leaves the timer stopped;
        47     * initialize callback data before starting it. Invalid arguments allow retry.
        48     * Once construction starts, further attempts are rejected even after failure;
        49     * restart firmware to retry. Failure may consume arena space and reserve hardware.
        50     * resources needs no prior initialization and changes only on success.
        51     *
        52     * @param[in] timer_callback Required callback invoked in interrupt context for
        53     * each serviced timer expiration, with a nominal period of one millisecond.
        54     * @param[out] resources Receives all constructed resources on success.
        55     *
        56     * @retval HAL_STATUS_OK All resources were constructed.
        57     * @retval HAL_STATUS_INVALID_ARGUMENT timer_callback or resources is NULL.
        58     * @retval HAL_STATUS_INVALID_STATE Construction has already been attempted, or
        59     * HAL resource construction is closed.
        60     * @retval HAL_STATUS_NOT_INITIALIZED The system has not been initialized.
        61     * @retval HAL_STATUS_NOT_SUPPORTED A board configuration is unavailable.
        62     * @retval HAL_STATUS_RESOURCE_BUSY A required hardware resource is already claimed.
        63     * @retval HAL_STATUS_NO_MEMORY HAL or SDK object storage is unavailable.
        64     * @retval HAL_STATUS_ERROR A peripheral could not be configured.
        65     */
        66    hal_status_t bsp_init(hal_timer_callback_t timer_callback, bsp_resources_t *resources);
        67
        68    #ifdef __cplusplus
        69    }
        70    #endif
        71
        72    #endif
    FILE: bsp/esp32_devkit_v1/bsp.c
         1    #include "bsp.h"
         2
         3    #include <stddef.h>
         4
         5    /** @brief A construction attempt reserves resources until firmware restart. */
         6    static bool init_attempted;
         7
         8    static hal_status_t init_button(hal_gpio_t **button) {
         9        const hal_gpio_config_t config = {
        10            .pin = {.port = 0, .number = 18},
        11            .direction = HAL_GPIO_INPUT,
        12            .pull = HAL_GPIO_PULL_NONE, // External 10 kOhm pull-up.
        13        };
        14        return hal_gpio_config(button, &config);
        15    }
        16
        17    static hal_status_t init_led(hal_gpio_t **led, bool active_level) {
        18        const hal_gpio_config_t config = {
        19            .pin = {.port = 0, .number = 19},
        20            .direction = HAL_GPIO_OUTPUT,
        21            .pull = HAL_GPIO_PULL_NONE,
        22            .initial_level = !active_level,
        23        };
        24        // Establish the inactive level before enabling the GPIO output.
        25        return hal_gpio_config(led, &config);
        26    }
        27
        28    static hal_status_t init_i2c(hal_i2c_t **i2c) {
        29        const hal_i2c_config_t config = {
        30            .instance = 0,
        31            .clock_pin = {.port = 0, .number = 22},
        32            .data_pin = {.port = 0, .number = 21},
        33            .frequency = 100000,
        34            .timeout = 100,
        35        };
        36        return hal_i2c_config(i2c, &config);
        37    }
        38
        39    static hal_status_t init_uart(hal_uart_t **uart) {
        40        const hal_uart_config_t config = {
        41            .instance = 0,
        42            .transmit_pin = {.port = 0, .number = 1},
        43            .receive_pin = {.port = 0, .number = 3},
        44            .baudrate = 115200,
        45            .data_width = HAL_UART_DATA_WIDTH_8,
        46            .parity = HAL_UART_PARITY_NONE,
        47            .stop_count = HAL_UART_STOP_COUNT_1,
        48            .timeout = 1000,
        49        };
        50        return hal_uart_config(uart, &config);
        51    }
        52
        53    static hal_status_t init_timer(hal_timer_t **timer, hal_timer_callback_t callback) {
        54        const hal_timer_config_t config = {
        55            .instance = 0,
        56            .period = 1,
        57            .callback = callback,
        58        };
        59        return hal_timer_config(timer, &config);
        60    }
        61
        62    hal_status_t bsp_init(hal_timer_callback_t timer_callback, bsp_resources_t *resources) {
        63        if (timer_callback == NULL || resources == NULL) {
        64            return HAL_STATUS_INVALID_ARGUMENT;
        65        }
        66        if (init_attempted) {
        67            return HAL_STATUS_INVALID_STATE;
        68        }
        69        init_attempted = true;
        70
        71        bsp_resources_t configured_resources = {
        72            .button_active_level = false,
        73            .led_active_level = true,
        74            .sht31_address = 0x44,
        75            .ssd1306_address = 0x3C,
        76        };
        77
        78        hal_status_t status = init_led(&configured_resources.led, configured_resources.led_active_level);
        79        if (status == HAL_STATUS_OK) {
        80            status = init_button(&configured_resources.button);
        81        }
        82        if (status == HAL_STATUS_OK) {
        83            status = init_i2c(&configured_resources.devices_i2c);
        84        }
        85        if (status == HAL_STATUS_OK) {
        86            status = init_uart(&configured_resources.telemetry_uart);
        87        }
        88        if (status == HAL_STATUS_OK) {
        89            status = init_timer(&configured_resources.timebase_timer, timer_callback);
        90        }
        91        if (status == HAL_STATUS_OK) {
        92            // Publish the handles only after every resource has been constructed.
        93            *resources = configured_resources;
        94        }
        95        return status;
        96    }
    FILE: hal/ports/esp32-wroom-32/hal_esp32.c
         1    #include "hal_esp32_private.h"
         2
         3    #include <limits.h>
         4
         5    #include "driver/gpio.h"
         6    #include "esp_private/esp_gpio_reserve.h"
         7
         8    static uint64_t claimed_pins = 0;
         9
        10    hal_status_t hal_esp32_memory_status(hal_memory_status_t status) {
        11        switch (status) {
        12            case HAL_MEMORY_OK:
        13                return HAL_STATUS_OK;
        14            case HAL_MEMORY_INVALID_ARGUMENT:
        15                return HAL_STATUS_INVALID_ARGUMENT;
        16            case HAL_MEMORY_NOT_INITIALIZED:
        17                return HAL_STATUS_NOT_INITIALIZED;
        18            case HAL_MEMORY_INVALID_STATE:
        19                return HAL_STATUS_INVALID_STATE;
        20            case HAL_MEMORY_NO_MEMORY:
        21                return HAL_STATUS_NO_MEMORY;
        22            default:
        23                return HAL_STATUS_ERROR;
        24        }
        25    }
        26
        27    hal_status_t hal_esp32_convert_timeout_to_ticks(uint32_t timeout, TickType_t *ticks) {
        28        if (timeout == 0 || ticks == NULL) {
        29            return HAL_STATUS_INVALID_ARGUMENT;
        30        }
        31        // The I2C driver accepts int milliseconds and uses this 32-bit conversion.
        32        if (timeout > INT_MAX || timeout > UINT32_MAX / configTICK_RATE_HZ) {
        33            return HAL_STATUS_NOT_SUPPORTED;
        34        }
        35        TickType_t converted_ticks = pdMS_TO_TICKS(timeout);
        36        if (converted_ticks == 0 || converted_ticks == portMAX_DELAY) {
        37            return HAL_STATUS_NOT_SUPPORTED;
        38        }
        39        *ticks = converted_ticks;
        40        return HAL_STATUS_OK;
        41    }
        42
        43    hal_status_t hal_esp32_pin_validate(hal_pin_t pin) {
        44        if (pin.port != 0 || pin.number >= GPIO_NUM_MAX || pin.number == 20 || !GPIO_IS_VALID_GPIO(pin.number)) {
        45            return HAL_STATUS_INVALID_ARGUMENT;
        46        }
        47        // GPIO6..11 connect to module flash; GPIO37/38 are not exposed.
        48        if ((pin.number >= 6 && pin.number <= 11) || pin.number == 37 || pin.number == 38) {
        49            return HAL_STATUS_NOT_SUPPORTED;
        50        }
        51        return HAL_STATUS_OK;
        52    }
        53
        54    hal_status_t hal_esp32_pin_claim(hal_pin_t pin) {
        55        hal_status_t status = hal_esp32_pin_validate(pin);
        56        if (status != HAL_STATUS_OK) {
        57            return status;
        58        }
        59        hal_memory_info_t info;
        60        status = hal_esp32_memory_status(hal_memory_get_info(&info));
        61        if (status != HAL_STATUS_OK) {
        62            return status;
        63        }
        64        if (info.sealed) {
        65            return HAL_STATUS_INVALID_STATE;
        66        }
        67
        68        uint64_t mask = UINT64_C(1) << pin.number;
        69        if ((claimed_pins & mask) != 0 || esp_gpio_is_reserved(mask)) {
        70            return HAL_STATUS_RESOURCE_BUSY;
        71        }
        72        claimed_pins |= mask;
        73        return HAL_STATUS_OK;
        74    }
        75
        76    void hal_esp32_pin_release(hal_pin_t pin) {
        77        if (hal_esp32_pin_validate(pin) == HAL_STATUS_OK) {
        78            claimed_pins &= ~(UINT64_C(1) << pin.number);
        79        }
        80    }
    FILE: hal/ports/esp32-wroom-32/hal_esp32_private.h
         1    /**
         2     * @file hal_esp32_private.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Internal services shared by this port's peripheral implementations.
         5     */
         6    #ifndef HAL_ESP32_PRIVATE_H
         7    #define HAL_ESP32_PRIVATE_H
         8
         9    #include <stdint.h>
        10
        11    #include "common/hal_pin.h"
        12    #include "common/hal_status.h"
        13
        14    #include "hal_memory.h"
        15
        16    #include "freertos/FreeRTOS.h"
        17
        18    /**
        19     * @brief Translate the memory module's independent status into the HAL contract.
        20     *
        21     * @param[in] status The memory module's status.
        22     *
        23     * @return hal_status_t The corresponding HAL status.
        24     */
        25    hal_status_t hal_esp32_memory_status(hal_memory_status_t status);
        26
        27    /**
        28     * @brief Validate a finite timeout in milliseconds and convert it to FreeRTOS ticks.
        29     *
        30     * @details Uses pdMS_TO_TICKS(), which rounds fractional ticks down.
        31     * Rejects values greater than INT_MAX for compatibility with the I2C driver,
        32     * values that overflow the conversion, and results equal to zero or
        33     * portMAX_DELAY. The conversion depends on configTICK_RATE_HZ.
        34     *
        35     * @param[in] timeout Timeout in milliseconds. Must be greater than zero.
        36     * @param[out] ticks Converted timeout. Modified only on success.
        37     *
        38     * @retval HAL_STATUS_OK Timeout validated and converted.
        39     * @retval HAL_STATUS_INVALID_ARGUMENT timeout is zero or ticks is NULL.
        40     * @retval HAL_STATUS_NOT_SUPPORTED Timeout exceeds the supported limits or
        41     * cannot produce a positive, finite tick count.
        42     */
        43    hal_status_t hal_esp32_convert_timeout_to_ticks(uint32_t timeout, TickType_t *ticks);
        44
        45    /**
        46     * @brief Validate bank zero, physical GPIO number and WROOM-32 module availability.
        47     *
        48     * @param[in] pin The pin to validate.
        49     *
        50     * @return hal_status_t HAL_STATUS_OK if the pin is valid, or an appropriate error code otherwise.
        51     */
        52    hal_status_t hal_esp32_pin_validate(hal_pin_t pin);
        53
        54    /**
        55     * @brief Claim a pin for a HAL resource after checking the arena and SDK reservations.
        56     * Peripheral-specific capabilities must also be checked before claiming.
        57     * Construction and SDK pin configuration must be sequential.
        58     *
        59     * @param[in] pin The pin to claim.
        60     *
        61     * @return hal_status_t HAL_STATUS_OK if the pin was successfully claimed, or an appropriate error code otherwise.
        62     */
        63    hal_status_t hal_esp32_pin_claim(hal_pin_t pin);
        64
        65    /**
        66     * @brief Release a previously claimed pin.
        67     *
        68     * @param[in] pin The pin to release.
        69     */
        70    void hal_esp32_pin_release(hal_pin_t pin);
        71
        72    #endif
    FILE: docs/pinout.md
         1    # Pinout — ESP32 DevKit V1
         2
         3    | Função | Pino do ESP32 | Ligação externa |
         4    | --- | --- | --- |
         5    | Alimentação | `3V3` | VCC do SHT31-D e do display SSD1306; resistor de pull-up do botão |
         6    | Terra | `GND` | GND dos módulos, do botão, do LED e do adaptador serial externo, se usado |
         7    | I²C0 SDA | `GPIO21` | SDA do SHT31-D e SDA do SSD1306 |
         8    | I²C0 SCL | `GPIO22` | SCL do SHT31-D e SCL do SSD1306 |
         9    | UART0 TX | `GPIO1` | RX do adaptador serial externo |
        10    | UART0 RX | `GPIO3` | TX do adaptador serial externo |
        11    | Botão | `GPIO18` | Botão normalmente aberto para GND; resistor externo de `10 kΩ` entre `GPIO18` e `3V3` |
        12    | LED | `GPIO19` | Resistor externo de `1 kΩ` em série com o ânodo do LED; cátodo para GND |
        13
        14    ## Barramento I²C
        15
        16    O SHT31-D e o display SSD1306 de `128 × 64` pixels compartilham `GPIO21` (SDA) e `GPIO22` (SCL) no barramento I²C0 a `100 kHz`. Os endereços I²C de 7 bits são `0x44` para o SHT31-D e `0x3C` para o SSD1306.
        17
        18    ## Botão e LED
        19
        20    O botão é ativo em nível baixo: solto, o resistor de `10 kΩ` mantém `GPIO18` em `3,3 V`; pressionado, o botão liga o pino ao GND. O LED é ativo em nível alto: `GPIO19` em nível alto acende o LED através do resistor de `1 kΩ`.
        21
        22    ## Comunicação serial
        23
        24    A UART0 usa `115200 8N1`. Para monitoramento pela conexão USB da DevKit V1, utilize a interface serial da própria placa.
    ✓ • 16ms

## Activity

    $ for f in app/*/*.c app/*/*.h; do echo "FILE: $f"; nl -ba "$f"; done
    FILE: app/acquisition/app_acquisition.c
         1    /**
         2     * @file app_acquisition.c
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Acquires periodic sensor measurements and preserves the last valid sample.
         5     */
         6    #include "app_acquisition.h"
         7    #include "status_led/app_status_led.h"
         8    #include "telemetry/app_telemetry.h"
         9
        10    #include <stddef.h>
        11
        12    #define APP_ACQUISITION_INTERVAL 2000U /** Minimum interval between attempts, in milliseconds. */
        13
        14    _Static_assert(APP_ACQUISITION_INTERVAL >= SHT31D_MEASUREMENT_TIME, "Retry interval must cover sensor conversion time");
        15
        16    typedef enum {
        17        APP_ACQUISITION_STARTING = 0, /** Send one measurement command. */
        18        APP_ACQUISITION_WAITING,      /** Wait for conversion before reading. */
        19        APP_ACQUISITION_INTERVAL_WAIT /** Wait before another attempt. */
        20    } app_acquisition_phase_t;
        21
        22    static uint32_t (*get_ticks)(void) = NULL;                       /** Time source supplied by the integration. */
        23    static app_acquisition_phase_t phase = APP_ACQUISITION_STARTING; /** Current task phase. */
        24    static uint32_t last_attempt = 0;                                /** Time after the latest measurement start returned. */
        25    static bool result_available = false;                            /** True after a completed attempt. */
        26    static bool initialized = false;                                 /** True after successful task initialization. */
        27    static acquisition_t last_result;                                /** Retained measurement and latest attempt status. */
        28
        29    static void publish_acquisition(sht31d_status_t status) {
        30        last_result.status = status;
        31        if (status == SHT31D_STATUS_COMMUNICATION_ERROR || status == SHT31D_STATUS_TIMEOUT) {
        32            last_result.i2c_error = true;
        33        } else if (status == SHT31D_STATUS_OK || status == SHT31D_STATUS_CRC_ERROR) {
        34            last_result.i2c_error = false;
        35        }
        36        result_available = true;
        37        app_status_led_set_i2c_error(APP_STATUS_LED_I2C_SENSOR, last_result.i2c_error);
        38        if (status == SHT31D_STATUS_OK) {
        39            const telemetry_sample_t sample = {
        40                .temperature = last_result.sample.temperature,
        41                .humidity = last_result.sample.humidity,
        42            };
        43            app_telemetry_report_sample(&sample);
        44        } else {
        45            app_telemetry_report_error(APP_TELEMETRY_SOURCE_SENSOR, (int32_t)status);
        46        }
        47    }
        48
        49    bool app_acquisition_init(uint32_t (*get_ticks_function)(void)) {
        50        if (get_ticks_function == NULL ||
        51            (initialized && phase == APP_ACQUISITION_WAITING)) {
        52            return false;
        53        }
        54        if (initialized && phase == APP_ACQUISITION_INTERVAL_WAIT &&
        55            (uint32_t)(get_ticks() - last_attempt) < SHT31D_MEASUREMENT_TIME) {
        56            return false;
        57        }
        58
        59        get_ticks = get_ticks_function;
        60        phase = APP_ACQUISITION_STARTING;
        61        last_attempt = 0;
        62        result_available = false;
        63        last_result = (acquisition_t){0};
        64        initialized = true;
        65        return true;
        66    }
        67
        68    void app_acquisition_run(sht31d_t *sensor) {
        69        if (sensor == NULL || !initialized) {
        70            return;
        71        }
        72
        73        if (phase == APP_ACQUISITION_INTERVAL_WAIT) {
        74            if ((uint32_t)(get_ticks() - last_attempt) < APP_ACQUISITION_INTERVAL) {
        75                return;
        76            }
        77            phase = APP_ACQUISITION_STARTING;
        78        }
        79
        80        if (phase == APP_ACQUISITION_STARTING) {
        81            const sht31d_status_t status = cmp_sht31d_start_measurement(sensor);
        82            last_attempt = get_ticks();
        83            if (status == SHT31D_STATUS_OK) {
        84                phase = APP_ACQUISITION_WAITING;
        85            } else {
        86                phase = APP_ACQUISITION_INTERVAL_WAIT;
        87                publish_acquisition(status);
        88            }
        89            return;
        90        }
        91
        92        if (phase == APP_ACQUISITION_WAITING) {
        93            if ((uint32_t)(get_ticks() - last_attempt) < SHT31D_MEASUREMENT_TIME) {
        94                return;
        95            }
        96
        97            sht31d_sample_t sample;
        98            const sht31d_status_t status = cmp_sht31d_read_measurement(sensor, &sample);
        99            phase = APP_ACQUISITION_INTERVAL_WAIT;
       100            if (status == SHT31D_STATUS_OK) {
       101                last_result.sample = sample;
       102                last_result.acquired_at = get_ticks();
       103                last_result.has_sample = true;
       104            }
       105            publish_acquisition(status);
       106        }
       107    }
       108
       109    bool app_acquisition_get_result(acquisition_t *result) {
       110        if (result == NULL || !result_available) {
       111            return false;
       112        }
       113        *result = last_result;
       114        return true;
       115    }
    FILE: app/button/app_button.c
         1    /**
         2     * @file app_button.c
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Detects valid button presses.
         5     */
         6    #include "app_button.h"
         7    #include "gui/app_gui.h"
         8
         9    #include <stddef.h>
        10
        11    static bool previous_pressed = false; /** Last valid button state. */
        12    static bool initialized = false;      /** True after task initialization. */
        13
        14    void app_button_init(void) {
        15        previous_pressed = false;
        16        initialized = true;
        17    }
        18
        19    void app_button_run(button_t *button) {
        20        if (!initialized || button == NULL) {
        21            return;
        22        }
        23
        24        bool pressed = false;
        25        // Keep the last valid edge state during a read failure.
        26        if (cmp_button_read(button, &pressed) != BUTTON_STATUS_OK) {
        27            return;
        28        }
        29
        30        const bool press_event = pressed && !previous_pressed;
        31        previous_pressed = pressed;
        32        if (press_event) {
        33            app_gui_toggle_screen();
        34        }
        35    }
    FILE: app/gui/app_gui.c
         1    /**
         2     * @file app_gui.c
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Coordinates screen selection, rendering, and display transfers.
         5     */
         6    #include "app_gui.h"
         7
         8    #include <stddef.h>
         9
        10    #include "acquisition/app_acquisition.h"
        11    #include "render.h"
        12    #include "status_led/app_status_led.h"
        13    #include "telemetry/app_telemetry.h"
        14
        15    #define RETRY_INTERVAL 1000U /** Delay after a failed display operation, in milliseconds. */
        16
        17    static uint32_t (*get_ticks)(void) = NULL;    /** Time source supplied by the integration. */
        18    static gui_screen_t screen = SCREEN_READINGS; /** Requested screen. */
        19    static gui_frame_t last_frame;                /** Snapshot currently drawn in the framebuffer. */
        20    static uint64_t uptime = 0;                   /** Accumulated time since startup, in milliseconds. */
        21    static uint32_t last_ticks = 0;               /** Last observed clock value. */
        22    static uint32_t retry_since = 0;              /** Time after the latest failed operation returned. */
        23    static bool pending = false;                  /** Pages remain in the current frame. */
        24    static bool rendered = false;                 /** A complete snapshot has been drawn in RAM. */
        25    static bool waiting_retry = false;            /** The next display attempt is delayed. */
        26    static bool initialized = false;              /** True after successful task initialization. */
        27
        28    /**
        29     * @brief Check whether the requested snapshot differs from the rendered one.
        30     *
        31     * @param[in] frame Requested snapshot.
        32     *
        33     * @retval true Content must be redrawn.
        34     * @retval false The current framebuffer already represents the snapshot.
        35     */
        36    static bool frame_changed(const gui_frame_t *frame) {
        37        if (!rendered || last_frame.screen != frame->screen || last_frame.has_result != frame->has_result) {
        38            return true;
        39        }
        40        if (frame->has_result &&
        41            (last_frame.acquisition.status != frame->acquisition.status ||
        42             last_frame.acquisition.has_sample != frame->acquisition.has_sample ||
        43             (frame->acquisition.has_sample &&
        44              (last_frame.acquisition.sample.temperature != frame->acquisition.sample.temperature ||
        45               last_frame.acquisition.sample.humidity != frame->acquisition.sample.humidity)))) {
        46            return true;
        47        }
        48        return frame->screen == SCREEN_DIAGNOSTICS && last_frame.uptime_seconds != frame->uptime_seconds;
        49    }
        50
        51    bool app_gui_init(uint32_t (*get_ticks_function)(void)) {
        52        if (get_ticks_function == NULL) {
        53            return false;
        54        }
        55        get_ticks = get_ticks_function;
        56        last_ticks = get_ticks();
        57        uptime = last_ticks;
        58        screen = SCREEN_READINGS;
        59        last_frame = (gui_frame_t){0};
        60        retry_since = 0;
        61        pending = false;
        62        rendered = false;
        63        waiting_retry = false;
        64        initialized = true;
        65        return true;
        66    }
        67
        68    void app_gui_toggle_screen(void) {
        69        if (initialized) {
        70            screen = screen == SCREEN_READINGS ? SCREEN_DIAGNOSTICS : SCREEN_READINGS;
        71        }
        72    }
        73
        74    void app_gui_run(ssd1306_t *display) {
        75        if (display == NULL || !initialized) {
        76            return;
        77        }
        78
        79        const uint32_t now = get_ticks();
        80        uptime += (uint32_t)(now - last_ticks);
        81        last_ticks = now;
        82        if (waiting_retry && (uint32_t)(now - retry_since) < RETRY_INTERVAL) {
        83            return;
        84        }
        85
        86        ssd1306_status_t status = SSD1306_STATUS_OK;
        87        if (!pending) {
        88            gui_frame_t frame = {.screen = screen, .uptime_seconds = uptime / 1000U};
        89            frame.has_result = app_acquisition_get_result(&frame.acquisition);
        90            if (!frame_changed(&frame)) {
        91                return;
        92            }
        93            status = render_frame(display, &frame);
        94            if (status == SSD1306_STATUS_OK) {
        95                last_frame = frame;
        96                rendered = true;
        97                pending = true;
        98            } else {
        99                rendered = false;
       100            }
       101        }
       102        if (status == SSD1306_STATUS_OK) {
       103            status = cmp_ssd1306_update(display, &pending);
       104        }
       105        waiting_retry = status != SSD1306_STATUS_OK;
       106        if (waiting_retry) {
       107            retry_since = get_ticks();
       108            app_telemetry_report_error(APP_TELEMETRY_SOURCE_DISPLAY, (int32_t)status);
       109            if (status == SSD1306_STATUS_TIMEOUT || status == SSD1306_STATUS_COMMUNICATION_ERROR) {
       110                app_status_led_set_i2c_error(APP_STATUS_LED_I2C_DISPLAY, true);
       111            }
       112        } else {
       113            app_status_led_set_i2c_error(APP_STATUS_LED_I2C_DISPLAY, false);
       114        }
       115    }
    FILE: app/gui/bitmaps.c
         1    /**
         2     * @file bitmaps.c
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Temperature and humidity bitmap data for the GUI.
         5     */
         6    #include "bitmaps.h"
         7
         8    /** @brief Nine-pixel thermometer, one two-byte row per scanline. */
         9    static const uint8_t thermometer_data[][2] = {
        10        {0x1C, 0x00},
        11        {0x22, 0x00},
        12        {0x22, 0x00},
        13        {0x22, 0x00},
        14        {0x22, 0x00},
        15        {0x2A, 0x00},
        16        {0x2A, 0x00},
        17        {0x2A, 0x00},
        18        {0x2A, 0x00},
        19        {0x2A, 0x00},
        20        {0x3E, 0x00},
        21        {0x6B, 0x00},
        22        {0x5D, 0x00},
        23        {0xBE, 0x80},
        24        {0xBE, 0x80},
        25        {0xBE, 0x80},
        26        {0x5D, 0x00},
        27        {0x63, 0x00},
        28        {0x1C, 0x00},
        29    };
        30
        31    /** @brief Nine-pixel humidity droplet, one two-byte row per scanline. */
        32    static const uint8_t droplet_data[][2] = {
        33        {0x08, 0x00},
        34        {0x14, 0x00},
        35        {0x14, 0x00},
        36        {0x22, 0x00},
        37        {0x22, 0x00},
        38        {0x41, 0x00},
        39        {0x41, 0x00},
        40        {0x80, 0x80},
        41        {0xA0, 0x80},
        42        {0xA0, 0x80},
        43        {0xA0, 0x80},
        44        {0x41, 0x00},
        45        {0x3E, 0x00},
        46    };
        47
        48    /** @brief Temperature icon backed by immutable pixel data. */
        49    static const ssd1306_bitmap_t thermometer_bitmap = {
        50        .data = (const uint8_t *)&thermometer_data,
        51        .size = sizeof(thermometer_data),
        52        .width = 9,
        53        .height = 19,
        54    };
        55
        56    /** @brief Humidity icon backed by immutable pixel data. */
        57    static const ssd1306_bitmap_t droplet_bitmap = {
        58        .data = (const uint8_t *)&droplet_data,
        59        .size = sizeof(droplet_data),
        60        .width = 9,
        61        .height = 13,
        62    };
        63
        64    const ssd1306_bitmap_t *get_thermometer_bitmap(void) {
        65        return &thermometer_bitmap;
        66    }
        67
        68    const ssd1306_bitmap_t *get_droplet_bitmap(void) {
        69        return &droplet_bitmap;
        70    }
    FILE: app/gui/render.c
         1    /**
         2     * @file render.c
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Draws reading cards and acquisition diagnostics in the framebuffer.
         5     */
         6    #include "render.h"
         7    #include "bitmaps.h"
         8
         9    #include <stdio.h>
        10
        11    #define DRAW_TRY(operation)                    \
        12        do {                                       \
        13            ssd1306_status_t result = (operation); \
        14            if (result != SSD1306_STATUS_OK) {     \
        15                return result;                     \
        16            }                                      \
        17        } while (0)
        18
        19    static ssd1306_status_t draw_right_text(ssd1306_t *display, uint32_t left, uint32_t right, uint32_t y, const char *text, ssd1306_font_t font) {
        20        uint32_t width = 0;
        21        uint32_t height = 0;
        22        DRAW_TRY(cmp_ssd1306_measure_text(text, font, &width, &height));
        23        if (width > right - left) {
        24            return SSD1306_STATUS_INVALID_ARGUMENT;
        25        }
        26        return cmp_ssd1306_draw_text(display, right - width, y, text, font);
        27    }
        28
        29    static ssd1306_status_t draw_header(ssd1306_t *display, const gui_frame_t *frame, const char *title) {
        30        DRAW_TRY(cmp_ssd1306_draw_text(display, 6, 3, title, SSD1306_FONT_SMALL));
        31        if (frame->has_result && frame->acquisition.status != SHT31D_STATUS_OK) {
        32            DRAW_TRY(cmp_ssd1306_draw_text(display, 92, 3, "!", SSD1306_FONT_SMALL));
        33        }
        34        for (uint32_t page = 0; page < 2; ++page) {
        35            uint32_t x = 108U + 11U * page;
        36            if ((uint32_t)frame->screen == page) {
        37                DRAW_TRY(cmp_ssd1306_fill_rect(display, x, 4, 7, 3, true));
        38            } else {
        39                DRAW_TRY(cmp_ssd1306_fill_rect(display, x + 2, 5, 3, 1, true));
        40            }
        41        }
        42        return SSD1306_STATUS_OK;
        43    }
        44
        45    static ssd1306_status_t draw_readings(ssd1306_t *display, const gui_frame_t *frame) {
        46        char temperature[12] = "--,-";
        47        char humidity[12] = "--,-";
        48        if (frame->acquisition.has_sample) {
        49            if (frame->acquisition.sample.temperature < -45000 || frame->acquisition.sample.temperature > 130000 ||
        50                frame->acquisition.sample.humidity > 100000) {
        51                return SSD1306_STATUS_INVALID_ARGUMENT;
        52            }
        53            // Round to tenths for presentation; keep the sample at its original resolution.
        54            int64_t value = frame->acquisition.sample.temperature;
        55            uint32_t magnitude = (uint32_t)(((value < 0 ? -value : value) + 50) / 100);
        56            uint32_t rh = (frame->acquisition.sample.humidity + 50U) / 100U;
        57            snprintf(temperature, sizeof(temperature), "%s%lu,%lu", value < 0 && magnitude != 0 ? "-" : "", (uint32_t)(magnitude / 10U), (uint32_t)(magnitude % 10U));
        58            snprintf(humidity, sizeof(humidity), "%lu,%lu", (uint32_t)(rh / 10U), (uint32_t)(rh % 10U));
        59        }
        60        DRAW_TRY(draw_header(display, frame, "TEMPERATURA"));
        61        DRAW_TRY(cmp_ssd1306_draw_bitmap(display, 6, 22, get_thermometer_bitmap()));
        62        DRAW_TRY(draw_right_text(display, 20, 103, 14, temperature, SSD1306_FONT_LARGE));
        63        DRAW_TRY(cmp_ssd1306_draw_text(display, 108, 24, "°C", SSD1306_FONT_MEDIUM));
        64        DRAW_TRY(cmp_ssd1306_draw_line(display, 6, 46, 121, 46, true));
        65        DRAW_TRY(cmp_ssd1306_draw_bitmap(display, 6, 50, get_droplet_bitmap()));
        66        DRAW_TRY(cmp_ssd1306_draw_text(display, 21, 53, "UMIDADE", SSD1306_FONT_SMALL));
        67        DRAW_TRY(draw_right_text(display, 65, 111, 48, humidity, SSD1306_FONT_MEDIUM));
        68        return cmp_ssd1306_draw_text(display, 116, 48, "%", SSD1306_FONT_MEDIUM);
        69    }
        70
        71    static const char *acquisition_label(const gui_frame_t *frame) {
        72        if (!frame->has_result) {
        73            return "AGUARDANDO";
        74        }
        75        switch (frame->acquisition.status) {
        76            case SHT31D_STATUS_OK:
        77                return "OK";
        78            case SHT31D_STATUS_TIMEOUT:
        79                return "TIMEOUT";
        80            case SHT31D_STATUS_CRC_ERROR:
        81                return "ERRO CRC";
        82            case SHT31D_STATUS_COMMUNICATION_ERROR:
        83                return "ERRO I2C";
        84            default:
        85                return "INDISPONIVEL";
        86        }
        87    }
        88
        89    static ssd1306_status_t draw_diagnostics(ssd1306_t *display, const gui_frame_t *frame) {
        90        uint64_t seconds = frame->uptime_seconds;
        91        uint64_t days = seconds / 86400U;
        92        char uptime_text[24];
        93        if (days == 0) {
        94            snprintf(uptime_text, sizeof(uptime_text), "%02lu:%02lu:%02lu", (uint32_t)(seconds / 3600U), (uint32_t)(seconds / 60U % 60U), (uint32_t)(seconds % 60U));
        95        } else {
        96            uint32_t shown_days = days > 9999999U ? 9999999U : (uint32_t)days;
        97            snprintf(uptime_text, sizeof(uptime_text), "%lu%s %02lu:%02lu:%02lu", (uint32_t)shown_days, days > 9999999U ? "+" : "d", (uint32_t)(seconds / 3600U % 24U), (uint32_t)(seconds / 60U % 60U), (uint32_t)(seconds % 60U));
        98        }
        99        DRAW_TRY(draw_header(display, frame, "DIAGNOSTICO"));
       100        DRAW_TRY(cmp_ssd1306_draw_text(display, 6, 18, "ULTIMA AQUISICAO", SSD1306_FONT_SMALL));
       101        DRAW_TRY(cmp_ssd1306_draw_text(display, 6, 28, acquisition_label(frame), SSD1306_FONT_SMALL));
       102        DRAW_TRY(cmp_ssd1306_draw_line(display, 6, 40, 121, 40, true));
       103        DRAW_TRY(cmp_ssd1306_draw_text(display, 6, 45, "TEMPO ATIVO", SSD1306_FONT_SMALL));
       104        return cmp_ssd1306_draw_text(display, 6, 55, uptime_text, SSD1306_FONT_SMALL);
       105    }
       106
       107    ssd1306_status_t render_frame(ssd1306_t *display, const gui_frame_t *frame) {
       108        DRAW_TRY(cmp_ssd1306_clear(display));
       109        return frame->screen == SCREEN_READINGS ? draw_readings(display, frame)
       110                                                : draw_diagnostics(display, frame);
       111    }
    FILE: app/status_led/app_status_led.c
         1    /**
         2     * @file app_status_led.c
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Indicates startup, normal operation, and I2C communication errors.
         5     */
         6    #include "app_status_led.h"
         7
         8    #include <stddef.h>
         9
        10    #define APP_STATUS_LED_BLINK_INTERVAL 500U /** Time between blink transitions, in milliseconds. */
        11
        12    static uint32_t (*get_ticks)(void) = NULL; /** Time source supplied by the integration. */
        13    static uint8_t i2c_errors = 0;             /** Independent sensor and display error bits. */
        14    static bool operation_ready = false;       /** True after startup is complete. */
        15    static uint32_t last_transition = 0;       /** Timestamp of the current blink phase. */
        16    static int8_t led_state = -1;              /** Last requested led state; -1 before the first command. */
        17    static bool blink_active = false;          /** True after starting the current blink sequence. */
        18    static bool initialized = false;           /** True after successful task initialization. */
        19
        20    bool app_status_led_init(uint32_t (*get_ticks_function)(void)) {
        21        if (get_ticks_function == NULL) {
        22            return false;
        23        }
        24
        25        get_ticks = get_ticks_function;
        26        last_transition = 0;
        27        i2c_errors = 0;
        28        operation_ready = false;
        29        led_state = -1;
        30        blink_active = false;
        31        initialized = true;
        32        return true;
        33    }
        34
        35    void app_status_led_set_ready(bool ready) {
        36        if (initialized) {
        37            operation_ready = ready;
        38        }
        39    }
        40
        41    void app_status_led_set_i2c_error(app_status_led_i2c_source_t source, bool active) {
        42        if (!initialized || (source != APP_STATUS_LED_I2C_SENSOR && source != APP_STATUS_LED_I2C_DISPLAY)) {
        43            return;
        44        }
        45
        46        const uint8_t bit = (uint8_t)(1U << (uint8_t)source);
        47        if (active) {
        48            i2c_errors |= bit;
        49        } else {
        50            i2c_errors &= (uint8_t)~bit;
        51        }
        52    }
        53
        54    void app_status_led_run(const led_t *led) {
        55        if (led == NULL || !initialized) {
        56            return;
        57        }
        58
        59        bool state_to_set = operation_ready;
        60
        61        if (i2c_errors != 0) {
        62            const uint32_t now = get_ticks();
        63            state_to_set = led_state == 1;
        64
        65            if (!blink_active) {
        66                blink_active = true;
        67                state_to_set = false;
        68                last_transition = now;
        69            } else if ((uint32_t)(now - last_transition) >= APP_STATUS_LED_BLINK_INTERVAL) {
        70                state_to_set = !state_to_set;
        71                last_transition = now;
        72            }
        73        } else {
        74            blink_active = false;
        75        }
        76
        77        if (state_to_set != led_state) {
        78            cmp_led_set(led, state_to_set);
        79            led_state = state_to_set;
        80        }
        81    }
    FILE: app/telemetry/app_telemetry.c
         1    /**
         2     * @file app_telemetry.c
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Buffers application records and handles UART retry and discard policies.
         5     */
         6    #include "app_telemetry.h"
         7    #include "app_telemetry_private.h"
         8
         9    #include <stddef.h>
        10
        11    static uint32_t (*get_ticks)(void) = NULL;     /** Time source supplied by the integration. */
        12    static record_t records[QUEUE_CAPACITY] = {0}; /** Records queue */
        13    static uint32_t head = 0;                      /** Oldest queued record. */
        14    static uint32_t count = 0;                     /** Number of queued records. */
        15    static uint32_t retry_since = 0;               /** Time after the latest failed send returned. */
        16    static bool waiting_retry = false;             /** The next UART attempt is delayed. */
        17    static bool initialized = false;               /** True after successful task initialization. */
        18
        19    static bool enqueue(const record_t *record) {
        20        if (!initialized) {
        21            return false;
        22        }
        23        if (count == QUEUE_CAPACITY) {
        24            return false;
        25        }
        26        records[(head + count) % QUEUE_CAPACITY] = *record;
        27        ++count;
        28        return true;
        29    }
        30
        31    static void remove_head(void) {
        32        head = (head + 1U) % QUEUE_CAPACITY;
        33        --count;
        34    }
        35
        36    bool app_telemetry_init(uint32_t (*get_ticks_function)(void)) {
        37        if (get_ticks_function == NULL) {
        38            return false;
        39        }
        40        get_ticks = get_ticks_function;
        41        head = 0;
        42        count = 0;
        43        retry_since = 0;
        44        waiting_retry = false;
        45        initialized = true;
        46        return true;
        47    }
        48
        49    bool app_telemetry_report_sample(const telemetry_sample_t *sample) {
        50        if (sample == NULL) {
        51            return false;
        52        }
        53        const record_t record = {.kind = RECORD_SAMPLE, .sample = *sample};
        54        return enqueue(&record);
        55    }
        56
        57    bool app_telemetry_report_error(telemetry_source_t source, int32_t code) {
        58        if (source != APP_TELEMETRY_SOURCE_SENSOR && source != APP_TELEMETRY_SOURCE_DISPLAY) {
        59            return false;
        60        }
        61        const record_t record = {.kind = RECORD_ERROR, .source = source, .code = code};
        62        return enqueue(&record);
        63    }
        64
        65    void app_telemetry_run(const telemetry_t *telemetry) {
        66        if (telemetry == NULL || !initialized || count == 0) {
        67            return;
        68        }
        69        if (waiting_retry && (uint32_t)(get_ticks() - retry_since) < RETRY_INTERVAL) {
        70            return;
        71        }
        72
        73        const record_t *record = &records[head];
        74        uint32_t transmitted = 0;
        75        telemetry_status_t status;
        76        if (record->kind == RECORD_SAMPLE) {
        77            status = cmp_telemetry_send_sample(telemetry, &record->sample, &transmitted);
        78        } else {
        79            const char *source = record->source == APP_TELEMETRY_SOURCE_SENSOR ? "SHT31D" : "SSD1306";
        80            status = cmp_telemetry_send_error(telemetry, source, record->code, &transmitted);
        81        }
        82        if (status == TELEMETRY_STATUS_OK) {
        83            remove_head();
        84            waiting_retry = false;
        85            return;
        86        }
        87
        88        // Only retry transient failures that accepted no prefix of the record.
        89        if (transmitted != 0 || (status != TELEMETRY_STATUS_TIMEOUT && status != TELEMETRY_STATUS_COMMUNICATION_ERROR)) {
        90            remove_head();
        91        }
        92        retry_since = get_ticks();
        93        waiting_retry = true;
        94    }
    FILE: app/acquisition/app_acquisition.h
         1    /**
         2     * @file app_acquisition.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Acquires periodic sensor measurements and preserves the last valid sample.
         5     */
         6    #ifndef APP_ACQUISITION_H
         7    #define APP_ACQUISITION_H
         8
         9    #include "cmp_sht31d.h"
        10
        11    #include <stdbool.h>
        12    #include <stdint.h>
        13
        14    #ifdef __cplusplus
        15    extern "C" {
        16    #endif
        17
        18    /**
        19     * @brief Latest acquisition result and retained valid measurement.
        20     *
        21     */
        22    typedef struct {
        23        sht31d_status_t status; /** Result of the latest completed attempt. */
        24        sht31d_sample_t sample; /** Last valid measurement; meaningful when has_sample is true. */
        25        uint32_t acquired_at;   /** Last valid read completion time in milliseconds, modulo 2^32. */
        26        bool has_sample;        /** True after the first successful measurement. */
        27        bool i2c_error;         /** Communication error or timeout, retained until a complete read. */
        28    } acquisition_t;
        29
        30    /**
        31     * @brief Initialize the single acquisition task without accessing the sensor.
        32     *
        33     * @param[in] get_ticks_function Required function to read the current time (ms)
        34     *
        35     * @retval true Task initialized successfully.
        36     * @retval false get_ticks_function is NULL, a measurement is pending, or its conversion wait is unfinished; previous state is preserved.
        37     */
        38    bool app_acquisition_init(uint32_t (*get_ticks_function)(void));
        39
        40    /**
        41     * @brief Start or read a measurement when the current phase is due.
        42     *
        43     * @param[in,out] sensor Initialized sensor component; NULL and calls before task initialization are ignored.
        44     */
        45    void app_acquisition_run(sht31d_t *sensor);
        46
        47    /**
        48     * @brief Copy the latest completed result without accessing the sensor.
        49     *
        50     * @param[out] result Last successful measurement
        51     *
        52     * @retval true Result copied successfully.
        53     * @retval false result is NULL or no attempt has completed since initialization.
        54     */
        55    bool app_acquisition_get_result(acquisition_t *result);
        56
        57    #ifdef __cplusplus
        58    }
        59    #endif
        60
        61    #endif
    FILE: app/button/app_button.h
         1    /**
         2     * @file app_button.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Detects valid button presses.
         5     */
         6    #ifndef APP_BUTTON_H
         7    #define APP_BUTTON_H
         8
         9    #ifdef __cplusplus
        10    extern "C" {
        11    #endif
        12
        13    #include "cmp_button.h"
        14
        15    /**
        16     * @brief Reset button press detection to the released state.
        17     */
        18    void app_button_init(void);
        19
        20    /**
        21     * @brief Read the button state and detect a valid press.
        22     *
        23     * @param[in,out] button Initialized button component; use the same instance between calls.
        24     */
        25    void app_button_run(button_t *button);
        26
        27    #ifdef __cplusplus
        28    }
        29    #endif
        30
        31    #endif
    FILE: app/gui/app_gui.h
         1    /**
         2     * @file app_gui.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Presents sensor readings and acquisition diagnostics on the OLED.
         5     */
         6    #ifndef APP_GUI_H
         7    #define APP_GUI_H
         8
         9    #include "cmp_ssd1306.h"
        10
        11    #include <stdbool.h>
        12    #include <stdint.h>
        13
        14    #ifdef __cplusplus
        15    extern "C" {
        16    #endif
        17
        18    /**
        19     * @brief Initialize the GUI on the readings screen
        20     *
        21     * @param[in] get_ticks_function Required function to read the current time (ms)
        22     *
        23     * @retval true Task initialized successfully.
        24     * @retval false get_ticks_function is NULL; previous state is preserved.
        25     */
        26    bool app_gui_init(uint32_t (*get_ticks_function)(void));
        27
        28    /**
        29     * @brief Update the display with the latest readings or diagnostics
        30     *
        31     * @param[in,out] display Display component
        32     */
        33    void app_gui_run(ssd1306_t *display);
        34
        35    /**
        36     * @brief Alternate between readings and diagnostics after initialization.
        37     */
        38    void app_gui_toggle_screen(void);
        39
        40    #ifdef __cplusplus
        41    }
        42    #endif
        43
        44    #endif
    FILE: app/gui/bitmaps.h
         1    /**
         2     * @file bitmaps.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Provides access to the GUI icon bitmaps.
         5     */
         6    #ifndef BITMAPS_H
         7    #define BITMAPS_H
         8
         9    #include "cmp_ssd1306.h"
        10
        11    /**
        12     * @brief Get the thermometer bitmap.
        13     *
        14     * @retval Thermometer bitmap
        15     */
        16    const ssd1306_bitmap_t *get_thermometer_bitmap(void);
        17
        18    /**
        19     * @brief Get the humidity droplet bitmap.
        20     *
        21     * @retval Droplet bitmap
        22     */
        23    const ssd1306_bitmap_t *get_droplet_bitmap(void);
        24
        25    #endif
    FILE: app/gui/render.h
         1    /**
         2     * @file render.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Internal GUI snapshots and framebuffer rendering.
         5     */
         6    #ifndef RENDER_H
         7    #define RENDER_H
         8
         9    #include "acquisition/app_acquisition.h"
        10    #include "cmp_ssd1306.h"
        11
        12    #include <stdbool.h>
        13    #include <stdint.h>
        14
        15    /**
        16     * @brief Screens selected by the application.
        17     *
        18     */
        19    typedef enum {
        20        SCREEN_READINGS = 0, /** Temperature and humidity. */
        21        SCREEN_DIAGNOSTICS   /** Latest acquisition status and uptime. */
        22    } gui_screen_t;
        23
        24    /**
        25     * @brief Immutable snapshot while its pages are being transferred.
        26     *
        27     */
        28    typedef struct {
        29        acquisition_t acquisition; /** Latest acquisition and retained valid sample. */
        30        uint64_t uptime_seconds;   /** Time since startup, rounded down to seconds. */
        31        gui_screen_t screen;       /** Screen drawn for this snapshot. */
        32        bool has_result;           /** True after a completed acquisition attempt. */
        33    } gui_frame_t;
        34
        35    /**
        36     * @brief Replace the framebuffer with the requested snapshot
        37     *
        38     * @param[in] display Display component.
        39     * @param[in] frame Required snapshot to draw.
        40     *
        41     * @retval SSD1306_STATUS_OK Framebuffer drawn.
        42     * @retval SSD1306_STATUS_INVALID_ARGUMENT Invalid display, sample, or drawing bounds.
        43     * @retval SSD1306_STATUS_NOT_INITIALIZED Display component is unavailable.
        44     * @retval SSD1306_STATUS_NOT_SUPPORTED A font or character is unavailable.
        45     */
        46    ssd1306_status_t render_frame(ssd1306_t *display, const gui_frame_t *frame);
        47
        48    #endif
    FILE: app/status_led/app_status_led.h
         1    /**
         2     * @file app_status_led.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Indicates startup, normal operation, and I2C communication errors.
         5     */
         6    #ifndef APP_STATUS_LED_H
         7    #define APP_STATUS_LED_H
         8
         9    #include "cmp_led.h"
        10
        11    #include <stdbool.h>
        12    #include <stdint.h>
        13
        14    #ifdef __cplusplus
        15    extern "C" {
        16    #endif
        17
        18    /**
        19     * @brief Devices that can report an I2C communication error.
        20     */
        21    typedef enum {
        22        APP_STATUS_LED_I2C_SENSOR = 0, /** SHT31-D communication. */
        23        APP_STATUS_LED_I2C_DISPLAY     /** SSD1306 communication. */
        24    } app_status_led_i2c_source_t;
        25
        26    /**
        27     * @brief Initialize the task in startup mode without writing to the LED.
        28     *
        29     * @param[in] get_ticks_function Required monotonic time source in milliseconds, modulo 2^32.
        30     *
        31     * @retval true Task initialized successfully.
        32     * @retval false get_ticks_function is NULL; previous state is preserved.
        33     */
        34    bool app_status_led_init(uint32_t (*get_ticks_function)(void));
        35
        36    /**
        37     * @brief Select normal operation or startup indication.
        38     *
        39     * @param[in] ready true for normal operation; false for startup. I2C errors take priority.
        40     */
        41    void app_status_led_set_ready(bool ready);
        42
        43    /**
        44     * @brief Update one device's I2C error without clearing the other device's error.
        45     *
        46     * @param[in] source Sensor or display; invalid values are ignored.
        47     * @param[in] active true while communication has failed; false after recovery.
        48     */
        49    void app_status_led_set_i2c_error(app_status_led_i2c_source_t source, bool active);
        50
        51    /**
        52     * @brief Keep the LED off at startup, on when ready, and blinking on I2C errors.
        53     *
        54     * @details Call sequentially for the same LED in normal context. Only changed
        55     * requests are written. Late calls perform at most one transition.
        56     *
        57     * @param[in] led LED component; NULL and calls before initialization are ignored.
        58     */
        59    void app_status_led_run(const led_t *led);
        60
        61    #ifdef __cplusplus
        62    }
        63    #endif
        64
        65    #endif
    FILE: app/telemetry/app_telemetry.h
         1    /**
         2     * @file app_telemetry.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Queues readings and error occurrences for cooperative UART transmission.
         5     */
         6    #ifndef APP_TELEMETRY_H
         7    #define APP_TELEMETRY_H
         8
         9    #include "cmp_telemetry.h"
        10
        11    #include <stdbool.h>
        12    #include <stdint.h>
        13
        14    #ifdef __cplusplus
        15    extern "C" {
        16    #endif
        17
        18    /** @brief Devices that produce error records. */
        19    typedef enum {
        20        APP_TELEMETRY_SOURCE_SENSOR = 0, /** SHT31-D component status codes. */
        21        APP_TELEMETRY_SOURCE_DISPLAY     /** SSD1306 component status codes. */
        22    } telemetry_source_t;
        23
        24    /**
        25     * @brief Initialize the telemetry task
        26     *
        27     * @param[in] get_ticks_function Required
        28     *
        29     * @retval true Task initialized successfully.
        30     * @retval false get_ticks_function is NULL; previous state is preserved.
        31     */
        32    bool app_telemetry_init(uint32_t (*get_ticks_function)(void));
        33
        34    /**
        35     * @brief Send a reading sample to the telemetry queue
        36     *
        37     * @param[in] sample Sample to send
        38     *
        39     * @retval true Reading queued.
        40     * @retval false sample is NULL, the task is unavailable, or the queue is full.
        41     */
        42    bool app_telemetry_report_sample(const telemetry_sample_t *sample);
        43
        44    /**
        45     * @brief Send an error occurrence to the telemetry queue
        46     *
        47     * @param[in] source Device that reported the error.
        48     * @param[in] code Error code from the device's component.
        49     *
        50     * @retval true Error queued.
        51     * @retval false source is invalid, the task is unavailable, or the queue is full.
        52     */
        53    bool app_telemetry_report_error(telemetry_source_t source, int32_t code);
        54
        55    /**
        56     * @brief Transmit queued readings and errors to the telemetry component
        57     *
        58     * @param[in] telemetry Telemetry component
        59     */
        60    void app_telemetry_run(const telemetry_t *telemetry);
        61
        62    #ifdef __cplusplus
        63    }
        64    #endif
        65
        66    #endif
    FILE: app/telemetry/app_telemetry_private.h
         1    /**
         2     * @file app_telemetry_private.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Private
         5     */
         6    #ifndef APP_TELEMETRY_PRIVATE_H
         7    #define APP_TELEMETRY_PRIVATE_H
         8
         9    #ifdef __cplusplus
        10    extern "C" {
        11    #endif
        12
        13    #include <stdbool.h>
        14    #include <stdint.h>
        15
        16    #include "telemetry/cmp_telemetry_types.h"
        17
        18    #define QUEUE_CAPACITY 8U    /** Maximum number of queued records. */
        19    #define RETRY_INTERVAL 1000U /** Delay after a failed send, in milliseconds. */
        20
        21    /**
        22     * @brief Records accepted by the application queue.
        23     */
        24    typedef enum {
        25        RECORD_SAMPLE = 0, /** One valid sensor measurement. */
        26        RECORD_ERROR       /** One component error occurrence. */
        27    } record_kind_t;
        28
        29    /**
        30     * @brief Owned copy of one reading or error occurrence.
        31     */
        32    typedef struct {
        33        record_kind_t kind;        /** Selected payload. */
        34        telemetry_sample_t sample; /** Reading to transmit. */
        35        telemetry_source_t source; /** Device that produced the error code. */
        36        int32_t code;              /** Original component result. */
        37    } record_t;
        38
        39    #ifdef __cplusplus
        40    }
        41    #endif
        42
        43    #endif
    ✓ • 33ms

## Activity

    $ wc -l hal/interface/*/*.h hal/ports/esp32-wroom-32/*/*.c components/*/*.c app/*/*.c && rg -n '^#\s*include|^#\s*(if|ifdef|ifndef)|VERSION|version|conform|capabil' app components hal/interface hal/ports/esp32-wroom-32/README.md -g '*.[ch]' -g '*.md'
        25 hal/interface/common/hal_pin.h
        25 hal/interface/common/hal_status.h
       110 hal/interface/gpio/hal_gpio.h
       104 hal/interface/i2c/hal_i2c.h
        55 hal/interface/system/hal_system.h
        91 hal/interface/timer/hal_timer.h
       120 hal/interface/uart/hal_uart.h
       136 hal/ports/esp32-wroom-32/gpio/hal_gpio.c
       184 hal/ports/esp32-wroom-32/i2c/hal_i2c.c
        20 hal/ports/esp32-wroom-32/system/hal_system.c
       155 hal/ports/esp32-wroom-32/timer/hal_timer.c
       228 hal/ports/esp32-wroom-32/uart/hal_uart.c
        71 components/button/cmp_button.c
        47 components/led/cmp_led.c
       111 components/sht31d/cmp_sht31d.c
       317 components/ssd1306/cmp_ssd1306.c
       463 components/ssd1306/cmp_ssd1306_fonts.c
       104 components/telemetry/cmp_telemetry.c
       115 app/acquisition/app_acquisition.c
        35 app/button/app_button.c
       115 app/gui/app_gui.c
        70 app/gui/bitmaps.c
       111 app/gui/render.c
        81 app/status_led/app_status_led.c
        94 app/telemetry/app_telemetry.c
      2987 total
    components/led/cmp_led_config.h:6:#ifndef CMP_LED_CONFIG_H
    components/led/cmp_led_config.h:9:#include "cmp_led.h"
    components/led/cmp_led_config.h:11:#include "gpio/hal_gpio.h"
    components/led/cmp_led_config.h:13:#ifdef __cplusplus
    components/led/cmp_led_config.h:41:#ifdef __cplusplus
    components/led/cmp_led.h:7:#ifndef CMP_LED_H
    components/led/cmp_led.h:10:#include <stdbool.h>
    components/led/cmp_led.h:12:#include "cmp_led_types.h"
    components/led/cmp_led.h:14:#ifdef __cplusplus
    components/led/cmp_led.h:46:#ifdef __cplusplus
    hal/interface/gpio/hal_gpio.h:7:#ifndef HAL_GPIO_H
    hal/interface/gpio/hal_gpio.h:10:#include <stdbool.h>
    hal/interface/gpio/hal_gpio.h:12:#include "common/hal_pin.h"
    hal/interface/gpio/hal_gpio.h:13:#include "common/hal_status.h"
    hal/interface/gpio/hal_gpio.h:15:#ifdef __cplusplus
    hal/interface/gpio/hal_gpio.h:57: * @details The port validates all fields and pin capabilities before allocation.
    hal/interface/gpio/hal_gpio.h:106:#ifdef __cplusplus
    hal/interface/system/hal_system.h:6:#ifndef HAL_SYSTEM_H
    hal/interface/system/hal_system.h:9:#include <stdint.h>
    hal/interface/system/hal_system.h:11:#include "common/hal_status.h"
    hal/interface/system/hal_system.h:13:#ifdef __cplusplus
    hal/interface/system/hal_system.h:51:#ifdef __cplusplus
    hal/interface/timer/hal_timer.h:7:#ifndef HAL_TIMER_H
    hal/interface/timer/hal_timer.h:10:#ifdef __cplusplus
    hal/interface/timer/hal_timer.h:14:#include <stdint.h>
    hal/interface/timer/hal_timer.h:16:#include "common/hal_status.h"
    hal/interface/timer/hal_timer.h:87:#ifdef __cplusplus
    app/button/app_button.h:6:#ifndef APP_BUTTON_H
    app/button/app_button.h:9:#ifdef __cplusplus
    app/button/app_button.h:13:#include "cmp_button.h"
    app/button/app_button.h:27:#ifdef __cplusplus
    app/button/app_button.c:6:#include "app_button.h"
    app/button/app_button.c:7:#include "gui/app_gui.h"
    app/button/app_button.c:9:#include <stddef.h>
    hal/interface/common/hal_status.h:7:#ifndef HAL_STATUS_H
    hal/interface/common/hal_pin.h:7:#ifndef HAL_PIN_H
    hal/interface/common/hal_pin.h:10:#include <stdint.h>
    hal/interface/common/hal_pin.h:17: * The BSP defines mappings and wiring; the port validates pin capabilities.
    app/status_led/app_status_led.c:6:#include "app_status_led.h"
    app/status_led/app_status_led.c:8:#include <stddef.h>
    hal/interface/i2c/hal_i2c.h:7:#ifndef HAL_I2C_H
    hal/interface/i2c/hal_i2c.h:10:#include <stddef.h>
    hal/interface/i2c/hal_i2c.h:11:#include <stdint.h>
    hal/interface/i2c/hal_i2c.h:13:#include "common/hal_pin.h"
    hal/interface/i2c/hal_i2c.h:14:#include "common/hal_status.h"
    hal/interface/i2c/hal_i2c.h:16:#ifdef __cplusplus
    hal/interface/i2c/hal_i2c.h:20:#include <stdint.h>
    hal/interface/i2c/hal_i2c.h:22:#include "common/hal_status.h"
    hal/interface/i2c/hal_i2c.h:100:#ifdef __cplusplus
    app/status_led/app_status_led.h:6:#ifndef APP_STATUS_LED_H
    app/status_led/app_status_led.h:9:#include "cmp_led.h"
    app/status_led/app_status_led.h:11:#include <stdbool.h>
    app/status_led/app_status_led.h:12:#include <stdint.h>
    app/status_led/app_status_led.h:14:#ifdef __cplusplus
    app/status_led/app_status_led.h:61:#ifdef __cplusplus
    hal/interface/uart/hal_uart.h:7:#ifndef HAL_UART_H
    hal/interface/uart/hal_uart.h:10:#ifdef __cplusplus
    hal/interface/uart/hal_uart.h:14:#include <stddef.h>
    hal/interface/uart/hal_uart.h:15:#include <stdint.h>
    hal/interface/uart/hal_uart.h:17:#include "common/hal_pin.h"
    hal/interface/uart/hal_uart.h:18:#include "common/hal_status.h"
    hal/interface/uart/hal_uart.h:116:#ifdef __cplusplus
    components/led/cmp_led_types.h:6:#ifndef CMP_LED_TYPES_H
    components/led/cmp_led_types.h:9:#ifdef __cplusplus
    components/led/cmp_led_types.h:28:#ifdef __cplusplus
    components/led/cmp_led.c:6:#include "cmp_led_config.h"
    components/led/cmp_led.c:8:#include <stddef.h>
    app/telemetry/app_telemetry.h:6:#ifndef APP_TELEMETRY_H
    app/telemetry/app_telemetry.h:9:#include "cmp_telemetry.h"
    app/telemetry/app_telemetry.h:11:#include <stdbool.h>
    app/telemetry/app_telemetry.h:12:#include <stdint.h>
    app/telemetry/app_telemetry.h:14:#ifdef __cplusplus
    app/telemetry/app_telemetry.h:62:#ifdef __cplusplus
    app/gui/bitmaps.h:6:#ifndef BITMAPS_H
    app/gui/bitmaps.h:9:#include "cmp_ssd1306.h"
    app/gui/render.h:6:#ifndef RENDER_H
    app/gui/render.h:9:#include "acquisition/app_acquisition.h"
    app/gui/render.h:10:#include "cmp_ssd1306.h"
    app/gui/render.h:12:#include <stdbool.h>
    app/gui/render.h:13:#include <stdint.h>
    app/telemetry/app_telemetry.c:6:#include "app_telemetry.h"
    app/telemetry/app_telemetry.c:7:#include "app_telemetry_private.h"
    app/telemetry/app_telemetry.c:9:#include <stddef.h>
    app/gui/app_gui.h:6:#ifndef APP_GUI_H
    app/gui/app_gui.h:9:#include "cmp_ssd1306.h"
    app/gui/app_gui.h:11:#include <stdbool.h>
    app/gui/app_gui.h:12:#include <stdint.h>
    app/gui/app_gui.h:14:#ifdef __cplusplus
    app/gui/app_gui.h:40:#ifdef __cplusplus
    app/gui/app_gui.c:6:#include "app_gui.h"
    app/gui/app_gui.c:8:#include <stddef.h>
    app/gui/app_gui.c:10:#include "acquisition/app_acquisition.h"
    app/gui/app_gui.c:11:#include "render.h"
    app/gui/app_gui.c:12:#include "status_led/app_status_led.h"
    app/gui/app_gui.c:13:#include "telemetry/app_telemetry.h"
    app/telemetry/app_telemetry_private.h:6:#ifndef APP_TELEMETRY_PRIVATE_H
    app/telemetry/app_telemetry_private.h:9:#ifdef __cplusplus
    app/telemetry/app_telemetry_private.h:13:#include <stdbool.h>
    app/telemetry/app_telemetry_private.h:14:#include <stdint.h>
    app/telemetry/app_telemetry_private.h:16:#include "telemetry/cmp_telemetry_types.h"
    app/telemetry/app_telemetry_private.h:39:#ifdef __cplusplus
    app/gui/bitmaps.c:6:#include "bitmaps.h"
    components/button/cmp_button_config.h:6:#ifndef CMP_BUTTON_CONFIG_H
    components/button/cmp_button_config.h:9:#include "cmp_button.h"
    components/button/cmp_button_config.h:11:#include "gpio/hal_gpio.h"
    components/button/cmp_button_config.h:13:#ifdef __cplusplus
    components/button/cmp_button_config.h:50:#ifdef __cplusplus
    app/gui/render.c:6:#include "render.h"
    app/gui/render.c:7:#include "bitmaps.h"
    app/gui/render.c:9:#include <stdio.h>
    components/button/cmp_button.h:7:#ifndef CMP_BUTTON_H
    components/button/cmp_button.h:10:#include <stdbool.h>
    components/button/cmp_button.h:11:#include <stdint.h>
    components/button/cmp_button.h:13:#include "cmp_button_types.h"
    components/button/cmp_button.h:15:#ifdef __cplusplus
    components/button/cmp_button.h:47:#ifdef __cplusplus
    app/acquisition/app_acquisition.h:6:#ifndef APP_ACQUISITION_H
    app/acquisition/app_acquisition.h:9:#include "cmp_sht31d.h"
    app/acquisition/app_acquisition.h:11:#include <stdbool.h>
    app/acquisition/app_acquisition.h:12:#include <stdint.h>
    app/acquisition/app_acquisition.h:14:#ifdef __cplusplus
    app/acquisition/app_acquisition.h:36: * @retval false get_ticks_function is NULL, a measurement is pending, or its conversion wait is unfinished; previous state is preserved.
    app/acquisition/app_acquisition.h:57:#ifdef __cplusplus
    components/button/cmp_button_types.h:6:#ifndef CMP_BUTTON_TYPES_H
    components/button/cmp_button_types.h:9:#include <stdint.h>
    components/button/cmp_button_types.h:11:#ifdef __cplusplus
    components/button/cmp_button_types.h:38:#ifdef __cplusplus
    components/button/cmp_button.c:6:#include "cmp_button_config.h"
    components/button/cmp_button.c:8:#include <stddef.h>
    components/sht31d/cmp_sht31d.h:6:#ifndef CMP_SHT31D_H
    components/sht31d/cmp_sht31d.h:9:#ifdef __cplusplus
    components/sht31d/cmp_sht31d.h:13:#include <stdint.h>
    components/sht31d/cmp_sht31d.h:15:#define SHT31D_MEASUREMENT_TIME 16 /** Minimum conversion wait after cmp_sht31d_start_measurement() returns (ms) */
    components/sht31d/cmp_sht31d.h:38: * @details Values use the datasheet conversion, rounded to the nearest integer.
    components/sht31d/cmp_sht31d.h:53: * first and let any previous conversion finish. Does not reset the sensor or
    components/sht31d/cmp_sht31d.h:71: * @details Sends 0x2400 without waiting for conversion. After success, the caller
    components/sht31d/cmp_sht31d.h:74: * pending, but conversion may have started: wait the full interval before retry
    components/sht31d/cmp_sht31d.h:92: * @details Wait the full conversion interval first; an early NACK is reported
    components/sht31d/cmp_sht31d.h:112:#ifdef __cplusplus
    app/acquisition/app_acquisition.c:6:#include "app_acquisition.h"
    app/acquisition/app_acquisition.c:7:#include "status_led/app_status_led.h"
    app/acquisition/app_acquisition.c:8:#include "telemetry/app_telemetry.h"
    app/acquisition/app_acquisition.c:10:#include <stddef.h>
    app/acquisition/app_acquisition.c:14:_Static_assert(APP_ACQUISITION_INTERVAL >= SHT31D_MEASUREMENT_TIME, "Retry interval must cover sensor conversion time");
    app/acquisition/app_acquisition.c:18:    APP_ACQUISITION_WAITING,      /** Wait for conversion before reading. */
    components/sht31d/cmp_sht31d.c:1:#include "cmp_sht31d.h"
    components/sht31d/cmp_sht31d.c:2:#include "cmp_sht31d_config.h"
    components/sht31d/cmp_sht31d.c:4:#include <stddef.h>
    components/sht31d/cmp_sht31d.c:66:    // Single shot, high repeatability, no clock stretching. Conversion is external
    components/sht31d/cmp_sht31d_config.h:6:#ifndef CMP_SHT31D_CONFIG_H
    components/sht31d/cmp_sht31d_config.h:9:#ifdef __cplusplus
    components/sht31d/cmp_sht31d_config.h:13:#include <stdbool.h>
    components/sht31d/cmp_sht31d_config.h:14:#include <stdint.h>
    components/sht31d/cmp_sht31d_config.h:16:#include "cmp_sht31d.h"
    components/sht31d/cmp_sht31d_config.h:18:#include "i2c/hal_i2c.h"
    components/sht31d/cmp_sht31d_config.h:46:#ifdef __cplusplus
    components/telemetry/cmp_telemetry.h:7:#ifndef CMP_TELEMETRY_H
    components/telemetry/cmp_telemetry.h:10:#include "cmp_telemetry_types.h"
    components/telemetry/cmp_telemetry.h:14:#ifdef __cplusplus
    components/telemetry/cmp_telemetry.h:75:#ifdef __cplusplus
    components/telemetry/cmp_telemetry_types.h:6:#ifndef CMP_TELEMETRY_TYPES_H
    components/telemetry/cmp_telemetry_types.h:9:#include <stdint.h>
    components/telemetry/cmp_telemetry_types.h:11:#ifdef __cplusplus
    components/telemetry/cmp_telemetry_types.h:38:#ifdef __cplusplus
    components/telemetry/cmp_telemetry_config.h:6:#ifndef CMP_TELEMETRY_CONFIG_H
    components/telemetry/cmp_telemetry_config.h:9:#include <stdbool.h>
    components/telemetry/cmp_telemetry_config.h:11:#include "cmp_telemetry.h"
    components/telemetry/cmp_telemetry_config.h:12:#include "uart/hal_uart.h"
    components/telemetry/cmp_telemetry_config.h:14:#ifdef __cplusplus
    components/telemetry/cmp_telemetry_config.h:33:#ifdef __cplusplus
    components/ssd1306/cmp_ssd1306_types.h:6:#ifndef CMP_SSD1306_TYPES_H
    components/ssd1306/cmp_ssd1306_types.h:9:#ifdef __cplusplus
    components/ssd1306/cmp_ssd1306_types.h:13:#include <stdint.h>
    components/ssd1306/cmp_ssd1306_types.h:51:#ifdef __cplusplus
    components/ssd1306/cmp_ssd1306_fonts.c:10:#include "cmp_ssd1306_fonts_private.h"
    components/ssd1306/cmp_ssd1306_fonts.c:12:#include <stddef.h>
    components/ssd1306/cmp_ssd1306_fonts_private.h:6:#ifndef CMP_SSD1306_FONTS_PRIVATE_H
    components/ssd1306/cmp_ssd1306_fonts_private.h:9:#include "cmp_ssd1306.h"
    components/ssd1306/cmp_ssd1306.h:13:#ifndef CMP_SSD1306_H
    components/ssd1306/cmp_ssd1306.h:16:#include <stdbool.h>
    components/ssd1306/cmp_ssd1306.h:17:#include <stdint.h>
    components/ssd1306/cmp_ssd1306.h:19:#include "cmp_ssd1306_types.h"
    components/ssd1306/cmp_ssd1306.h:21:#ifdef __cplusplus
    components/ssd1306/cmp_ssd1306.h:164:#ifdef __cplusplus
    components/telemetry/cmp_telemetry.c:6:#include "cmp_telemetry_config.h"
    components/telemetry/cmp_telemetry.c:8:#include <inttypes.h>
    components/telemetry/cmp_telemetry.c:9:#include <stddef.h>
    components/telemetry/cmp_telemetry.c:10:#include <stdio.h>
    components/ssd1306/cmp_ssd1306_config.h:6:#ifndef CMP_SSD1306_CONFIG_H
    components/ssd1306/cmp_ssd1306_config.h:9:#include "cmp_ssd1306.h"
    components/ssd1306/cmp_ssd1306_config.h:11:#include "i2c/hal_i2c.h"
    components/ssd1306/cmp_ssd1306_config.h:13:#ifdef __cplusplus
    components/ssd1306/cmp_ssd1306_config.h:44:#ifdef __cplusplus
    components/ssd1306/cmp_ssd1306.c:1:#include "cmp_ssd1306_config.h"
    components/ssd1306/cmp_ssd1306.c:2:#include "cmp_ssd1306_fonts_private.h"
    components/ssd1306/cmp_ssd1306.c:4:#include <stddef.h>
    components/ssd1306/cmp_ssd1306.c:5:#include <string.h>
    ✓ • 0ms

## Activity

    $ cat scripts/build.py scripts/_common.py && ls -la && command -v idf.py
    #!/usr/bin/env python3
    """Build the ESP32 firmware with an activated ESP-IDF 6.1 environment."""
    
    import argparse
    import time
    
    from _common import PROJECT_ROOT, add_build_argument, entrypoint, idf_command, run_command
    
    
    def main():
        parser = argparse.ArgumentParser(description="Compila o firmware com ESP-IDF 6.1.")
        add_build_argument(parser)
        parser.add_argument(
            "-D", "--define", action="append", default=[], metavar="NAME=VALUE",
            help="Definição do CMake; pode ser repetida, por exemplo -D SDKCONFIG=/tmp/sdkconfig.",
        )
        args = parser.parse_args()
        command = idf_command() + ["-C", str(PROJECT_ROOT), "-B", str(args.build_dir), "-D", "IDF_TARGET=esp32"]
        for definition in args.define:
            if "=" not in definition or not definition.split("=", 1)[0]:
                parser.error("Cada definição deve usar NAME=VALUE.")
            command.extend(["-D", definition])
        command.append("build")
    
        print(f"Compilando em {args.build_dir}...", flush=True)
        start = time.monotonic()
        result = run_command(command)
        status = "concluída" if result == 0 else "falhou"
        print(f"Compilação {status} ({time.monotonic() - start:.2f} s).", flush=True)
        return result
    
    
    if __name__ == "__main__":
        raise SystemExit(entrypoint(main))
    """Shared paths, arguments and process helpers for the project scripts."""
    
    import argparse
    import json
    import os
    from pathlib import Path
    import shutil
    import subprocess
    import sys
    
    
    PROJECT_ROOT = Path(__file__).resolve().parent.parent
    DEFAULT_BUILD_DIR = PROJECT_ROOT / "build"
    
    
    def positive_integer(value):
        try:
            number = int(value)
        except ValueError as error:
            raise argparse.ArgumentTypeError("Use um número inteiro positivo.") from error
        if number <= 0:
            raise argparse.ArgumentTypeError("Use um número inteiro positivo.")
        return number
    
    
    def project_path(value):
        path = Path(value).expanduser()
        if not path.is_absolute():
            path = PROJECT_ROOT / path
        return Path(os.path.abspath(path))
    
    
    def add_build_argument(parser):
        parser.add_argument(
            "-B", "--build-dir", type=project_path, default=DEFAULT_BUILD_DIR,
            help="Diretório de build; caminhos relativos partem da raiz do projeto (padrão: build).",
        )
    
    
    def add_port_argument(parser):
        parser.add_argument("port", nargs="?", help="Porta serial, por exemplo /dev/ttyUSB0 ou COM5.")
        parser.add_argument("-p", "--port", dest="port_option", help="Alternativa ao argumento posicional PORT.")
    
    
    def serial_port(args):
        if args.port and args.port_option:
            raise ValueError("Informe a porta apenas uma vez: PORT ou --port.")
        configured = args.port_option or args.port or os.environ.get("FW_SERIAL_PORT") or os.environ.get("ESPPORT")
        if configured:
            return configured
    
        from serial.tools import list_ports
    
        ports = sorted(port.device for port in list_ports.comports() if port.vid is not None)
        if len(ports) == 1:
            return ports[0]
        if ports:
            raise ValueError("Mais de uma porta USB disponível: " + ", ".join(ports) + ". Informe --port.")
        raise ValueError("Nenhuma porta USB detectada. Informe --port ou defina FW_SERIAL_PORT/ESPPORT.")
    
    
    def python_executable():
        environment = os.environ.get("IDF_PYTHON_ENV_PATH")
        if environment:
            relative = "Scripts/python.exe" if os.name == "nt" else "bin/python"
            executable = Path(environment) / relative
            if not executable.is_file():
                raise ValueError("IDF_PYTHON_ENV_PATH não contém um interpretador Python válido.")
            return str(executable)
        return sys.executable
    
    
    def idf_command():
        sdk = os.environ.get("IDF_PATH")
        executable = str(Path(sdk) / "tools" / "idf.py") if sdk else shutil.which("idf.py")
        if not executable or not Path(executable).is_file():
            raise ValueError(
                "ESP-IDF não encontrado. Abra um terminal ESP-IDF 6.1 ou ative o ambiente "
                "com o script gerado pelo instalador (no EIM, activate_idf_<versão>.sh/.ps1). "
                "Para instalações tradicionais, use o export.sh/export.ps1 do SDK."
            )
        return [python_executable(), executable]
    
    
    def read_build_description(build_dir):
        with (build_dir / "project_description.json").open(encoding="utf-8") as source:
            description = json.load(source)
        if not isinstance(description, dict) or not isinstance(description.get("project_path"), str):
            raise ValueError("Descrição de build inválida.")
        if Path(description["project_path"]).resolve() != PROJECT_ROOT:
            raise ValueError("O diretório de build pertence a outro projeto.")
        return description
    
    
    def run_command(command, cwd=PROJECT_ROOT):
        try:
            result = subprocess.run([str(part) for part in command], cwd=cwd, check=False)
            return result.returncode if result.returncode >= 0 else 128 - result.returncode
        except KeyboardInterrupt:
            print("\nOperação interrompida.", file=sys.stderr)
            return 130
    
    
    def entrypoint(main):
        try:
            return main()
        except KeyboardInterrupt:
            print("\nOperação interrompida.", file=sys.stderr)
            return 130
        except (OSError, ValueError, ImportError) as error:
            print(f"Erro: {error}", file=sys.stderr)
            if isinstance(error, ImportError):
                print("Execute com o Python do ambiente ESP-IDF 6.1, que inclui pyserial.", file=sys.stderr)
            return 1
    total 2876
    drwxrwxr-x 15 julia julia    4096 Oct 27 21:51 .
    drwxrwxr-x  5 julia julia    4096 Oct 27 21:48 ..
    drwxrwxr-x  3 julia julia    4096 Oct 27 20:37 .agents
    dr-xr-xr-x  2 julia julia      40 Oct 27 21:51 .aws
    -rw-rw-r--  1 julia julia     931 Oct 27 20:37 .clang-format
    dr-xr-xr-x  2 julia julia      40 Oct 27 21:51 .codex
    drwxrwxr-x  2 julia julia    4096 Oct 27 20:37 .devcontainer
    drwxrwxr-x  9 julia julia    4096 Oct 27 21:48 .git
    -rw-rw-r--  1 julia julia    1188 Oct 27 20:37 .gitignore
    -rw-rw-r--  1 julia julia     818 Oct 27 20:37 .gitmodules
    drwxrwxr-x  2 julia julia    4096 Oct 27 21:47 .vscode
    -rw-rw-r--  1 julia julia     354 Oct 27 20:37 AGENTS.md
    -rw-rw-r--  1 julia julia    2260 Oct 27 21:47 CMakeLists.txt
    drwxrwxr-x  7 julia julia    4096 Oct 27 20:37 app
    drwxrwxr-x  3 julia julia    4096 Oct 27 20:37 bsp
    drwxrwxr-x  7 julia julia    4096 Oct 27 20:37 components
    drwxrwxr-x  2 julia julia    4096 Oct 27 20:37 docs
    drwxrwxr-x  5 julia julia    4096 Oct 27 20:37 hal
    drwxrwxr-x  2 julia julia    4096 Oct 27 21:47 main
    -rw-rw-r--  1 julia julia 2870729 Oct 27 21:47 proposta.pdf
    drwxrwxr-x  2 julia julia    4096 Oct 27 21:47 scripts
    ✗ (1) • 0ms

## Activity

    $ for f in hal/interface/common/hal_status.h hal/interface/timer/hal_timer.h hal/interface/i2c/hal_i2c.h hal/interface/uart/hal_uart.h hal/interface/gpio/hal_gpio.h hal/interface/system/hal_system.h; do echo "FILE: $f"; nl -ba "$f"; done
    FILE: hal/interface/common/hal_status.h
         1    /**
         2     * @file hal_status.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Result codes for HAL operations.
         5     *
         6     */
         7    #ifndef HAL_STATUS_H
         8    #define HAL_STATUS_H
         9
        10    /**
        11     * @brief Result of a framework operation, independent of the platform SDK.
        12     */
        13    typedef enum {
        14        HAL_STATUS_OK = 0,               /** The operation completed successfully */
        15        HAL_STATUS_ERROR = 1,            /** A platform or communication operation failed */
        16        HAL_STATUS_TIMEOUT = 2,          /** The configured platform transfer-wait mechanism timed out */
        17        HAL_STATUS_INVALID_ARGUMENT = 3, /** A pointer, identifier, value, or combination is invalid */
        18        HAL_STATUS_NOT_SUPPORTED = 4,    /** A valid operation or configuration is unavailable */
        19        HAL_STATUS_NOT_INITIALIZED = 5,  /** Required platform, memory, or resource initialization is missing */
        20        HAL_STATUS_NO_MEMORY = 6,        /** Required object or platform-driver storage is unavailable */
        21        HAL_STATUS_RESOURCE_BUSY = 7,    /** The requested hardware resource is already assigned */
        22        HAL_STATUS_INVALID_STATE = 8     /** The operation is forbidden in the current lifecycle state */
        23    } hal_status_t;
        24
        25    #endif
    FILE: hal/interface/timer/hal_timer.h
         1    /**
         2     * @file hal_timer.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Periodic notifications from a dedicated hardware timer.
         5     *
         6     */
         7    #ifndef HAL_TIMER_H
         8    #define HAL_TIMER_H
         9
        10    #ifdef __cplusplus
        11    extern "C" {
        12    #endif
        13
        14    #include <stdint.h>
        15
        16    #include "common/hal_status.h"
        17
        18    /**
        19     * @brief Opaque hardware timer object.
        20     */
        21    typedef struct hal_timer hal_timer_t;
        22
        23    /**
        24     * @brief Handle a serviced hardware timer expiration in interrupt context.
        25     *
        26     * @details Keep callbacks short and nonblocking; call only ISR-safe HAL APIs.
        27     * Synchronize shared application data for the platform; volatile alone does
        28     * not ensure atomic access.
        29     */
        30    typedef void (*hal_timer_callback_t)(void);
        31
        32    /**
        33     * @brief Fixed periodic configuration copied by the constructor.
        34     *
        35     * @details Reserves dedicated hardware for the firmware lifetime. The port
        36     * validates the period before allocation or setup and documents supported
        37     * periods/resolution. Unavailable settings return NOT_SUPPORTED without substitution.
        38     */
        39    typedef struct {
        40        uint8_t instance;              /** Port-defined hardware timer resource identifier selected by the BSP */
        41        uint32_t period;               /** Positive interval between periodic hardware expirations, in milliseconds */
        42        hal_timer_callback_t callback; /** Required expiration callback; must not be NULL */
        43    } hal_timer_config_t;
        44
        45    /**
        46     * @brief Construct and configure a periodic hardware timer without starting it.
        47     *
        48     * @details Construct sequentially with initialized, unsealed framework memory.
        49     * Configuration is fixed; callbacks begin only after hal_timer_start(). Failure
        50     * may consume arena space. No destruction or runtime reconfiguration.
        51     *
        52     * @param[out] timer Timer object pointer to receive the allocated handle; must not be NULL.
        53     * @param[in] config Complete startup configuration; must not be NULL.
        54     *
        55     * @retval HAL_STATUS_OK The timer is configured and stopped.
        56     * @retval HAL_STATUS_INVALID_ARGUMENT A required pointer, callback, identifier, or period is invalid.
        57     * @retval HAL_STATUS_NOT_SUPPORTED The requested period or dedicated hardware resource is unavailable on this platform.
        58     * @retval HAL_STATUS_NOT_INITIALIZED Platform or arena initialization is missing.
        59     * @retval HAL_STATUS_INVALID_STATE The construction phase has ended.
        60     * @retval HAL_STATUS_RESOURCE_BUSY The selected hardware resource is already assigned or the hardware timer pool is exhausted.
        61     * @retval HAL_STATUS_NO_MEMORY Framework or SDK storage is unavailable.
        62     * @retval HAL_STATUS_ERROR The platform failed to configure the timer.
        63     */
        64    hal_status_t hal_timer_config(hal_timer_t **timer, const hal_timer_config_t *config);
        65
        66    /**
        67     * @brief Start a configured periodic timer once, after application initialization.
        68     *
        69     * @details Call in normal context after initializing dependencies and callback
        70     * data. Allocates no framework objects; allowed after arena sealing. First
        71     * expiration follows one period from hardware activation; the callback may run
        72     * before return. Repeated starts do not restart it; failed starts leave no callbacks.
        73     * Interrupt latency may delay callbacks or coalesce expirations under port-defined
        74     * conditions. Missed periods need not be recovered; a callback counter measures
        75     * delivered notifications, not necessarily physical elapsed time.
        76     *
        77     * @param[in] timer Valid handle returned by hal_timer_config().
        78     *
        79     * @retval HAL_STATUS_OK The timer is running with periodic notifications enabled.
        80     * @retval HAL_STATUS_INVALID_ARGUMENT The handle is NULL.
        81     * @retval HAL_STATUS_NOT_INITIALIZED The timer or platform is not ready.
        82     * @retval HAL_STATUS_INVALID_STATE The timer is already running.
        83     * @retval HAL_STATUS_ERROR The platform failed to start the timer.
        84     */
        85    hal_status_t hal_timer_start(hal_timer_t *timer);
        86
        87    #ifdef __cplusplus
        88    }
        89    #endif
        90
        91    #endif
    FILE: hal/interface/i2c/hal_i2c.h
         1    /**
         2     * @file hal_i2c.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Synchronous, addressed I2C master bus contract.
         5     *
         6     */
         7    #ifndef HAL_I2C_H
         8    #define HAL_I2C_H
         9
        10    #include <stddef.h>
        11    #include <stdint.h>
        12
        13    #include "common/hal_pin.h"
        14    #include "common/hal_status.h"
        15
        16    #ifdef __cplusplus
        17    extern "C" {
        18    #endif
        19
        20    #include <stdint.h>
        21
        22    #include "common/hal_status.h"
        23
        24    /**
        25     * @brief Opaque structure representing an I2C peripheral.
        26     *
        27     */
        28    typedef struct hal_i2c hal_i2c_t;
        29
        30    /**
        31     * @brief I2C configuration structure.
        32     *
        33     * @details The board supplies external pull-ups. The port configures open-drain
        34     * routing; separate GPIO objects must not claim these pins. Build-time routing
        35     * may limit accepted configurations.
        36     * The constructor validates a positive, finite timeout before allocation or
        37     * hardware setup. Port-defined values/resolution apply; a fixed-limit port
        38     * accepts only its matching setting, otherwise returning NOT_SUPPORTED.
        39     * No infinite wait; the driver timeout is not a strict whole-call deadline.
        40     */
        41    typedef struct {
        42        uint8_t instance;    /** Zero-based controller identifier mapped by the BSP */
        43        hal_pin_t clock_pin; /** Serial clock pin */
        44        hal_pin_t data_pin;  /** Serial data pin; must differ from clock_pin */
        45        uint32_t frequency;  /** Requested bus clock frequency in hertz; must be positive */
        46        uint32_t timeout;    /** Positive lifetime driver timeout in milliseconds; port-defined values/resolution */
        47    } hal_i2c_config_t;
        48
        49    /**
        50     * @brief Construct a master bus object and configure its controller and pins.
        51     *
        52     * @param[out] i2c I2C object pointer to receive the allocated handle; must not be NULL.
        53     * @param[in] config Pointer to the GPIO configuration. must not be NULL
        54     *
        55     * @retval HAL_STATUS_OK The bus is ready; this does not probe any attached device.
        56     * @retval HAL_STATUS_INVALID_ARGUMENT A pointer, identifier, frequency, timeout, or pin combination is invalid.
        57     * @retval HAL_STATUS_NOT_SUPPORTED The requested frequency, routing, or timeout setting is unavailable.
        58     * @retval HAL_STATUS_NOT_INITIALIZED Platform or arena initialization is missing.
        59     * @retval HAL_STATUS_INVALID_STATE The construction phase has ended.
        60     * @retval HAL_STATUS_RESOURCE_BUSY The controller or either pin is already assigned.
        61     * @retval HAL_STATUS_NO_MEMORY Object storage is unavailable.
        62     * @retval HAL_STATUS_ERROR The platform failed to configure the bus.
        63     */
        64    hal_status_t hal_i2c_config(hal_i2c_t **i2c, const hal_i2c_config_t *config);
        65
        66    /**
        67     * @brief Write a nonempty block in one synchronous transaction ending with STOP.
        68     *
        69     * @param[in] i2c I2C object pointer.
        70     * @param[in] address Unshifted seven-bit address from 0x08 through 0x77; reserved addresses are excluded.
        71     * @param[in] buffer Source data; must not be NULL.
        72     * @param[in] size Number of source bytes
        73     *
        74     * @retval HAL_STATUS_OK All bytes were accepted by the addressed device.
        75     * @retval HAL_STATUS_INVALID_ARGUMENT A pointer, address, or size is invalid.
        76     * @retval HAL_STATUS_NOT_INITIALIZED The bus is not ready.
        77     * @retval HAL_STATUS_NOT_SUPPORTED The port does not provide this operation or transfer size.
        78     * @retval HAL_STATUS_TIMEOUT The configured driver timeout expired; device state may have changed.
        79     * @retval HAL_STATUS_ERROR A NACK, arbitration loss, or other bus error occurred.
        80     */
        81    hal_status_t hal_i2c_write(hal_i2c_t *i2c, uint8_t address, const uint8_t *buffer, uint32_t size);
        82
        83    /**
        84     * @brief Read a nonempty block in one synchronous transaction ending with STOP.
        85     *
        86     * @param[in] i2c I2C object pointer
        87     * @param[in] address Unshifted seven-bit address from 0x08 through 0x77.
        88     * @param[out] buffer Destination for size bytes; may contain partial data on failure.
        89     * @param[in] size Number of bytes to receive; must be positive.
        90     *
        91     * @retval HAL_STATUS_OK All requested bytes were received.
        92     * @retval HAL_STATUS_INVALID_ARGUMENT A pointer, address, or size is invalid.
        93     * @retval HAL_STATUS_NOT_INITIALIZED The bus is not ready.
        94     * @retval HAL_STATUS_NOT_SUPPORTED The port does not provide this operation or transfer size.
        95     * @retval HAL_STATUS_TIMEOUT The configured driver timeout expired; discard the destination contents.
        96     * @retval HAL_STATUS_ERROR A NACK, arbitration loss, or other bus error occurred.
        97     */
        98    hal_status_t hal_i2c_read(hal_i2c_t *i2c, uint8_t address, uint8_t *buffer, uint32_t size);
        99
       100    #ifdef __cplusplus
       101    }
       102    #endif
       103
       104    #endif
    FILE: hal/interface/uart/hal_uart.h
         1    /**
         2     * @file hal_uart.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Synchronous UART block transmission contract.
         5     *
         6     */
         7    #ifndef HAL_UART_H
         8    #define HAL_UART_H
         9
        10    #ifdef __cplusplus
        11    extern "C" {
        12    #endif
        13
        14    #include <stddef.h>
        15    #include <stdint.h>
        16
        17    #include "common/hal_pin.h"
        18    #include "common/hal_status.h"
        19
        20    /**
        21     * @brief Opaque structure representing an UART peripheral.
        22     *
        23     */
        24    typedef struct hal_uart hal_uart_t;
        25
        26    /**
        27     * @brief Payload bits per character, excluding parity and framing bits.
        28     */
        29    typedef enum {
        30        HAL_UART_DATA_WIDTH_7 = 7, /** Seven payload bits; unavailable on some platforms */
        31        HAL_UART_DATA_WIDTH_8 = 8  /** Eight payload bits used by the proof of concept */
        32    } hal_uart_data_width_t;
        33
        34    /**
        35     * @brief Optional parity bit generated by the controller.
        36     */
        37    typedef enum {
        38        HAL_UART_PARITY_NONE = 0, /** No parity bit */
        39        HAL_UART_PARITY_EVEN,     /** Even parity; subject to platform support */
        40        HAL_UART_PARITY_ODD       /** Odd parity; subject to platform support */
        41    } hal_uart_parity_t;
        42
        43    /**
        44     * @brief Stop bits appended to each character.
        45     */
        46    typedef enum {
        47        HAL_UART_STOP_COUNT_1 = 1, /** One stop bit */
        48        HAL_UART_STOP_COUNT_2 = 2  /** Two stop bits; subject to platform support */
        49    } hal_uart_stop_count_t;
        50
        51    /**
        52     * @brief UART configuration copied by the constructor.
        53     *
        54     * @details Pins must route to instance, matching BSP build-time routing where
        55     * required; arbitrary runtime remapping is not guaranteed. No hardware flow
        56     * control or receive API; receive_pin describes wiring. Payload width excludes
        57     * parity, regardless of the SDK's word-length convention.
        58     * The constructor validates a positive, finite timeout before allocation or
        59     * hardware setup. Port-defined values/resolution apply; a fixed-limit port
        60     * accepts only its matching setting, otherwise returning NOT_SUPPORTED.
        61     * No infinite wait; the driver timeout is not a strict whole-call deadline.
        62     */
        63    typedef struct {
        64        uint8_t instance;                 /** Zero-based controller identifier */
        65        hal_pin_t transmit_pin;           /** Transmit pin */
        66        hal_pin_t receive_pin;            /** Receive pin; must differ from transmit_pin */
        67        uint32_t baudrate;                /** Requested symbol rate in baud; must be positive */
        68        hal_uart_data_width_t data_width; /** Number of payload bits per character */
        69        hal_uart_parity_t parity;         /** Parity configuration */
        70        hal_uart_stop_count_t stop_count; /** Number of stop bits per character */
        71        uint32_t timeout;                 /** Positive lifetime driver timeout in milliseconds; port-defined values/resolution */
        72    } hal_uart_config_t;
        73
        74    /**
        75     * @brief Construct a UART object and configure its controller and pin route.
        76     *
        77     * @param[out] uart UART Object pointer.
        78     * @param[in] config Pointer to the UART configuration. must not be NULL
        79     *
        80     * @retval HAL_STATUS_OK The UART is ready for block transmission.
        81     * @retval HAL_STATUS_INVALID_ARGUMENT A pointer, identifier, baudrate, timeout, enum, or pin combination is invalid.
        82     * @retval HAL_STATUS_NOT_SUPPORTED A valid frame format, baudrate, route, or timeout setting is unavailable.
        83     * @retval HAL_STATUS_NOT_INITIALIZED Platform or arena initialization is missing.
        84     * @retval HAL_STATUS_INVALID_STATE The construction phase has ended.
        85     * @retval HAL_STATUS_RESOURCE_BUSY The controller or a requested pin is already assigned.
        86     * @retval HAL_STATUS_NO_MEMORY Object storage is unavailable.
        87     * @retval HAL_STATUS_ERROR The platform failed to configure the UART.
        88     */
        89    hal_status_t hal_uart_config(hal_uart_t **uart, const hal_uart_config_t *config);
        90
        91    /**
        92     * @brief Write data to an UART device.
        93     *
        94     * @details Success means all bytes were submitted and the SDK transfer completed,
        95     * without confirming remote reception. The count is the accepted prefix; bytes
        96     * may still leave the wire after failure. The source buffer can be reused on
        97     * return; the port retains no reference. Uses the configured driver wait limit.
        98     * No retries; resending after a partial result may duplicate bytes. Seven-bit
        99     * mode validates every byte is <= 0x7F before transmitting.
       100     *
       101     * @param[in] uart Valid UART handle.
       102     * @param[in] buffer Source data; may be NULL only when size is zero.
       103     * @param[in] size Number of source bytes. Zero is a successful no-op for valid arguments.
       104     * @param[out] transmitted_count Required output initialized to zero before argument validation;
       105     * receives size on success or the accepted prefix length on transfer failure.
       106     *
       107     * @retval HAL_STATUS_OK All bytes were accepted and the transfer completed, or size was zero.
       108     * @retval HAL_STATUS_INVALID_ARGUMENT A pointer or payload byte is invalid.
       109     * @retval HAL_STATUS_NOT_INITIALIZED The UART is not ready.
       110     * @retval HAL_STATUS_NOT_SUPPORTED The port cannot provide this operation or transfer size.
       111     * @retval HAL_STATUS_TIMEOUT The configured timeout expired; transmitted_count may be nonzero.
       112     * @retval HAL_STATUS_ERROR A transmission error occurred; transmitted_count may be nonzero.
       113     */
       114    hal_status_t hal_uart_write(hal_uart_t *uart, const uint8_t *buffer, uint32_t size, uint32_t *transmitted_count);
       115
       116    #ifdef __cplusplus
       117    }
       118    #endif
       119
       120    #endif
    FILE: hal/interface/gpio/hal_gpio.h
         1    /**
         2     * @file hal_gpio.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Portable digital input and push-pull output contract.
         5     *
         6     */
         7    #ifndef HAL_GPIO_H
         8    #define HAL_GPIO_H
         9
        10    #include <stdbool.h>
        11
        12    #include "common/hal_pin.h"
        13    #include "common/hal_status.h"
        14
        15    #ifdef __cplusplus
        16    extern "C" {
        17    #endif
        18
        19    /**
        20     * @brief Opaque GPIO object owned by the framework.
        21     */
        22    typedef struct hal_gpio hal_gpio_t;
        23
        24    /**
        25     * @brief Digital direction selected when constructing a GPIO object.
        26     */
        27    typedef enum {
        28        HAL_GPIO_INPUT = 0, /** Digital input sampled through hal_gpio_read() */
        29        HAL_GPIO_OUTPUT     /** Push-pull digital output controlled through hal_gpio_write() */
        30    } hal_gpio_direction_t;
        31
        32    /**
        33     * @brief Internal resistor selection; external resistors are unaffected.
        34     */
        35    typedef enum {
        36        HAL_GPIO_PULL_NONE = 0, /** Disable internal pull-up and pull-down resistors */
        37        HAL_GPIO_PULL_UP,       /** Enable an internal pull-up resistor */
        38        HAL_GPIO_PULL_DOWN      /** Enable an internal pull-down resistor */
        39    } hal_gpio_pull_t;
        40
        41    /**
        42     * @brief GPIO configuration.
        43     *
        44     * @details GPIO operations always use the physical electrical level.
        45     *
        46     */
        47    typedef struct {
        48        hal_pin_t pin;                  /** Physical bank and pin */
        49        hal_gpio_direction_t direction; /** Digital input or push-pull output */
        50        hal_gpio_pull_t pull;           /** Requested internal resistor configuration */
        51        bool initial_level;             /** Initial output level: false is low, true is high; ignored for inputs */
        52    } hal_gpio_config_t;
        53
        54    /**
        55     * @brief Construct a GPIO object and configure its physical pin during startup.
        56     *
        57     * @details The port validates all fields and pin capabilities before allocation.
        58     * For an output, the requested initial level is established as part of enabling
        59     * the output; a global guarantee of glitch-free transitions is not provided.
        60     * Construction requires an initialized, unsealed framework memory.
        61     * The framework owns object storage; failed construction may consume arena
        62     * space without guaranteeing memory or hardware rollback.
        63     *
        64     * @param[out] gpio GPIO object pointer to receive the allocated handle; must not be NULL.
        65     * @param[in] config Pointer to the GPIO configuration. must not be NULL
        66     *
        67     * @retval HAL_STATUS_OK The object and pin are ready for the configured direction.
        68     * @retval HAL_STATUS_INVALID_ARGUMENT A pointer, pin identifier, or enum value is invalid.
        69     * @retval HAL_STATUS_NOT_SUPPORTED The selected pin cannot provide the requested configuration.
        70     * @retval HAL_STATUS_NOT_INITIALIZED Platform or arena initialization is missing.
        71     * @retval HAL_STATUS_INVALID_STATE The construction phase has ended.
        72     * @retval HAL_STATUS_RESOURCE_BUSY The physical pin is already assigned.
        73     * @retval HAL_STATUS_NO_MEMORY Object storage is unavailable.
        74     * @retval HAL_STATUS_ERROR The platform failed to configure the pin.
        75     */
        76    hal_status_t hal_gpio_config(hal_gpio_t **gpio, const hal_gpio_config_t *config);
        77
        78    /**
        79     * @brief Synchronously sample the physical level of a digital input.
        80     *
        81     * @param[in] gpio Valid GPIO handle configured as an input.
        82     * @param[out] level Receives false for low or true for high, only on success.
        83     *
        84     * @retval HAL_STATUS_OK The input level was read.
        85     * @retval HAL_STATUS_INVALID_ARGUMENT A required pointer is NULL.
        86     * @retval HAL_STATUS_NOT_INITIALIZED The resource is not ready.
        87     * @retval HAL_STATUS_NOT_SUPPORTED The handle is configured as an output.
        88     * @retval HAL_STATUS_ERROR The platform could not read the pin.
        89     */
        90    hal_status_t hal_gpio_read(hal_gpio_t *gpio, bool *level);
        91
        92    /**
        93     * @brief Synchronously set the physical level of a push-pull output.
        94     *
        95     * @param[in] gpio Valid GPIO handle configured as an output.
        96     * @param[in] level Requested electrical level: false is low and true is high.
        97     *
        98     * @retval HAL_STATUS_OK The output operation was issued successfully.
        99     * @retval HAL_STATUS_INVALID_ARGUMENT The handle is NULL.
       100     * @retval HAL_STATUS_NOT_INITIALIZED The resource is not ready.
       101     * @retval HAL_STATUS_NOT_SUPPORTED The handle is configured as an input.
       102     * @retval HAL_STATUS_ERROR The platform could not write the pin.
       103     */
       104    hal_status_t hal_gpio_write(hal_gpio_t *gpio, bool level);
       105
       106    #ifdef __cplusplus
       107    }
       108    #endif
       109
       110    #endif
    FILE: hal/interface/system/hal_system.h
         1    /**
         2     * @file hal_system.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief System initialization
         5     */
         6    #ifndef HAL_SYSTEM_H
         7    #define HAL_SYSTEM_H
         8
         9    #include <stdint.h>
        10
        11    #include "common/hal_status.h"
        12
        13    #ifdef __cplusplus
        14    extern "C" {
        15    #endif
        16
        17    /**
        18     * @brief Backing storage supplied by the integration for all HAL objects.
        19     */
        20    typedef struct {
        21        void *memory_buffer;      /** Persistent writable storage aligned for max_align_t. */
        22        uint32_t memory_capacity; /** Positive buffer size in bytes; includes allocation padding. */
        23    } hal_system_config_t;
        24
        25    /**
        26     * @brief Initialize the platform infrastructure and open HAL construction.
        27     *
        28     * @details Call once at startup, before HAL construction and after any SDK
        29     * startup required by the port. Establishes system clocks from the board/SDK
        30     * build configuration, respecting prior SDK initialization. Resource constructors
        31     * complete peripheral clocks, routing, and configuration.
        32     *
        33     * @param[in] config Required system configuration.
        34     *
        35     * @retval HAL_STATUS_OK The system is ready for HAL resource construction.
        36     * @retval HAL_STATUS_INVALID_ARGUMENT config or memory_buffer is NULL, capacity
        37     * is zero, or the buffer alignment is invalid.
        38     * @retval HAL_STATUS_INVALID_STATE System initialization has already succeeded.
        39     * @retval HAL_STATUS_ERROR Required platform initialization failed.
        40     */
        41    hal_status_t hal_system_init(const hal_system_config_t *config);
        42
        43    /**
        44     * @brief Permanently close HAL object allocation after resource construction.
        45     *
        46     * @retval HAL_STATUS_OK Construction is closed, including a repeated seal.
        47     * @retval HAL_STATUS_NOT_INITIALIZED hal_system_init() has not succeeded.
        48     */
        49    hal_status_t hal_system_seal(void);
        50
        51    #ifdef __cplusplus
        52    }
        53    #endif
        54
        55    #endif
    ✓ • 0ms

## Activity

    $ for f in components/button/cmp_button.c components/button/cmp_button_config.h components/button/cmp_button_types.h components/sht31d/cmp_sht31d.c components/led/cmp_led.c components/telemetry/cmp_telemetry.c; do echo "FILE: $f"; nl -ba "$f"; done
    FILE: components/button/cmp_button.c
         1    /**
         2     * @file cmp_button.c
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief GPIO-backed logical button readings.
         5     */
         6    #include "cmp_button_config.h"
         7
         8    #include <stddef.h>
         9
        10    static button_status_t cmp_button_translate_status(hal_status_t status) {
        11        switch (status) {
        12            case HAL_STATUS_OK:
        13                return BUTTON_STATUS_OK;
        14            case HAL_STATUS_INVALID_ARGUMENT:
        15                return BUTTON_STATUS_INVALID_ARGUMENT;
        16            case HAL_STATUS_NOT_INITIALIZED:
        17                return BUTTON_STATUS_NOT_INITIALIZED;
        18            case HAL_STATUS_NOT_SUPPORTED:
        19                return BUTTON_STATUS_NOT_SUPPORTED;
        20            default:
        21                return BUTTON_STATUS_GPIO_ERROR;
        22        }
        23    }
        24
        25    button_status_t cmp_button_init(button_t *button, const button_config_t *config) {
        26        if (button == NULL || config == NULL || config->gpio == NULL || config->get_ticks == NULL) {
        27            return BUTTON_STATUS_INVALID_ARGUMENT;
        28        }
        29        *button = (button_t){
        30            .gpio = config->gpio,
        31            .active_level = config->active_level,
        32            .debounce = config->debounce != 0 ? config->debounce : CMP_BUTTON_DEFAULT_DEBOUNCE,
        33            .get_ticks = config->get_ticks,
        34        };
        35
        36        // Verify the input GPIO without retaining or publishing its initial state.
        37        bool level;
        38        button_status_t status = cmp_button_translate_status(hal_gpio_read(button->gpio, &level));
        39        if (status == BUTTON_STATUS_OK) {
        40            button->initialized = true;
        41        }
        42        return status;
        43    }
        44
        45    button_status_t cmp_button_read(button_t *button, bool *pressed) {
        46        if (button == NULL || pressed == NULL) {
        47            return BUTTON_STATUS_INVALID_ARGUMENT;
        48        }
        49        if (!button->initialized) {
        50            return BUTTON_STATUS_NOT_INITIALIZED;
        51        }
        52
        53        bool level;
        54        button_status_t status = cmp_button_translate_status(hal_gpio_read(button->gpio, &level));
        55        if (status != BUTTON_STATUS_OK) {
        56            button->candidate_valid = false;
        57            return status;
        58        }
        59
        60        uint32_t now = button->get_ticks();
        61        bool sample_pressed = level == button->active_level;
        62        if (!button->candidate_valid || sample_pressed != button->candidate_pressed) {
        63            button->candidate_pressed = sample_pressed;
        64            button->candidate_since = now;
        65            button->candidate_valid = true;
        66        } else if ((uint32_t)(now - button->candidate_since) >= button->debounce) {
        67            button->stable_pressed = sample_pressed;
        68        }
        69        *pressed = button->stable_pressed;
        70        return BUTTON_STATUS_OK;
        71    }
    FILE: components/button/cmp_button_config.h
         1    /**
         2     * @file cmp_button_config.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Construction and caller-owned storage for the button component.
         5     */
         6    #ifndef CMP_BUTTON_CONFIG_H
         7    #define CMP_BUTTON_CONFIG_H
         8
         9    #include "cmp_button.h"
        10
        11    #include "gpio/hal_gpio.h"
        12
        13    #ifdef __cplusplus
        14    extern "C" {
        15    #endif
        16
        17    #define CMP_BUTTON_DEFAULT_DEBOUNCE 30U /** Default stable sample interval (ms) */
        18
        19    /**
        20     * @brief Input resource and electrical polarity supplied by the composition.
        21     *
        22     * @details The BSP configures the input and its pull resistors. This component
        23     * uses the input GPIO without selecting pins or constructing GPIO resources.
        24     */
        25    struct button_config {
        26        hal_gpio_t *gpio;             /** Required, already constructed input GPIO. */
        27        bool active_level;            /** Electrical level representing a pressed button. */
        28        uint32_t debounce;            /** Stable sample interval in milliseconds; zero selects the 30 ms default. */
        29        button_get_ticks_t get_ticks; /** Required time source supplied by the integration. */
        30    };
        31
        32    /**
        33     * @brief Caller-owned persistent context; fields are private to the component.
        34     *
        35     * @details Declare in the composition root and pass button_t pointers to
        36     * consumers. cmp_button_init() sets all fields. No singleton or HAL arena allocation.
        37     */
        38    struct button {
        39        hal_gpio_t *gpio;             /** Input GPIO handle. */
        40        bool active_level;            /** Electrical level representing a pressed button. */
        41        bool initialized;             /** True after successful GPIO verification. */
        42        uint32_t debounce;            /** Effective stable sample interval in milliseconds. */
        43        uint32_t candidate_since;     /** Timestamp in milliseconds of the first sample of the candidate state. */
        44        bool candidate_pressed;       /** Most recently sampled logical state. */
        45        bool candidate_valid;         /** False before the first sample or after a GPIO failure. */
        46        bool stable_pressed;          /** Last accepted state; initially released. */
        47        button_get_ticks_t get_ticks; /** Function to get time source in milliseconds */
        48    };
        49
        50    #ifdef __cplusplus
        51    }
        52    #endif
        53
        54    #endif
    FILE: components/button/cmp_button_types.h
         1    /**
         2     * @file cmp_button_types.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Public button context, configuration, and result types.
         5     */
         6    #ifndef CMP_BUTTON_TYPES_H
         7    #define CMP_BUTTON_TYPES_H
         8
         9    #include <stdint.h>
        10
        11    #ifdef __cplusplus
        12    extern "C" {
        13    #endif
        14
        15    /** @brief Caller-owned context, defined in cmp_button_config.h. */
        16    typedef struct button button_t;
        17
        18    /** @brief Construction dependencies, defined in cmp_button_config.h. */
        19    typedef struct button_config button_config_t;
        20
        21    /**
        22     * @brief Return monotonic system ticks in milliseconds, modulo 2^32.
        23     * @details Supplied by the integration and called in normal execution context.
        24     * Must remain callable throughout the component lifetime and return promptly.
        25     * Synchronization with any interrupt-updated clock belongs to the provider.
        26     */
        27    typedef uint32_t (*button_get_ticks_t)(void);
        28
        29    /** @brief Component results, independent of HAL and SDK status types. */
        30    typedef enum {
        31        BUTTON_STATUS_OK = 0,           /** Operation completed successfully. */
        32        BUTTON_STATUS_INVALID_ARGUMENT, /** A required pointer is NULL. */
        33        BUTTON_STATUS_NOT_INITIALIZED,  /** Context or GPIO is unavailable. */
        34        BUTTON_STATUS_NOT_SUPPORTED,    /** GPIO cannot be read as an input. */
        35        BUTTON_STATUS_GPIO_ERROR        /** GPIO read failed. */
        36    } button_status_t;
        37
        38    #ifdef __cplusplus
        39    }
        40    #endif
        41
        42    #endif
    FILE: components/sht31d/cmp_sht31d.c
         1    #include "cmp_sht31d.h"
         2    #include "cmp_sht31d_config.h"
         3
         4    #include <stddef.h>
         5
         6    static sht31d_status_t cmp_sht31d_translate_status(hal_status_t status) {
         7        switch (status) {
         8            case HAL_STATUS_OK:
         9                return SHT31D_STATUS_OK;
        10            case HAL_STATUS_INVALID_ARGUMENT:
        11                return SHT31D_STATUS_INVALID_ARGUMENT;
        12            case HAL_STATUS_NOT_INITIALIZED:
        13                return SHT31D_STATUS_NOT_INITIALIZED;
        14            case HAL_STATUS_NOT_SUPPORTED:
        15                return SHT31D_STATUS_NOT_SUPPORTED;
        16            case HAL_STATUS_TIMEOUT:
        17                return SHT31D_STATUS_TIMEOUT;
        18            default:
        19                return SHT31D_STATUS_COMMUNICATION_ERROR;
        20        }
        21    }
        22
        23    static sht31d_status_t cmp_sht31d_require_initialized(const sht31d_t *sensor) {
        24        if (sensor == NULL) {
        25            return SHT31D_STATUS_INVALID_ARGUMENT;
        26        }
        27        return sensor->initialized ? SHT31D_STATUS_OK : SHT31D_STATUS_NOT_INITIALIZED;
        28    }
        29
        30    static uint8_t cmp_sht31d_word_crc(const uint8_t *bytes) {
        31        uint8_t crc = 0xFF;
        32        for (uint32_t i = 0; i < 2; ++i) {
        33            crc ^= bytes[i];
        34            for (uint32_t bit = 0; bit < 8; ++bit) {
        35                crc = (crc & 0x80U) != 0 ? (uint8_t)(((uint32_t)crc << 1) ^ 0x31U) : (uint8_t)((uint32_t)crc << 1);
        36            }
        37        }
        38        return crc;
        39    }
        40
        41    sht31d_status_t cmp_sht31d_init(sht31d_t *sensor, const sht31d_config_t *config) {
        42        if (sensor == NULL || config == NULL || config->i2c == NULL ||
        43            (config->address != 0x44 && config->address != 0x45)) {
        44            return SHT31D_STATUS_INVALID_ARGUMENT;
        45        }
        46        *sensor = (sht31d_t){.i2c = config->i2c, .address = config->address};
        47
        48        // Keep the internal heater off for ambient temperature/humidity acquisition.
        49        const uint8_t command[] = {0x30, 0x66};
        50        sht31d_status_t status = cmp_sht31d_translate_status(hal_i2c_write(sensor->i2c, sensor->address, command, (uint32_t)sizeof(command)));
        51        if (status == SHT31D_STATUS_OK) {
        52            sensor->initialized = true;
        53        }
        54        return status;
        55    }
        56
        57    sht31d_status_t cmp_sht31d_start_measurement(sht31d_t *sensor) {
        58        sht31d_status_t status = cmp_sht31d_require_initialized(sensor);
        59        if (status != SHT31D_STATUS_OK) {
        60            return status;
        61        }
        62        if (sensor->measurement_pending) {
        63            return SHT31D_STATUS_INVALID_STATE;
        64        }
        65
        66        // Single shot, high repeatability, no clock stretching. Conversion is external
        67        // to this transfer; the caller waits before requesting the six result bytes.
        68        const uint8_t command[] = {0x24, 0x00};
        69        status = cmp_sht31d_translate_status(hal_i2c_write(sensor->i2c, sensor->address, command, (uint32_t)sizeof(command)));
        70        if (status == SHT31D_STATUS_OK) {
        71            sensor->measurement_pending = true;
        72        }
        73        return status;
        74    }
        75
        76    sht31d_status_t cmp_sht31d_read_measurement(sht31d_t *sensor, sht31d_sample_t *sample) {
        77        if (sample == NULL) {
        78            return SHT31D_STATUS_INVALID_ARGUMENT;
        79        }
        80        sht31d_status_t status = cmp_sht31d_require_initialized(sensor);
        81        if (status != SHT31D_STATUS_OK) {
        82            return status;
        83        }
        84        if (!sensor->measurement_pending) {
        85            return SHT31D_STATUS_INVALID_STATE;
        86        }
        87
        88        // Partial/failed reads cannot be reused as a valid acquisition. Require a
        89        // fresh command even when the bus fails before completing the transaction.
        90        sensor->measurement_pending = false;
        91        uint8_t bytes[6];
        92        status = cmp_sht31d_translate_status(hal_i2c_read(sensor->i2c, sensor->address, bytes, (uint32_t)sizeof(bytes)));
        93        if (status != SHT31D_STATUS_OK) {
        94            return status;
        95        }
        96        if (cmp_sht31d_word_crc(bytes) != bytes[2] || cmp_sht31d_word_crc(bytes + 3) != bytes[5]) {
        97            return SHT31D_STATUS_CRC_ERROR;
        98        }
        99
       100        uint32_t raw_temperature = ((uint32_t)bytes[0] << 8) | bytes[1];
       101        uint32_t raw_humidity = ((uint32_t)bytes[3] << 8) | bytes[4];
       102
       103        // Datasheet scale: T = -45 + 175 * raw / 65535; RH = 100 * raw / 65535.
       104        // Widen before multiplication and round once to millidegrees/millipercent.
       105        const sht31d_sample_t measurement = {
       106            .temperature = -45000 + (int32_t)((UINT64_C(175000) * raw_temperature + 32767U) / 65535U),
       107            .humidity = (uint32_t)((UINT64_C(100000) * raw_humidity + 32767U) / 65535U),
       108        };
       109        *sample = measurement;
       110        return SHT31D_STATUS_OK;
       111    }
    FILE: components/led/cmp_led.c
         1    /**
         2     * @file cmp_led.c
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief GPIO-backed logical LED control.
         5     */
         6    #include "cmp_led_config.h"
         7
         8    #include <stddef.h>
         9
        10    static led_status_t cmp_led_translate_status(hal_status_t status) {
        11        switch (status) {
        12            case HAL_STATUS_OK:
        13                return LED_STATUS_OK;
        14            case HAL_STATUS_INVALID_ARGUMENT:
        15                return LED_STATUS_INVALID_ARGUMENT;
        16            case HAL_STATUS_NOT_INITIALIZED:
        17                return LED_STATUS_NOT_INITIALIZED;
        18            case HAL_STATUS_NOT_SUPPORTED:
        19                return LED_STATUS_NOT_SUPPORTED;
        20            default:
        21                return LED_STATUS_GPIO_ERROR;
        22        }
        23    }
        24
        25    led_status_t cmp_led_init(led_t *led, const led_config_t *config) {
        26        if (led == NULL || config == NULL || config->gpio == NULL) {
        27            return LED_STATUS_INVALID_ARGUMENT;
        28        }
        29        *led = (led_t){.gpio = config->gpio, .active_level = config->active_level};
        30
        31        led_status_t status = cmp_led_translate_status(hal_gpio_write(led->gpio, !led->active_level));
        32        if (status == LED_STATUS_OK) {
        33            led->initialized = true;
        34        }
        35        return status;
        36    }
        37
        38    led_status_t cmp_led_set(const led_t *led, bool on) {
        39        if (led == NULL) {
        40            return LED_STATUS_INVALID_ARGUMENT;
        41        }
        42        if (!led->initialized) {
        43            return LED_STATUS_NOT_INITIALIZED;
        44        }
        45
        46        return cmp_led_translate_status(hal_gpio_write(led->gpio, on ? led->active_level : !led->active_level));
        47    }
    FILE: components/telemetry/cmp_telemetry.c
         1    /**
         2     * @file cmp_telemetry.c
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Bounded integer record formatting and synchronous UART transmission.
         5     */
         6    #include "cmp_telemetry_config.h"
         7
         8    #include <inttypes.h>
         9    #include <stddef.h>
        10    #include <stdio.h>
        11
        12    static telemetry_status_t cmp_telemetry_translate_status(hal_status_t status) {
        13        switch (status) {
        14            case HAL_STATUS_OK:
        15                return TELEMETRY_STATUS_OK;
        16            case HAL_STATUS_INVALID_ARGUMENT:
        17                return TELEMETRY_STATUS_INVALID_ARGUMENT;
        18            case HAL_STATUS_NOT_INITIALIZED:
        19                return TELEMETRY_STATUS_NOT_INITIALIZED;
        20            case HAL_STATUS_NOT_SUPPORTED:
        21                return TELEMETRY_STATUS_NOT_SUPPORTED;
        22            case HAL_STATUS_TIMEOUT:
        23                return TELEMETRY_STATUS_TIMEOUT;
        24            default:
        25                return TELEMETRY_STATUS_COMMUNICATION_ERROR;
        26        }
        27    }
        28
        29    static telemetry_status_t cmp_telemetry_require_initialized(const telemetry_t *telemetry, uint32_t *transmitted_count) {
        30        if (transmitted_count != NULL) {
        31            *transmitted_count = 0;
        32        }
        33        if (telemetry == NULL || transmitted_count == NULL) {
        34            return TELEMETRY_STATUS_INVALID_ARGUMENT;
        35        }
        36        return telemetry->initialized ? TELEMETRY_STATUS_OK : TELEMETRY_STATUS_NOT_INITIALIZED;
        37    }
        38
        39    static bool cmp_telemetry_valid_source(const char *source) {
        40        if (source == NULL || source[0] == '\0') {
        41            return false;
        42        }
        43        for (uint32_t i = 0; i <= TELEMETRY_SOURCE_MAX_LENGTH; ++i) {
        44            unsigned char ch = (unsigned char)source[i];
        45            if (ch == '\0') {
        46                return true;
        47            }
        48            if (i == TELEMETRY_SOURCE_MAX_LENGTH ||
        49                !((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
        50                  (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '.')) {
        51                return false;
        52            }
        53        }
        54        return false;
        55    }
        56
        57    static telemetry_status_t cmp_telemetry_send_record(const telemetry_t *telemetry, const char *record, int length, uint32_t capacity, uint32_t *transmitted_count) {
        58        if (length < 0 || (uint32_t)length >= capacity) {
        59            return TELEMETRY_STATUS_FORMAT_ERROR;
        60        }
        61        return cmp_telemetry_translate_status(hal_uart_write(telemetry->uart, (const uint8_t *)record, (uint32_t)length, transmitted_count));
        62    }
        63
        64    telemetry_status_t cmp_telemetry_init(telemetry_t *telemetry, const telemetry_config_t *config) {
        65        if (telemetry == NULL || config == NULL || config->uart == NULL) {
        66            return TELEMETRY_STATUS_INVALID_ARGUMENT;
        67        }
        68        *telemetry = (telemetry_t){.uart = config->uart};
        69
        70        uint32_t transmitted;
        71        telemetry_status_t status = cmp_telemetry_translate_status(hal_uart_write(telemetry->uart, NULL, 0, &transmitted));
        72        if (status == TELEMETRY_STATUS_OK) {
        73            telemetry->initialized = true;
        74        }
        75        return status;
        76    }
        77
        78    telemetry_status_t cmp_telemetry_send_sample(const telemetry_t *telemetry, const telemetry_sample_t *sample, uint32_t *transmitted_count) {
        79        telemetry_status_t status = cmp_telemetry_require_initialized(telemetry, transmitted_count);
        80        if (sample == NULL) {
        81            return TELEMETRY_STATUS_INVALID_ARGUMENT;
        82        }
        83        if (status != TELEMETRY_STATUS_OK) {
        84            return status;
        85        }
        86
        87        char record[128] = {0};
        88        int length = snprintf(record, sizeof(record), "SAMPLE;temperature_mC=%" PRId32 ";humidity_mpercent=%" PRIu32 "\r\n", sample->temperature, sample->humidity);
        89        return cmp_telemetry_send_record(telemetry, record, length, (uint32_t)sizeof(record), transmitted_count);
        90    }
        91
        92    telemetry_status_t cmp_telemetry_send_error(const telemetry_t *telemetry, const char *source, int32_t code, uint32_t *transmitted_count) {
        93        telemetry_status_t status = cmp_telemetry_require_initialized(telemetry, transmitted_count);
        94        if (!cmp_telemetry_valid_source(source)) {
        95            return TELEMETRY_STATUS_INVALID_ARGUMENT;
        96        }
        97        if (status != TELEMETRY_STATUS_OK) {
        98            return status;
        99        }
       100
       101        char record[TELEMETRY_SOURCE_MAX_LENGTH + 64] = {0};
       102        int32_t length = snprintf(record, sizeof(record), "ERROR;source=%s;code=%" PRId32 "\r\n", source, code);
       103        return cmp_telemetry_send_record(telemetry, record, length, (uint32_t)sizeof(record), transmitted_count);
       104    }
    ✓ • 0ms

## Assistant

A separação entre aplicação, componentes, HAL, porte e BSP já aparece no código atual; o trecho “Estado desta árvore” da skill está desatualizado. Encontrei duas diferenças concretas em relação ao PDF: a HAL fornece notificações periódicas em vez de consulta ao tempo decorrido, e a interface I2C não oferece a transação combinada prevista. Vou conferir o comportamento da prova de conceito e as evidências de validação.

## Activity

    $ for f in app/gui/render.c app/gui/app_gui.c app/status_led/app_status_led.c app/telemetry/app_telemetry.c; do echo "FILE: $f"; nl -ba "$f"; done
    FILE: app/gui/render.c
         1    /**
         2     * @file render.c
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Draws reading cards and acquisition diagnostics in the framebuffer.
         5     */
         6    #include "render.h"
         7    #include "bitmaps.h"
         8
         9    #include <stdio.h>
        10
        11    #define DRAW_TRY(operation)                    \
        12        do {                                       \
        13            ssd1306_status_t result = (operation); \
        14            if (result != SSD1306_STATUS_OK) {     \
        15                return result;                     \
        16            }                                      \
        17        } while (0)
        18
        19    static ssd1306_status_t draw_right_text(ssd1306_t *display, uint32_t left, uint32_t right, uint32_t y, const char *text, ssd1306_font_t font) {
        20        uint32_t width = 0;
        21        uint32_t height = 0;
        22        DRAW_TRY(cmp_ssd1306_measure_text(text, font, &width, &height));
        23        if (width > right - left) {
        24            return SSD1306_STATUS_INVALID_ARGUMENT;
        25        }
        26        return cmp_ssd1306_draw_text(display, right - width, y, text, font);
        27    }
        28
        29    static ssd1306_status_t draw_header(ssd1306_t *display, const gui_frame_t *frame, const char *title) {
        30        DRAW_TRY(cmp_ssd1306_draw_text(display, 6, 3, title, SSD1306_FONT_SMALL));
        31        if (frame->has_result && frame->acquisition.status != SHT31D_STATUS_OK) {
        32            DRAW_TRY(cmp_ssd1306_draw_text(display, 92, 3, "!", SSD1306_FONT_SMALL));
        33        }
        34        for (uint32_t page = 0; page < 2; ++page) {
        35            uint32_t x = 108U + 11U * page;
        36            if ((uint32_t)frame->screen == page) {
        37                DRAW_TRY(cmp_ssd1306_fill_rect(display, x, 4, 7, 3, true));
        38            } else {
        39                DRAW_TRY(cmp_ssd1306_fill_rect(display, x + 2, 5, 3, 1, true));
        40            }
        41        }
        42        return SSD1306_STATUS_OK;
        43    }
        44
        45    static ssd1306_status_t draw_readings(ssd1306_t *display, const gui_frame_t *frame) {
        46        char temperature[12] = "--,-";
        47        char humidity[12] = "--,-";
        48        if (frame->acquisition.has_sample) {
        49            if (frame->acquisition.sample.temperature < -45000 || frame->acquisition.sample.temperature > 130000 ||
        50                frame->acquisition.sample.humidity > 100000) {
        51                return SSD1306_STATUS_INVALID_ARGUMENT;
        52            }
        53            // Round to tenths for presentation; keep the sample at its original resolution.
        54            int64_t value = frame->acquisition.sample.temperature;
        55            uint32_t magnitude = (uint32_t)(((value < 0 ? -value : value) + 50) / 100);
        56            uint32_t rh = (frame->acquisition.sample.humidity + 50U) / 100U;
        57            snprintf(temperature, sizeof(temperature), "%s%lu,%lu", value < 0 && magnitude != 0 ? "-" : "", (uint32_t)(magnitude / 10U), (uint32_t)(magnitude % 10U));
        58            snprintf(humidity, sizeof(humidity), "%lu,%lu", (uint32_t)(rh / 10U), (uint32_t)(rh % 10U));
        59        }
        60        DRAW_TRY(draw_header(display, frame, "TEMPERATURA"));
        61        DRAW_TRY(cmp_ssd1306_draw_bitmap(display, 6, 22, get_thermometer_bitmap()));
        62        DRAW_TRY(draw_right_text(display, 20, 103, 14, temperature, SSD1306_FONT_LARGE));
        63        DRAW_TRY(cmp_ssd1306_draw_text(display, 108, 24, "°C", SSD1306_FONT_MEDIUM));
        64        DRAW_TRY(cmp_ssd1306_draw_line(display, 6, 46, 121, 46, true));
        65        DRAW_TRY(cmp_ssd1306_draw_bitmap(display, 6, 50, get_droplet_bitmap()));
        66        DRAW_TRY(cmp_ssd1306_draw_text(display, 21, 53, "UMIDADE", SSD1306_FONT_SMALL));
        67        DRAW_TRY(draw_right_text(display, 65, 111, 48, humidity, SSD1306_FONT_MEDIUM));
        68        return cmp_ssd1306_draw_text(display, 116, 48, "%", SSD1306_FONT_MEDIUM);
        69    }
        70
        71    static const char *acquisition_label(const gui_frame_t *frame) {
        72        if (!frame->has_result) {
        73            return "AGUARDANDO";
        74        }
        75        switch (frame->acquisition.status) {
        76            case SHT31D_STATUS_OK:
        77                return "OK";
        78            case SHT31D_STATUS_TIMEOUT:
        79                return "TIMEOUT";
        80            case SHT31D_STATUS_CRC_ERROR:
        81                return "ERRO CRC";
        82            case SHT31D_STATUS_COMMUNICATION_ERROR:
        83                return "ERRO I2C";
        84            default:
        85                return "INDISPONIVEL";
        86        }
        87    }
        88
        89    static ssd1306_status_t draw_diagnostics(ssd1306_t *display, const gui_frame_t *frame) {
        90        uint64_t seconds = frame->uptime_seconds;
        91        uint64_t days = seconds / 86400U;
        92        char uptime_text[24];
        93        if (days == 0) {
        94            snprintf(uptime_text, sizeof(uptime_text), "%02lu:%02lu:%02lu", (uint32_t)(seconds / 3600U), (uint32_t)(seconds / 60U % 60U), (uint32_t)(seconds % 60U));
        95        } else {
        96            uint32_t shown_days = days > 9999999U ? 9999999U : (uint32_t)days;
        97            snprintf(uptime_text, sizeof(uptime_text), "%lu%s %02lu:%02lu:%02lu", (uint32_t)shown_days, days > 9999999U ? "+" : "d", (uint32_t)(seconds / 3600U % 24U), (uint32_t)(seconds / 60U % 60U), (uint32_t)(seconds % 60U));
        98        }
        99        DRAW_TRY(draw_header(display, frame, "DIAGNOSTICO"));
       100        DRAW_TRY(cmp_ssd1306_draw_text(display, 6, 18, "ULTIMA AQUISICAO", SSD1306_FONT_SMALL));
       101        DRAW_TRY(cmp_ssd1306_draw_text(display, 6, 28, acquisition_label(frame), SSD1306_FONT_SMALL));
       102        DRAW_TRY(cmp_ssd1306_draw_line(display, 6, 40, 121, 40, true));
       103        DRAW_TRY(cmp_ssd1306_draw_text(display, 6, 45, "TEMPO ATIVO", SSD1306_FONT_SMALL));
       104        return cmp_ssd1306_draw_text(display, 6, 55, uptime_text, SSD1306_FONT_SMALL);
       105    }
       106
       107    ssd1306_status_t render_frame(ssd1306_t *display, const gui_frame_t *frame) {
       108        DRAW_TRY(cmp_ssd1306_clear(display));
       109        return frame->screen == SCREEN_READINGS ? draw_readings(display, frame)
       110                                                : draw_diagnostics(display, frame);
       111    }
    FILE: app/gui/app_gui.c
         1    /**
         2     * @file app_gui.c
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Coordinates screen selection, rendering, and display transfers.
         5     */
         6    #include "app_gui.h"
         7
         8    #include <stddef.h>
         9
        10    #include "acquisition/app_acquisition.h"
        11    #include "render.h"
        12    #include "status_led/app_status_led.h"
        13    #include "telemetry/app_telemetry.h"
        14
        15    #define RETRY_INTERVAL 1000U /** Delay after a failed display operation, in milliseconds. */
        16
        17    static uint32_t (*get_ticks)(void) = NULL;    /** Time source supplied by the integration. */
        18    static gui_screen_t screen = SCREEN_READINGS; /** Requested screen. */
        19    static gui_frame_t last_frame;                /** Snapshot currently drawn in the framebuffer. */
        20    static uint64_t uptime = 0;                   /** Accumulated time since startup, in milliseconds. */
        21    static uint32_t last_ticks = 0;               /** Last observed clock value. */
        22    static uint32_t retry_since = 0;              /** Time after the latest failed operation returned. */
        23    static bool pending = false;                  /** Pages remain in the current frame. */
        24    static bool rendered = false;                 /** A complete snapshot has been drawn in RAM. */
        25    static bool waiting_retry = false;            /** The next display attempt is delayed. */
        26    static bool initialized = false;              /** True after successful task initialization. */
        27
        28    /**
        29     * @brief Check whether the requested snapshot differs from the rendered one.
        30     *
        31     * @param[in] frame Requested snapshot.
        32     *
        33     * @retval true Content must be redrawn.
        34     * @retval false The current framebuffer already represents the snapshot.
        35     */
        36    static bool frame_changed(const gui_frame_t *frame) {
        37        if (!rendered || last_frame.screen != frame->screen || last_frame.has_result != frame->has_result) {
        38            return true;
        39        }
        40        if (frame->has_result &&
        41            (last_frame.acquisition.status != frame->acquisition.status ||
        42             last_frame.acquisition.has_sample != frame->acquisition.has_sample ||
        43             (frame->acquisition.has_sample &&
        44              (last_frame.acquisition.sample.temperature != frame->acquisition.sample.temperature ||
        45               last_frame.acquisition.sample.humidity != frame->acquisition.sample.humidity)))) {
        46            return true;
        47        }
        48        return frame->screen == SCREEN_DIAGNOSTICS && last_frame.uptime_seconds != frame->uptime_seconds;
        49    }
        50
        51    bool app_gui_init(uint32_t (*get_ticks_function)(void)) {
        52        if (get_ticks_function == NULL) {
        53            return false;
        54        }
        55        get_ticks = get_ticks_function;
        56        last_ticks = get_ticks();
        57        uptime = last_ticks;
        58        screen = SCREEN_READINGS;
        59        last_frame = (gui_frame_t){0};
        60        retry_since = 0;
        61        pending = false;
        62        rendered = false;
        63        waiting_retry = false;
        64        initialized = true;
        65        return true;
        66    }
        67
        68    void app_gui_toggle_screen(void) {
        69        if (initialized) {
        70            screen = screen == SCREEN_READINGS ? SCREEN_DIAGNOSTICS : SCREEN_READINGS;
        71        }
        72    }
        73
        74    void app_gui_run(ssd1306_t *display) {
        75        if (display == NULL || !initialized) {
        76            return;
        77        }
        78
        79        const uint32_t now = get_ticks();
        80        uptime += (uint32_t)(now - last_ticks);
        81        last_ticks = now;
        82        if (waiting_retry && (uint32_t)(now - retry_since) < RETRY_INTERVAL) {
        83            return;
        84        }
        85
        86        ssd1306_status_t status = SSD1306_STATUS_OK;
        87        if (!pending) {
        88            gui_frame_t frame = {.screen = screen, .uptime_seconds = uptime / 1000U};
        89            frame.has_result = app_acquisition_get_result(&frame.acquisition);
        90            if (!frame_changed(&frame)) {
        91                return;
        92            }
        93            status = render_frame(display, &frame);
        94            if (status == SSD1306_STATUS_OK) {
        95                last_frame = frame;
        96                rendered = true;
        97                pending = true;
        98            } else {
        99                rendered = false;
       100            }
       101        }
       102        if (status == SSD1306_STATUS_OK) {
       103            status = cmp_ssd1306_update(display, &pending);
       104        }
       105        waiting_retry = status != SSD1306_STATUS_OK;
       106        if (waiting_retry) {
       107            retry_since = get_ticks();
       108            app_telemetry_report_error(APP_TELEMETRY_SOURCE_DISPLAY, (int32_t)status);
       109            if (status == SSD1306_STATUS_TIMEOUT || status == SSD1306_STATUS_COMMUNICATION_ERROR) {
       110                app_status_led_set_i2c_error(APP_STATUS_LED_I2C_DISPLAY, true);
       111            }
       112        } else {
       113            app_status_led_set_i2c_error(APP_STATUS_LED_I2C_DISPLAY, false);
       114        }
       115    }
    FILE: app/status_led/app_status_led.c
         1    /**
         2     * @file app_status_led.c
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Indicates startup, normal operation, and I2C communication errors.
         5     */
         6    #include "app_status_led.h"
         7
         8    #include <stddef.h>
         9
        10    #define APP_STATUS_LED_BLINK_INTERVAL 500U /** Time between blink transitions, in milliseconds. */
        11
        12    static uint32_t (*get_ticks)(void) = NULL; /** Time source supplied by the integration. */
        13    static uint8_t i2c_errors = 0;             /** Independent sensor and display error bits. */
        14    static bool operation_ready = false;       /** True after startup is complete. */
        15    static uint32_t last_transition = 0;       /** Timestamp of the current blink phase. */
        16    static int8_t led_state = -1;              /** Last requested led state; -1 before the first command. */
        17    static bool blink_active = false;          /** True after starting the current blink sequence. */
        18    static bool initialized = false;           /** True after successful task initialization. */
        19
        20    bool app_status_led_init(uint32_t (*get_ticks_function)(void)) {
        21        if (get_ticks_function == NULL) {
        22            return false;
        23        }
        24
        25        get_ticks = get_ticks_function;
        26        last_transition = 0;
        27        i2c_errors = 0;
        28        operation_ready = false;
        29        led_state = -1;
        30        blink_active = false;
        31        initialized = true;
        32        return true;
        33    }
        34
        35    void app_status_led_set_ready(bool ready) {
        36        if (initialized) {
        37            operation_ready = ready;
        38        }
        39    }
        40
        41    void app_status_led_set_i2c_error(app_status_led_i2c_source_t source, bool active) {
        42        if (!initialized || (source != APP_STATUS_LED_I2C_SENSOR && source != APP_STATUS_LED_I2C_DISPLAY)) {
        43            return;
        44        }
        45
        46        const uint8_t bit = (uint8_t)(1U << (uint8_t)source);
        47        if (active) {
        48            i2c_errors |= bit;
        49        } else {
        50            i2c_errors &= (uint8_t)~bit;
        51        }
        52    }
        53
        54    void app_status_led_run(const led_t *led) {
        55        if (led == NULL || !initialized) {
        56            return;
        57        }
        58
        59        bool state_to_set = operation_ready;
        60
        61        if (i2c_errors != 0) {
        62            const uint32_t now = get_ticks();
        63            state_to_set = led_state == 1;
        64
        65            if (!blink_active) {
        66                blink_active = true;
        67                state_to_set = false;
        68                last_transition = now;
        69            } else if ((uint32_t)(now - last_transition) >= APP_STATUS_LED_BLINK_INTERVAL) {
        70                state_to_set = !state_to_set;
        71                last_transition = now;
        72            }
        73        } else {
        74            blink_active = false;
        75        }
        76
        77        if (state_to_set != led_state) {
        78            cmp_led_set(led, state_to_set);
        79            led_state = state_to_set;
        80        }
        81    }
    FILE: app/telemetry/app_telemetry.c
         1    /**
         2     * @file app_telemetry.c
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Buffers application records and handles UART retry and discard policies.
         5     */
         6    #include "app_telemetry.h"
         7    #include "app_telemetry_private.h"
         8
         9    #include <stddef.h>
        10
        11    static uint32_t (*get_ticks)(void) = NULL;     /** Time source supplied by the integration. */
        12    static record_t records[QUEUE_CAPACITY] = {0}; /** Records queue */
        13    static uint32_t head = 0;                      /** Oldest queued record. */
        14    static uint32_t count = 0;                     /** Number of queued records. */
        15    static uint32_t retry_since = 0;               /** Time after the latest failed send returned. */
        16    static bool waiting_retry = false;             /** The next UART attempt is delayed. */
        17    static bool initialized = false;               /** True after successful task initialization. */
        18
        19    static bool enqueue(const record_t *record) {
        20        if (!initialized) {
        21            return false;
        22        }
        23        if (count == QUEUE_CAPACITY) {
        24            return false;
        25        }
        26        records[(head + count) % QUEUE_CAPACITY] = *record;
        27        ++count;
        28        return true;
        29    }
        30
        31    static void remove_head(void) {
        32        head = (head + 1U) % QUEUE_CAPACITY;
        33        --count;
        34    }
        35
        36    bool app_telemetry_init(uint32_t (*get_ticks_function)(void)) {
        37        if (get_ticks_function == NULL) {
        38            return false;
        39        }
        40        get_ticks = get_ticks_function;
        41        head = 0;
        42        count = 0;
        43        retry_since = 0;
        44        waiting_retry = false;
        45        initialized = true;
        46        return true;
        47    }
        48
        49    bool app_telemetry_report_sample(const telemetry_sample_t *sample) {
        50        if (sample == NULL) {
        51            return false;
        52        }
        53        const record_t record = {.kind = RECORD_SAMPLE, .sample = *sample};
        54        return enqueue(&record);
        55    }
        56
        57    bool app_telemetry_report_error(telemetry_source_t source, int32_t code) {
        58        if (source != APP_TELEMETRY_SOURCE_SENSOR && source != APP_TELEMETRY_SOURCE_DISPLAY) {
        59            return false;
        60        }
        61        const record_t record = {.kind = RECORD_ERROR, .source = source, .code = code};
        62        return enqueue(&record);
        63    }
        64
        65    void app_telemetry_run(const telemetry_t *telemetry) {
        66        if (telemetry == NULL || !initialized || count == 0) {
        67            return;
        68        }
        69        if (waiting_retry && (uint32_t)(get_ticks() - retry_since) < RETRY_INTERVAL) {
        70            return;
        71        }
        72
        73        const record_t *record = &records[head];
        74        uint32_t transmitted = 0;
        75        telemetry_status_t status;
        76        if (record->kind == RECORD_SAMPLE) {
        77            status = cmp_telemetry_send_sample(telemetry, &record->sample, &transmitted);
        78        } else {
        79            const char *source = record->source == APP_TELEMETRY_SOURCE_SENSOR ? "SHT31D" : "SSD1306";
        80            status = cmp_telemetry_send_error(telemetry, source, record->code, &transmitted);
        81        }
        82        if (status == TELEMETRY_STATUS_OK) {
        83            remove_head();
        84            waiting_retry = false;
        85            return;
        86        }
        87
        88        // Only retry transient failures that accepted no prefix of the record.
        89        if (transmitted != 0 || (status != TELEMETRY_STATUS_TIMEOUT && status != TELEMETRY_STATUS_COMMUNICATION_ERROR)) {
        90            remove_head();
        91        }
        92        retry_since = get_ticks();
        93        waiting_retry = true;
        94    }
    ✓ • 0ms

## Activity

    $ for f in hal/ports/esp32-wroom-32/README.md hal/ports/esp32-wroom-32/timer/hal_timer.c hal/ports/esp32-wroom-32/i2c/hal_i2c.c; do echo "FILE: $f"; nl -ba "$f"; done
    FILE: hal/ports/esp32-wroom-32/README.md
         1    # esp32-wroom-32
    FILE: hal/ports/esp32-wroom-32/timer/hal_timer.c
         1    #include "timer/hal_timer.h"
         2    #include "hal_esp32_private.h"
         3    #include "hal_memory.h"
         4
         5    #include <stdatomic.h>
         6    #include <stdbool.h>
         7    #include <stddef.h>
         8    #include <stdint.h>
         9
        10    #include "driver/gptimer.h"
        11    #include "esp_attr.h"
        12    #include "esp_memory_utils.h"
        13    #include "sdkconfig.h"
        14
        15    /** @brief Four logical identifiers backed by SDK-allocated dedicated timers. */
        16    enum { TIMER_INSTANCE_COUNT = 4,
        17           TIMER_RESOLUTION_HZ = 1000000 };
        18
        19    /** @brief Persistent periodic hardware timer and its ISR callback. */
        20    struct hal_timer {
        21        gptimer_handle_t device;       /** Dedicated SDK hardware timer. */
        22        hal_timer_callback_t callback; /** Application callback invoked from the timer ISR. */
        23        atomic_bool running;           /** Publishes application initialization to the ISR. */
        24        bool initialized;              /** True after complete construction, with counting stopped. */
        25    };
        26
        27    static bool claimed_timers[TIMER_INSTANCE_COUNT];
        28
        29    _Static_assert(sizeof(hal_timer_t) <= UINT32_MAX, "Timer object exceeds arena size range");
        30    _Static_assert(_Alignof(hal_timer_t) <= _Alignof(max_align_t), "Timer object requires extended alignment");
        31    _Static_assert(ATOMIC_BOOL_LOCK_FREE == 2, "Timer ISR requires lock-free atomic bool");
        32
        33    static bool IRAM_ATTR on_alarm(gptimer_handle_t device, const gptimer_alarm_event_data_t *event, void *context) {
        34        (void)device;
        35        (void)event;
        36        hal_timer_t *timer = context;
        37        if (atomic_load_explicit(&timer->running, memory_order_acquire)) {
        38            timer->callback();
        39        }
        40        return false;
        41    }
        42
        43    static hal_status_t configure_timer(hal_timer_t *object, uint32_t period) {
        44        const gptimer_config_t config = {
        45            .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        46            .direction = GPTIMER_COUNT_UP,
        47            .resolution_hz = TIMER_RESOLUTION_HZ,
        48        };
        49        esp_err_t error = gptimer_new_timer(&config, &object->device);
        50        if (error == ESP_OK) {
        51            uint32_t resolution = 0;
        52            error = gptimer_get_resolution(object->device, &resolution);
        53            if (error == ESP_OK && resolution != TIMER_RESOLUTION_HZ) {
        54                gptimer_del_timer(object->device);
        55                return HAL_STATUS_NOT_SUPPORTED;
        56            }
        57        }
        58        if (error == ESP_OK) {
        59            const gptimer_alarm_config_t alarm = {
        60                .alarm_count = (uint64_t)period * 1000,
        61                .reload_count = 0,
        62                .flags.auto_reload_on_alarm = true,
        63            };
        64            error = gptimer_set_alarm_action(object->device, &alarm);
        65        }
        66        if (error == ESP_OK) {
        67            const gptimer_event_callbacks_t callbacks = {.on_alarm = on_alarm};
        68            error = gptimer_register_event_callbacks(object->device, &callbacks, object);
        69        }
        70        if (error == ESP_OK) {
        71            error = gptimer_set_raw_count(object->device, 0);
        72        }
        73        if (error == ESP_OK) {
        74            // Enabling installs the interrupt and acquires the SDK clock lock now.
        75            error = gptimer_enable(object->device);
        76        }
        77        if (error != ESP_OK) {
        78            if (object->device != NULL) {
        79                gptimer_del_timer(object->device);
        80            }
        81            if (error == ESP_ERR_NO_MEM) {
        82                return HAL_STATUS_NO_MEMORY;
        83            }
        84            return error == ESP_ERR_NOT_FOUND ? HAL_STATUS_RESOURCE_BUSY : HAL_STATUS_ERROR;
        85        }
        86        return HAL_STATUS_OK;
        87    }
        88
        89    hal_status_t hal_timer_config(hal_timer_t **timer, const hal_timer_config_t *config) {
        90        if (timer == NULL) {
        91            return HAL_STATUS_INVALID_ARGUMENT;
        92        }
        93        *timer = NULL;
        94        if (config == NULL || config->callback == NULL || config->period == 0 || config->instance >= TIMER_INSTANCE_COUNT) {
        95            return HAL_STATUS_INVALID_ARGUMENT;
        96        }
        97    #if CONFIG_GPTIMER_ISR_CACHE_SAFE
        98        if (!esp_ptr_in_iram(config->callback)) {
        99            return HAL_STATUS_NOT_SUPPORTED;
       100        }
       101    #endif
       102        hal_memory_info_t info;
       103        hal_status_t status = hal_esp32_memory_status(hal_memory_get_info(&info));
       104        if (status != HAL_STATUS_OK) {
       105            return status;
       106        }
       107        if (info.sealed) {
       108            return HAL_STATUS_INVALID_STATE;
       109        }
       110        if (claimed_timers[config->instance]) {
       111            return HAL_STATUS_RESOURCE_BUSY;
       112        }
       113        void *storage = NULL;
       114        status = hal_esp32_memory_status(hal_memory_alloc((uint32_t)sizeof(hal_timer_t), &storage));
       115        if (status != HAL_STATUS_OK) {
       116            return status;
       117        }
       118    #if CONFIG_GPTIMER_ISR_CACHE_SAFE
       119        if (!esp_ptr_internal(storage)) {
       120            return HAL_STATUS_NOT_SUPPORTED;
       121        }
       122    #endif
       123        claimed_timers[config->instance] = true;
       124        hal_timer_t *object = storage;
       125        object->device = NULL;
       126        object->callback = config->callback;
       127        object->initialized = false;
       128        atomic_init(&object->running, false);
       129        status = configure_timer(object, config->period);
       130        if (status != HAL_STATUS_OK) {
       131            return status;
       132        }
       133        object->initialized = true;
       134        *timer = object;
       135        return HAL_STATUS_OK;
       136    }
       137
       138    hal_status_t hal_timer_start(hal_timer_t *timer) {
       139        if (timer == NULL) {
       140            return HAL_STATUS_INVALID_ARGUMENT;
       141        }
       142        if (!timer->initialized) {
       143            return HAL_STATUS_NOT_INITIALIZED;
       144        }
       145        if (atomic_load_explicit(&timer->running, memory_order_acquire)) {
       146            return HAL_STATUS_INVALID_STATE;
       147        }
       148        // Publish application state before the hardware may deliver its first alarm.
       149        atomic_store_explicit(&timer->running, true, memory_order_release);
       150        if (gptimer_start(timer->device) != ESP_OK) {
       151            atomic_store_explicit(&timer->running, false, memory_order_release);
       152            return HAL_STATUS_ERROR;
       153        }
       154        return HAL_STATUS_OK;
       155    }
    FILE: hal/ports/esp32-wroom-32/i2c/hal_i2c.c
         1    #include "i2c/hal_i2c.h"
         2    #include "hal_esp32_private.h"
         3    #include "hal_memory.h"
         4
         5    #include <stdbool.h>
         6    #include <stddef.h>
         7    #include <stdint.h>
         8
         9    #include "driver/gpio.h"
        10    #include "driver/i2c_master.h"
        11
        12    /** @brief Persistent synchronous bus and its reusable addressed endpoint. */
        13    struct hal_i2c {
        14        i2c_master_bus_handle_t bus;    /** SDK controller handle. */
        15        i2c_master_dev_handle_t device; /** Endpoint whose address changes before each transfer. */
        16        int timeout;                    /** Finite SDK timeout in milliseconds. */
        17        bool initialized;               /** True after complete construction. */
        18    };
        19
        20    static bool claimed_controllers[SOC_I2C_NUM];
        21
        22    _Static_assert(sizeof(hal_i2c_t) <= UINT32_MAX, "I2C object exceeds arena size range");
        23    _Static_assert(_Alignof(hal_i2c_t) <= _Alignof(max_align_t), "I2C object requires extended alignment");
        24
        25    static hal_status_t validate_config(const hal_i2c_config_t *config) {
        26        if (config->instance >= SOC_I2C_NUM || config->frequency == 0 || config->timeout == 0 ||
        27            (config->clock_pin.port == config->data_pin.port && config->clock_pin.number == config->data_pin.number)) {
        28            return HAL_STATUS_INVALID_ARGUMENT;
        29        }
        30        hal_status_t status = hal_esp32_pin_validate(config->clock_pin);
        31        if (status != HAL_STATUS_OK) {
        32            return status;
        33        }
        34        status = hal_esp32_pin_validate(config->data_pin);
        35        if (status != HAL_STATUS_OK) {
        36            return status;
        37        }
        38        if (!GPIO_IS_VALID_OUTPUT_GPIO(config->clock_pin.number) || !GPIO_IS_VALID_OUTPUT_GPIO(config->data_pin.number) ||
        39            config->frequency < 10000 || config->frequency > 400000) {
        40            return HAL_STATUS_NOT_SUPPORTED;
        41        }
        42        TickType_t ticks;
        43        return hal_esp32_convert_timeout_to_ticks(config->timeout, &ticks);
        44    }
        45
        46    static hal_status_t transfer_status(esp_err_t status) {
        47        if (status == ESP_OK) {
        48            return HAL_STATUS_OK;
        49        }
        50        return status == ESP_ERR_TIMEOUT ? HAL_STATUS_TIMEOUT : HAL_STATUS_ERROR;
        51    }
        52
        53    static hal_status_t configure_bus(hal_i2c_t *object, const hal_i2c_config_t *config) {
        54        const gpio_config_t pins = {
        55            .pin_bit_mask = (UINT64_C(1) << config->clock_pin.number) | (UINT64_C(1) << config->data_pin.number),
        56            .mode = GPIO_MODE_INPUT,
        57            .pull_up_en = GPIO_PULLUP_DISABLE,
        58            .pull_down_en = GPIO_PULLDOWN_DISABLE,
        59            .intr_type = GPIO_INTR_DISABLE,
        60        };
        61        // Establish digital pads and high latches before the SDK enables open drain.
        62        if (gpio_config(&pins) != ESP_OK || gpio_set_level((gpio_num_t)config->clock_pin.number, 1) != ESP_OK ||
        63            gpio_set_level((gpio_num_t)config->data_pin.number, 1) != ESP_OK) {
        64            return HAL_STATUS_ERROR;
        65        }
        66        const i2c_master_bus_config_t bus_config = {
        67            .i2c_port = config->instance,
        68            .scl_io_num = (gpio_num_t)config->clock_pin.number,
        69            .sda_io_num = (gpio_num_t)config->data_pin.number,
        70            .clk_source = I2C_CLK_SRC_DEFAULT,
        71            .glitch_ignore_cnt = 7,
        72            .flags.enable_internal_pullup = false,
        73        };
        74        esp_err_t error = i2c_new_master_bus(&bus_config, &object->bus);
        75        if (error == ESP_OK) {
        76            const i2c_device_config_t device_config = {
        77                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        78                .device_address = 0x08,
        79                .scl_speed_hz = config->frequency,
        80            };
        81            // One synchronous endpoint serves every address without runtime allocation.
        82            error = i2c_master_bus_add_device(object->bus, &device_config, &object->device);
        83        }
        84        if (error == ESP_OK) {
        85            if (gpio_hold_dis((gpio_num_t)config->clock_pin.number) != ESP_OK ||
        86                gpio_hold_dis((gpio_num_t)config->data_pin.number) != ESP_OK) {
        87                error = ESP_FAIL;
        88            }
        89        }
        90        if (error != ESP_OK) {
        91            // SDK cleanup does not reopen arena storage or relinquish touched pins.
        92            if (object->device != NULL) {
        93                i2c_master_bus_rm_device(object->device);
        94            }
        95            if (object->bus != NULL) {
        96                i2c_del_master_bus(object->bus);
        97            }
        98            if (error == ESP_ERR_NO_MEM) {
        99                return HAL_STATUS_NO_MEMORY;
       100            }
       101            return error == ESP_ERR_NOT_FOUND || error == ESP_ERR_INVALID_STATE ? HAL_STATUS_RESOURCE_BUSY : HAL_STATUS_ERROR;
       102        }
       103        return HAL_STATUS_OK;
       104    }
       105
       106    hal_status_t hal_i2c_config(hal_i2c_t **i2c, const hal_i2c_config_t *config) {
       107        if (i2c == NULL) {
       108            return HAL_STATUS_INVALID_ARGUMENT;
       109        }
       110        *i2c = NULL;
       111        if (config == NULL) {
       112            return HAL_STATUS_INVALID_ARGUMENT;
       113        }
       114        hal_status_t status = validate_config(config);
       115        if (status != HAL_STATUS_OK) {
       116            return status;
       117        }
       118        status = hal_esp32_pin_claim(config->clock_pin);
       119        if (status != HAL_STATUS_OK) {
       120            return status;
       121        }
       122        status = hal_esp32_pin_claim(config->data_pin);
       123        if (status != HAL_STATUS_OK) {
       124            hal_esp32_pin_release(config->clock_pin);
       125            return status;
       126        }
       127        i2c_master_bus_handle_t existing = NULL;
       128        if (claimed_controllers[config->instance] || i2c_master_get_bus_handle(config->instance, &existing) == ESP_OK) {
       129            hal_esp32_pin_release(config->clock_pin);
       130            hal_esp32_pin_release(config->data_pin);
       131            return HAL_STATUS_RESOURCE_BUSY;
       132        }
       133        void *storage = NULL;
       134        status = hal_esp32_memory_status(hal_memory_alloc((uint32_t)sizeof(hal_i2c_t), &storage));
       135        if (status != HAL_STATUS_OK) {
       136            hal_esp32_pin_release(config->clock_pin);
       137            hal_esp32_pin_release(config->data_pin);
       138            return status;
       139        }
       140        claimed_controllers[config->instance] = true;
       141        hal_i2c_t *object = storage;
       142        *object = (hal_i2c_t){.timeout = (int)config->timeout};
       143        status = configure_bus(object, config);
       144        if (status != HAL_STATUS_OK) {
       145            return status;
       146        }
       147        object->initialized = true;
       148        *i2c = object;
       149        return HAL_STATUS_OK;
       150    }
       151
       152    static hal_status_t validate_transfer(hal_i2c_t *i2c, uint8_t address, const void *buffer, uint32_t size) {
       153        if (i2c == NULL || buffer == NULL || size == 0 || address < 0x08 || address > 0x77) {
       154            return HAL_STATUS_INVALID_ARGUMENT;
       155        }
       156        if (!i2c->initialized) {
       157            return HAL_STATUS_NOT_INITIALIZED;
       158        }
       159        return size > UINT16_MAX ? HAL_STATUS_NOT_SUPPORTED : HAL_STATUS_OK;
       160    }
       161
       162    hal_status_t hal_i2c_write(hal_i2c_t *i2c, uint8_t address, const uint8_t *buffer, uint32_t size) {
       163        hal_status_t status = validate_transfer(i2c, address, buffer, size);
       164        if (status != HAL_STATUS_OK) {
       165            return status;
       166        }
       167        esp_err_t error = i2c_master_device_change_address(i2c->device, address, i2c->timeout);
       168        if (error == ESP_OK) {
       169            error = i2c_master_transmit(i2c->device, buffer, size, i2c->timeout);
       170        }
       171        return transfer_status(error);
       172    }
       173
       174    hal_status_t hal_i2c_read(hal_i2c_t *i2c, uint8_t address, uint8_t *buffer, uint32_t size) {
       175        hal_status_t status = validate_transfer(i2c, address, buffer, size);
       176        if (status != HAL_STATUS_OK) {
       177            return status;
       178        }
       179        esp_err_t error = i2c_master_device_change_address(i2c->device, address, i2c->timeout);
       180        if (error == ESP_OK) {
       181            error = i2c_master_receive(i2c->device, buffer, size, i2c->timeout);
       182        }
       183        return transfer_status(error);
       184    }
    ✓ • 0ms

## Activity

    $ command -v cc && cc -std=c11 -fsyntax-only -Iapp -Ibsp -Icomponents/button -Icomponents/led -Icomponents/sht31d -Icomponents/ssd1306 -Icomponents/telemetry -Ihal/interface app/acquisition/app_acquisition.c app/button/app_button.c app/gui/app_gui.c app/gui/bitmaps.c app/gui/render.c app/status_led/app_status_led.c app/telemetry/app_telemetry.c components/button/cmp_button.c components/led/cmp_led.c components/sht31d/cmp_sht31d.c components/ssd1306/cmp_ssd1306.c components/ssd1306/cmp_ssd1306_fonts.c components/telemetry/cmp_telemetry.c bsp/esp32_devkit_v1/bsp.c
    /usr/bin/cc
    app/gui/render.c: In function ‘draw_readings’:
    app/gui/render.c:57:57: warning: format ‘%lu’ expects argument of type ‘long unsigned int’, but argument 5 has type ‘unsigned int’ [-Wformat=]
       57 |         snprintf(temperature, sizeof(temperature), "%s%lu,%lu", value < 0 && magnitude != 0 ? "-" : "", (uint32_t)(magnitude / 10U), (uint32_t)(magnitude % 10U));
          |                                                       ~~^                                               ~~~~~~~~~~~~~~~~~~~~~~~~~~~
          |                                                         |                                               |
          |                                                         long unsigned int                               unsigned int
          |                                                       %u
    app/gui/render.c:57:61: warning: format ‘%lu’ expects argument of type ‘long unsigned int’, but argument 6 has type ‘unsigned int’ [-Wformat=]
       57 |         snprintf(temperature, sizeof(temperature), "%s%lu,%lu", value < 0 && magnitude != 0 ? "-" : "", (uint32_t)(magnitude / 10U), (uint32_t)(magnitude % 10U));
          |                                                           ~~^                                                                        ~~~~~~~~~~~~~~~~~~~~~~~~~~~
          |                                                             |                                                                        |
          |                                                             long unsigned int                                                        unsigned int
          |                                                           %u
    app/gui/render.c:58:49: warning: format ‘%lu’ expects argument of type ‘long unsigned int’, but argument 4 has type ‘unsigned int’ [-Wformat=]
       58 |         snprintf(humidity, sizeof(humidity), "%lu,%lu", (uint32_t)(rh / 10U), (uint32_t)(rh % 10U));
          |                                               ~~^       ~~~~~~~~~~~~~~~~~~~~
          |                                                 |       |
          |                                                 |       unsigned int
          |                                                 long unsigned int
          |                                               %u
    app/gui/render.c:58:53: warning: format ‘%lu’ expects argument of type ‘long unsigned int’, but argument 5 has type ‘unsigned int’ [-Wformat=]
       58 |         snprintf(humidity, sizeof(humidity), "%lu,%lu", (uint32_t)(rh / 10U), (uint32_t)(rh % 10U));
          |                                                   ~~^                         ~~~~~~~~~~~~~~~~~~~~
          |                                                     |                         |
          |                                                     long unsigned int         unsigned int
          |                                                   %u
    app/gui/render.c: In function ‘draw_diagnostics’:
    app/gui/render.c:94:57: warning: format ‘%lu’ expects argument of type ‘long unsigned int’, but argument 4 has type ‘unsigned int’ [-Wformat=]
       94 |         snprintf(uptime_text, sizeof(uptime_text), "%02lu:%02lu:%02lu", (uint32_t)(seconds / 3600U), (uint32_t)(seconds / 60U % 60U), (uint32_t)(seconds % 60U));
          |                                                     ~~~~^               ~~~~~~~~~~~~~~~~~~~~~~~~~~~
          |                                                         |               |
          |                                                         |               unsigned int
          |                                                         long unsigned int
          |                                                     %02u
    app/gui/render.c:94:63: warning: format ‘%lu’ expects argument of type ‘long unsigned int’, but argument 5 has type ‘unsigned int’ [-Wformat=]
       94 |         snprintf(uptime_text, sizeof(uptime_text), "%02lu:%02lu:%02lu", (uint32_t)(seconds / 3600U), (uint32_t)(seconds / 60U % 60U), (uint32_t)(seconds % 60U));
          |                                                           ~~~~^                                      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          |                                                               |                                      |
          |                                                               long unsigned int                      unsigned int
          |                                                           %02u
    app/gui/render.c:94:69: warning: format ‘%lu’ expects argument of type ‘long unsigned int’, but argument 6 has type ‘unsigned int’ [-Wformat=]
       94 |         snprintf(uptime_text, sizeof(uptime_text), "%02lu:%02lu:%02lu", (uint32_t)(seconds / 3600U), (uint32_t)(seconds / 60U % 60U), (uint32_t)(seconds % 60U));
          |                                                                 ~~~~^                                                                 ~~~~~~~~~~~~~~~~~~~~~~~~~
          |                                                                     |                                                                 |
          |                                                                     long unsigned int                                                 unsigned int
          |                                                                 %02u
    app/gui/render.c:97:55: warning: format ‘%lu’ expects argument of type ‘long unsigned int’, but argument 4 has type ‘unsigned int’ [-Wformat=]
       97 |         snprintf(uptime_text, sizeof(uptime_text), "%lu%s %02lu:%02lu:%02lu", (uint32_t)shown_days, days > 9999999U ? "+" : "d", (uint32_t)(seconds / 3600U % 24U), (uint32_t)(seconds / 60U % 60U), (uint32_t)(seconds % 60U));
          |                                                     ~~^                       ~~~~~~~~~~~~~~~~~~~~
          |                                                       |                       |
          |                                                       long unsigned int       unsigned int
          |                                                     %u
    app/gui/render.c:97:63: warning: format ‘%lu’ expects argument of type ‘long unsigned int’, but argument 6 has type ‘unsigned int’ [-Wformat=]
       97 |         snprintf(uptime_text, sizeof(uptime_text), "%lu%s %02lu:%02lu:%02lu", (uint32_t)shown_days, days > 9999999U ? "+" : "d", (uint32_t)(seconds / 3600U % 24U), (uint32_t)(seconds / 60U % 60U), (uint32_t)(seconds % 60U));
          |                                                           ~~~~^                                                                  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          |                                                               |                                                                  |
          |                                                               long unsigned int                                                  unsigned int
          |                                                           %02u
    app/gui/render.c:97:69: warning: format ‘%lu’ expects argument of type ‘long unsigned int’, but argument 7 has type ‘unsigned int’ [-Wformat=]
       97 |         snprintf(uptime_text, sizeof(uptime_text), "%lu%s %02lu:%02lu:%02lu", (uint32_t)shown_days, days > 9999999U ? "+" : "d", (uint32_t)(seconds / 3600U % 24U), (uint32_t)(seconds / 60U % 60U), (uint32_t)(seconds % 60U));
          |                                                                 ~~~~^                                                                                               ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          |                                                                     |                                                                                               |
          |                                                                     long unsigned int                                                                               unsigned int
          |                                                                 %02u
    app/gui/render.c:97:75: warning: format ‘%lu’ expects argument of type ‘long unsigned int’, but argument 8 has type ‘unsigned int’ [-Wformat=]
       97 |         snprintf(uptime_text, sizeof(uptime_text), "%lu%s %02lu:%02lu:%02lu", (uint32_t)shown_days, days > 9999999U ? "+" : "d", (uint32_t)(seconds / 3600U % 24U), (uint32_t)(seconds / 60U % 60U), (uint32_t)(seconds % 60U));
          |                                                                       ~~~~^                                                                                                                          ~~~~~~~~~~~~~~~~~~~~~~~~~
          |                                                                           |                                                                                                                          |
          |                                                                           long unsigned int                                                                                                          unsigned int
          |                                                                       %02u
    In file included from app/telemetry/app_telemetry.c:7:
    app/telemetry/app_telemetry_private.h:16:10: fatal error: telemetry/cmp_telemetry_types.h: No such file or directory
       16 | #include "telemetry/cmp_telemetry_types.h"
          |          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    compilation terminated.
    ✗ (1) • 183ms

## Activity

    $ nl -ba components/ssd1306/cmp_ssd1306.c | sed -n '1,215p' && cat components/button/README.md components/sht31d/README.md components/telemetry/README.md
         1    #include "cmp_ssd1306_config.h"
         2    #include "cmp_ssd1306_fonts_private.h"
         3
         4    #include <stddef.h>
         5    #include <string.h>
         6
         7    /** @brief The controller maps each RAM page to eight vertical pixels. */
         8    enum {
         9        PAGE_HEIGHT = 8,
        10        PAGE_COUNT = SSD1306_HEIGHT / PAGE_HEIGHT,
        11        COMMAND_CONTROL = 0x00,
        12        DATA_CONTROL = 0x40
        13    };
        14
        15    static ssd1306_status_t cmp_ssd1306_translate_status(hal_status_t status) {
        16        switch (status) {
        17            case HAL_STATUS_OK:
        18                return SSD1306_STATUS_OK;
        19            case HAL_STATUS_INVALID_ARGUMENT:
        20                return SSD1306_STATUS_INVALID_ARGUMENT;
        21            case HAL_STATUS_NOT_INITIALIZED:
        22                return SSD1306_STATUS_NOT_INITIALIZED;
        23            case HAL_STATUS_NOT_SUPPORTED:
        24                return SSD1306_STATUS_NOT_SUPPORTED;
        25            case HAL_STATUS_TIMEOUT:
        26                return SSD1306_STATUS_TIMEOUT;
        27            default:
        28                return SSD1306_STATUS_COMMUNICATION_ERROR;
        29        }
        30    }
        31
        32    static ssd1306_status_t cmp_ssd1306_require_initialized(const ssd1306_t *display) {
        33        if (display == NULL) {
        34            return SSD1306_STATUS_INVALID_ARGUMENT;
        35        }
        36        return display->initialized ? SSD1306_STATUS_OK : SSD1306_STATUS_NOT_INITIALIZED;
        37    }
        38
        39    static bool cmp_ssd1306_rectangle_fits(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        40        return x < SSD1306_WIDTH && y < SSD1306_HEIGHT &&
        41               width <= SSD1306_WIDTH - x && height <= SSD1306_HEIGHT - y;
        42    }
        43
        44    static void cmp_ssd1306_put_pixel(ssd1306_t *display, uint32_t x, uint32_t y, bool on) {
        45        uint32_t page = y / PAGE_HEIGHT;
        46        uint32_t index = page * SSD1306_WIDTH + x;
        47        uint8_t mask = (uint8_t)(1U << (y % PAGE_HEIGHT));
        48        uint8_t previous = display->framebuffer[index];
        49        uint8_t value = on ? (uint8_t)(previous | mask) : (uint8_t)(previous & (uint8_t)~mask);
        50        if (value != previous) {
        51            display->framebuffer[index] = value;
        52            display->dirty_pages |= (uint8_t)(1U << page);
        53        }
        54    }
        55
        56    static ssd1306_status_t cmp_ssd1306_write_page(ssd1306_t *display, uint32_t page) {
        57        const uint8_t commands[] = {COMMAND_CONTROL, (uint8_t)(0xB0U | page), 0x00, 0x10};
        58        hal_status_t status = hal_i2c_write(display->i2c, display->address, commands, (uint32_t)sizeof(commands));
        59        if (status != HAL_STATUS_OK) {
        60            return cmp_ssd1306_translate_status(status);
        61        }
        62
        63        uint8_t packet[SSD1306_WIDTH + 1];
        64        packet[0] = DATA_CONTROL;
        65        memcpy(packet + 1, display->framebuffer + page * SSD1306_WIDTH, SSD1306_WIDTH);
        66        return cmp_ssd1306_translate_status(hal_i2c_write(display->i2c, display->address, packet, (uint32_t)sizeof(packet)));
        67    }
        68
        69    static ssd1306_status_t cmp_ssd1306_next_codepoint(const uint8_t **cursor, uint32_t *codepoint) {
        70        if (**cursor >= 0x20 && **cursor <= 0x7E) {
        71            *codepoint = **cursor;
        72            ++*cursor;
        73            return SSD1306_STATUS_OK;
        74        }
        75        if ((*cursor)[0] == 0xC2 && (*cursor)[1] == 0xB0) {
        76            *codepoint = 0xB0;
        77            *cursor += 2;
        78            return SSD1306_STATUS_OK;
        79        }
        80        return SSD1306_STATUS_NOT_SUPPORTED;
        81    }
        82
        83    static const ssd1306_glyph_t *cmp_ssd1306_find_glyph(const ssd1306_font_data_t *font, uint32_t codepoint) {
        84        for (uint32_t i = 0; i < font->count; ++i) {
        85            if (font->glyphs[i].codepoint == codepoint) {
        86                return &font->glyphs[i];
        87            }
        88        }
        89        return NULL;
        90    }
        91
        92    static ssd1306_status_t cmp_ssd1306_measure_text_width(const char *text, const ssd1306_font_data_t *font, uint32_t *width) {
        93        const uint8_t *cursor = (const uint8_t *)text;
        94        uint32_t measured_width = 0;
        95        while (*cursor != 0) {
        96            uint32_t codepoint;
        97            ssd1306_status_t status = cmp_ssd1306_next_codepoint(&cursor, &codepoint);
        98            if (status != SSD1306_STATUS_OK) {
        99                return status;
       100            }
       101            const ssd1306_glyph_t *glyph = cmp_ssd1306_find_glyph(font, codepoint);
       102            if (glyph == NULL) {
       103                return SSD1306_STATUS_NOT_SUPPORTED;
       104            }
       105            uint32_t advance = glyph->width + (measured_width == 0 ? 0U : 1U);
       106            if (measured_width > UINT32_MAX - advance) {
       107                return SSD1306_STATUS_INVALID_ARGUMENT;
       108            }
       109            measured_width += advance;
       110        }
       111        *width = measured_width;
       112        return SSD1306_STATUS_OK;
       113    }
       114
       115    ssd1306_status_t cmp_ssd1306_init(ssd1306_t *display, const ssd1306_config_t *config) {
       116        if (display == NULL || config == NULL || config->i2c == NULL ||
       117            (config->address != 0x3C && config->address != 0x3D)) {
       118            return SSD1306_STATUS_INVALID_ARGUMENT;
       119        }
       120        *display = (ssd1306_t){.i2c = config->i2c, .address = config->address};
       121
       122        // Internal charge pump, page addressing, remapped segments and COM scan.
       123        // Keep the panel off while replacing its unspecified power-on RAM contents.
       124        const uint8_t commands[][6] = {
       125            {0x00, 0xAE, 0xD5, 0x80, 0xA8, 0x3F},
       126            {0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20},
       127            {0x02, 0xA1, 0xC8, 0xDA, 0x12, 0x81},
       128            {0xCF, 0xD9, 0xF1, 0xDB, 0x30, 0x2E},
       129            {0x23, 0x00, 0xD6, 0x00, 0xA4, 0xA6},
       130        };
       131        ssd1306_status_t status = cmp_ssd1306_translate_status(hal_i2c_write(display->i2c, display->address, (const uint8_t *)&commands, (uint32_t)sizeof(commands)));
       132        if (status != SSD1306_STATUS_OK) {
       133            return status;
       134        }
       135        for (uint32_t page = 0; page < PAGE_COUNT; ++page) {
       136            status = cmp_ssd1306_write_page(display, page);
       137            if (status != SSD1306_STATUS_OK) {
       138                return status;
       139            }
       140        }
       141
       142        const uint8_t display_on[] = {COMMAND_CONTROL, 0xAF};
       143        status = cmp_ssd1306_translate_status(hal_i2c_write(display->i2c, display->address, display_on, (uint32_t)sizeof(display_on)));
       144        if (status == SSD1306_STATUS_OK) {
       145            display->initialized = true;
       146        }
       147        return status;
       148    }
       149
       150    ssd1306_status_t cmp_ssd1306_clear(ssd1306_t *display) {
       151        ssd1306_status_t status = cmp_ssd1306_require_initialized(display);
       152        if (status != SSD1306_STATUS_OK) {
       153            return status;
       154        }
       155        for (uint32_t i = 0; i < SSD1306_FRAMEBUFFER_SIZE; ++i) {
       156            if (display->framebuffer[i] != 0) {
       157                display->framebuffer[i] = 0;
       158                display->dirty_pages |= (uint8_t)(1U << (i / SSD1306_WIDTH));
       159            }
       160        }
       161        return SSD1306_STATUS_OK;
       162    }
       163
       164    ssd1306_status_t cmp_ssd1306_fill_rect(ssd1306_t *display, uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool on) {
       165        ssd1306_status_t status = cmp_ssd1306_require_initialized(display);
       166        if (status != SSD1306_STATUS_OK) {
       167            return status;
       168        }
       169        if (width == 0 || height == 0 || !cmp_ssd1306_rectangle_fits(x, y, width, height)) {
       170            return SSD1306_STATUS_INVALID_ARGUMENT;
       171        }
       172        for (uint32_t row = 0; row < height; ++row) {
       173            for (uint32_t column = 0; column < width; ++column) {
       174                cmp_ssd1306_put_pixel(display, x + column, y + row, on);
       175            }
       176        }
       177        return SSD1306_STATUS_OK;
       178    }
       179
       180    ssd1306_status_t cmp_ssd1306_draw_line(ssd1306_t *display, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, bool on) {
       181        ssd1306_status_t status = cmp_ssd1306_require_initialized(display);
       182        if (status != SSD1306_STATUS_OK) {
       183            return status;
       184        }
       185        if (x0 >= SSD1306_WIDTH || x1 >= SSD1306_WIDTH || y0 >= SSD1306_HEIGHT || y1 >= SSD1306_HEIGHT) {
       186            return SSD1306_STATUS_INVALID_ARGUMENT;
       187        }
       188
       189        int32_t x = (int32_t)x0;
       190        int32_t y = (int32_t)y0;
       191        int32_t end_x = (int32_t)x1;
       192        int32_t end_y = (int32_t)y1;
       193        int32_t dx = x < end_x ? end_x - x : x - end_x;
       194        int32_t dy = y < end_y ? y - end_y : end_y - y;
       195        int32_t step_x = x < end_x ? 1 : -1;
       196        int32_t step_y = y < end_y ? 1 : -1;
       197        int32_t error = dx + dy;
       198        while (true) {
       199            cmp_ssd1306_put_pixel(display, (uint32_t)x, (uint32_t)y, on);
       200            if (x == end_x && y == end_y) {
       201                break;
       202            }
       203            int32_t doubled = 2 * error;
       204            if (doubled >= dy) {
       205                error += dy;
       206                x += step_x;
       207            }
       208            if (doubled <= dx) {
       209                error += dx;
       210                y += step_y;
       211            }
       212        }
       213        return SSD1306_STATUS_OK;
       214    }
       215
    # button# sht31d# telemetry
    ✓ • 0ms

## Activity

    $ rg -n 'concurr|reentran|interrupt|ISR|serializ|ownership|owned|lifetime|version|VERSION|capabil|conform|test|teste|flash|RAM|hash' hal/interface hal/ports components docs CMakeLists.txt .gitmodules -g '!cmp_ssd1306_fonts.c' -g '!generate_fonts.py' && nl -ba hal/memory/hal_memory.h && nl -ba hal/memory/hal_memory.c && nl -ba hal/ports/esp32-wroom-32/uart/hal_uart.c
    CMakeLists.txt:1:cmake_minimum_required(VERSION 3.22)
    CMakeLists.txt:8:    if(NOT IDF_VERSION_MAJOR EQUAL 6 OR NOT IDF_VERSION_MINOR EQUAL 1)
    components/led/cmp_led_config.h:4: * @brief Construction and caller-owned storage for the LED component.
    components/led/cmp_led_config.h:30: * @brief Caller-owned persistent context; fields are private to the component.
    components/led/cmp_led.h:21: * @param[out] led Required caller-owned context.
    hal/ports/esp32-wroom-32/hal_esp32_private.h:32: * values that overflow the conversion, and results equal to zero or
    hal/ports/esp32-wroom-32/hal_esp32_private.h:33: * portMAX_DELAY. The conversion depends on configTICK_RATE_HZ.
    hal/ports/esp32-wroom-32/hal_esp32_private.h:56: * Peripheral-specific capabilities must also be checked before claiming.
    components/led/cmp_led_types.h:13:/** @brief Caller-owned context, defined in cmp_led_config.h. */
    hal/interface/common/hal_pin.h:17: * The BSP defines mappings and wiring; the port validates pin capabilities.
    hal/interface/gpio/hal_gpio.h:20: * @brief Opaque GPIO object owned by the framework.
    hal/interface/gpio/hal_gpio.h:57: * @details The port validates all fields and pin capabilities before allocation.
    hal/ports/esp32-wroom-32/gpio/hal_gpio.c:48:    // disables open drain/interrupts and establishes the requested pulls.
    hal/interface/i2c/hal_i2c.h:46:    uint32_t timeout;    /** Positive lifetime driver timeout in milliseconds; port-defined values/resolution */
    components/button/cmp_button_config.h:4: * @brief Construction and caller-owned storage for the button component.
    components/button/cmp_button_config.h:33: * @brief Caller-owned persistent context; fields are private to the component.
    components/button/cmp_button.h:22: * @param[out] button Required caller-owned context.
    hal/ports/esp32-wroom-32/hal_esp32.c:31:    // The I2C driver accepts int milliseconds and uses this 32-bit conversion.
    hal/ports/esp32-wroom-32/hal_esp32.c:47:    // GPIO6..11 connect to module flash; GPIO37/38 are not exposed.
    hal/interface/uart/hal_uart.h:71:    uint32_t timeout;                 /** Positive lifetime driver timeout in milliseconds; port-defined values/resolution */
    components/button/cmp_button_types.h:15:/** @brief Caller-owned context, defined in cmp_button_config.h. */
    components/button/cmp_button_types.h:24: * Must remain callable throughout the component lifetime and return promptly.
    components/button/cmp_button_types.h:25: * Synchronization with any interrupt-updated clock belongs to the provider.
    hal/interface/timer/hal_timer.h:24: * @brief Handle a serviced hardware timer expiration in interrupt context.
    hal/interface/timer/hal_timer.h:26: * @details Keep callbacks short and nonblocking; call only ISR-safe HAL APIs.
    hal/interface/timer/hal_timer.h:35: * @details Reserves dedicated hardware for the firmware lifetime. The port
    components/telemetry/cmp_telemetry.h:25: * @param[out] telemetry Required caller-owned context.
    components/telemetry/cmp_telemetry_types.h:15:/** @brief Caller-owned context, defined in cmp_telemetry_config.h. */
    hal/ports/esp32-wroom-32/timer/hal_timer.c:19:/** @brief Persistent periodic hardware timer and its ISR callback. */
    hal/ports/esp32-wroom-32/timer/hal_timer.c:22:    hal_timer_callback_t callback; /** Application callback invoked from the timer ISR. */
    hal/ports/esp32-wroom-32/timer/hal_timer.c:23:    atomic_bool running;           /** Publishes application initialization to the ISR. */
    hal/ports/esp32-wroom-32/timer/hal_timer.c:31:_Static_assert(ATOMIC_BOOL_LOCK_FREE == 2, "Timer ISR requires lock-free atomic bool");
    hal/ports/esp32-wroom-32/timer/hal_timer.c:33:static bool IRAM_ATTR on_alarm(gptimer_handle_t device, const gptimer_alarm_event_data_t *event, void *context) {
    hal/ports/esp32-wroom-32/timer/hal_timer.c:74:        // Enabling installs the interrupt and acquires the SDK clock lock now.
    hal/ports/esp32-wroom-32/timer/hal_timer.c:97:#if CONFIG_GPTIMER_ISR_CACHE_SAFE
    hal/ports/esp32-wroom-32/timer/hal_timer.c:118:#if CONFIG_GPTIMER_ISR_CACHE_SAFE
    components/telemetry/cmp_telemetry_config.h:4: * @brief Construction and caller-owned storage for the telemetry component.
    components/telemetry/cmp_telemetry_config.h:24: * @brief Caller-owned persistent context; fields are private to the component.
    hal/ports/esp32-wroom-32/uart/hal_uart.c:108:        // The TX completion wait enables its own interrupt when needed.
    components/ssd1306/cmp_ssd1306_config.h:30: * @brief Caller-owned persistent storage; fields are private to the component.
    components/ssd1306/cmp_ssd1306_config.h:39:    uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE]; /** Page-major pixels, bit zero at the top of each page. */
    components/ssd1306/cmp_ssd1306_types.h:15:/** @brief Caller-owned context; its storage definition is in cmp_ssd1306_config.h. */
    components/ssd1306/cmp_ssd1306_types.h:42: * each row's final byte are ignored. The caller retains ownership of data.
    components/ssd1306/cmp_ssd1306.c:7:/** @brief The controller maps each RAM page to eight vertical pixels. */
    components/ssd1306/cmp_ssd1306.c:123:    // Keep the panel off while replacing its unspecified power-on RAM contents.
    components/ssd1306/cmp_ssd1306.c:155:    for (uint32_t i = 0; i < SSD1306_FRAMEBUFFER_SIZE; ++i) {
    components/ssd1306/cmp_ssd1306.h:6: * Serialize calls and shared-bus access in normal context; no ISR or
    components/ssd1306/cmp_ssd1306.h:10: * Drawing changes RAM only, validates bounds before editing, and never clips.
    components/ssd1306/cmp_ssd1306.h:28:#define SSD1306_FRAMEBUFFER_SIZE (SSD1306_WIDTH * SSD1306_HEIGHT / 8) /** Framebuffer size (bytes). */
    components/ssd1306/cmp_ssd1306.h:31: * @brief Configure the controller, clear its RAM while off, and turn it on.
    components/ssd1306/cmp_ssd1306.h:38: * @param[out] display Required caller-owned context.
    components/sht31d/cmp_sht31d.h:15:#define SHT31D_MEASUREMENT_TIME 16 /** Minimum conversion wait after cmp_sht31d_start_measurement() returns (ms) */
    components/sht31d/cmp_sht31d.h:17:/** @brief Caller-owned context; its storage definition is in cmp_sht31d_config.h. */
    components/sht31d/cmp_sht31d.h:38: * @details Values use the datasheet conversion, rounded to the nearest integer.
    components/sht31d/cmp_sht31d.h:53: * first and let any previous conversion finish. Does not reset the sensor or
    components/sht31d/cmp_sht31d.h:56: * @param[out] sensor Required caller-owned context.
    components/sht31d/cmp_sht31d.h:71: * @details Sends 0x2400 without waiting for conversion. After success, the caller
    components/sht31d/cmp_sht31d.h:74: * pending, but conversion may have started: wait the full interval before retry
    components/sht31d/cmp_sht31d.h:92: * @details Wait the full conversion interval first; an early NACK is reported
    components/sht31d/cmp_sht31d_config.h:4: * @brief Construction and caller-owned storage for the SHT31-D composition root.
    components/sht31d/cmp_sht31d_config.h:21: * @brief Resources supplied by the composition, with no transfer of ownership.
    components/sht31d/cmp_sht31d_config.h:33: * @brief Caller-owned persistent context; fields are private to the component.
    components/sht31d/cmp_sht31d.c:66:    // Single shot, high repeatability, no clock stretching. Conversion is external
         1    /**
         2     * @file hal_memory.h
         3     * @author Fabricio Santos (fmsantos8@ucs.br)
         4     * @brief Shared persistent memory arena.
         5     */
         6    #ifndef HAL_MEMORY_H
         7    #define HAL_MEMORY_H
         8
         9    #include <stdbool.h>
        10    #include <stddef.h>
        11    #include <stdint.h>
        12
        13    #if !defined(__cplusplus) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L)
        14    #error "hal_memory requires C11 or later"
        15    #endif
        16
        17    #ifdef __cplusplus
        18    extern "C" {
        19    #endif
        20
        21    /** @brief Memory-module results, independent of HAL and SDK status types. */
        22    typedef enum {
        23        HAL_MEMORY_OK = 0,           /** Operation succeeded. */
        24        HAL_MEMORY_INVALID_ARGUMENT, /** A required pointer, size, capacity, or buffer alignment is invalid. */
        25        HAL_MEMORY_NOT_INITIALIZED,  /** The arena has not been initialized. */
        26        HAL_MEMORY_INVALID_STATE,    /** Initialization already succeeded, or allocation is forbidden because the arena is sealed. */
        27        HAL_MEMORY_NO_MEMORY         /** The requested block and required alignment padding do not fit. */
        28    } hal_memory_status_t;
        29
        30    /** @brief Arena snapshot, available before and after sealing. */
        31    typedef struct {
        32        uint32_t capacity;  /** Total backing-storage size in bytes. */
        33        uint32_t used;      /** Bytes consumed by blocks and preceding alignment padding. */
        34        uint32_t remaining; /** capacity - used; a subsequent allocation may need padding. */
        35        bool sealed;        /** True after allocation has permanently ended. */
        36    } hal_memory_info_t;
        37
        38    /**
        39     * @brief Initialize the module's singleton arena with external storage.
        40     *
        41     * @details One private arena serves the firmware; no caller context is needed.
        42     * Dedicate at least capacity writable bytes, aligned for max_align_t, for all
        43     * allocated objects' lifetimes. Storage must not overlap outputs or another
        44     * allocator's storage and is not cleared. Invalid initial arguments allow retry;
        45     * after success, init() returns INVALID_STATE without changing either buffer
        46     * or the arena, even after sealing. Serialize all calls in normal context;
        47     * no synchronization or ISR support.
        48     *
        49     * @param[in] buffer Caller-owned backing storage, suitably aligned.
        50     * @param[in] capacity Positive backing-storage size in bytes; need not be an
        51     * alignment multiple.
        52     *
        53     * @retval HAL_MEMORY_OK The arena is initialized and open.
        54     * @retval HAL_MEMORY_INVALID_ARGUMENT A required pointer is NULL, capacity is
        55     * zero, or buffer is not aligned for max_align_t.
        56     * @retval HAL_MEMORY_INVALID_STATE The arena has already been initialized.
        57     */
        58    hal_memory_status_t hal_memory_init(void *buffer, uint32_t capacity);
        59
        60    /**
        61     * @brief Reserve a persistent, nonempty block aligned for max_align_t.
        62     *
        63     * @details Blocks are uninitialized; alignment padding consumes capacity.
        64     * Extended alignment is unsupported. Failure consumes no storage. When
        65     * pointer is non-NULL, *pointer is cleared before validation and receives
        66     * the block address only on success. The output must not overlap
        67     * backing storage. Pass an actual void * variable's address, then assign it to
        68     * the typed pointer; do not cast a typed pointer-to-pointer to void **.
        69     *
        70     * @param[in] size Positive requested byte count.
        71     * @param[out] pointer Required pointer output, independent of the arena.
        72     *
        73     * @retval HAL_MEMORY_OK A disjoint block of size bytes is reserved.
        74     * @retval HAL_MEMORY_INVALID_ARGUMENT A required pointer is NULL or size is zero.
        75     * @retval HAL_MEMORY_NOT_INITIALIZED The arena has not been initialized.
        76     * @retval HAL_MEMORY_INVALID_STATE The arena is sealed.
        77     * @retval HAL_MEMORY_NO_MEMORY The block and required padding do not fit.
        78     */
        79    hal_memory_status_t hal_memory_alloc(uint32_t size, void **pointer);
        80
        81    /**
        82     * @brief Permanently prevent further allocations without altering live blocks.
        83     *
        84     * @details Idempotent; preserves storage and consumption. No reset or reopening
        85     * during this firmware run, including through init().
        86     *
        87     * @retval HAL_MEMORY_OK The arena is sealed, including a repeated seal.
        88     * @retval HAL_MEMORY_NOT_INITIALIZED The arena has not been initialized.
        89     */
        90    hal_memory_status_t hal_memory_seal(void);
        91
        92    /**
        93     * @brief Obtain capacity, consumption, and lifecycle information.
        94     *
        95     * @details Queries allocate no storage and remain available after sealing.
        96     * The output is written only on success and must not overlap backing storage.
        97     *
        98     * @param[out] info Required independent snapshot output.
        99     *
       100     * @retval HAL_MEMORY_OK The snapshot was written.
       101     * @retval HAL_MEMORY_INVALID_ARGUMENT A required pointer is NULL.
       102     * @retval HAL_MEMORY_NOT_INITIALIZED The arena has not been initialized.
       103     */
       104    hal_memory_status_t hal_memory_get_info(hal_memory_info_t *info);
       105
       106    #ifdef __cplusplus
       107    }
       108    #endif
       109
       110    #endif
         1    #include "hal_memory.h"
         2
         3    #include <stdint.h>
         4
         5    _Static_assert(_Alignof(max_align_t) <= UINT32_MAX, "hal_memory alignment must fit uint32_t");
         6
         7    /**
         8     * @brief Private context for the module's single arena.
         9     */
        10    typedef struct {
        11        unsigned char *buffer; /** Caller-owned backing storage aligned for max_align_t. */
        12        uint32_t capacity;     /** Total backing-storage size in bytes. */
        13        uint32_t used;         /** Bytes consumed by blocks and preceding alignment padding. */
        14        bool initialized;      /** True after successful arena initialization. */
        15        bool sealed;           /** True after allocation has permanently ended. */
        16    } hal_memory_t;
        17
        18    static hal_memory_t memory = {0};
        19
        20    hal_memory_status_t hal_memory_init(void *buffer, uint32_t capacity) {
        21        if (memory.initialized) {
        22            return HAL_MEMORY_INVALID_STATE;
        23        }
        24        if (buffer == NULL || capacity == 0) {
        25            return HAL_MEMORY_INVALID_ARGUMENT;
        26        }
        27
        28        if ((uintptr_t)buffer % _Alignof(max_align_t) != 0) {
        29            return HAL_MEMORY_INVALID_ARGUMENT;
        30        }
        31
        32        memory.buffer = buffer;
        33        memory.capacity = capacity;
        34        memory.used = 0;
        35        memory.initialized = true;
        36        memory.sealed = false;
        37        return HAL_MEMORY_OK;
        38    }
        39
        40    hal_memory_status_t hal_memory_alloc(uint32_t size, void **pointer) {
        41        if (pointer != NULL) {
        42            *pointer = NULL;
        43        }
        44
        45        if (pointer == NULL || size == 0) {
        46            return HAL_MEMORY_INVALID_ARGUMENT;
        47        }
        48
        49        if (!memory.initialized) {
        50            return HAL_MEMORY_NOT_INITIALIZED;
        51        }
        52
        53        if (memory.sealed) {
        54            return HAL_MEMORY_INVALID_STATE;
        55        }
        56
        57        const uint32_t alignment = (uint32_t)_Alignof(max_align_t);
        58        const uint32_t padding = (alignment - memory.used % alignment) % alignment;
        59        const uint32_t remaining = memory.capacity - memory.used;
        60
        61        // Subtract before adding so even a UINT32_MAX request cannot wrap the cursor.
        62        if (padding > remaining || size > remaining - padding) {
        63            return HAL_MEMORY_NO_MEMORY;
        64        }
        65
        66        *pointer = memory.buffer + memory.used + padding;
        67        memory.used += padding + size;
        68        return HAL_MEMORY_OK;
        69    }
        70
        71    hal_memory_status_t hal_memory_seal(void) {
        72        if (!memory.initialized) {
        73            return HAL_MEMORY_NOT_INITIALIZED;
        74        }
        75
        76        memory.sealed = true;
        77        return HAL_MEMORY_OK;
        78    }
        79
        80    hal_memory_status_t hal_memory_get_info(hal_memory_info_t *info) {
        81        if (info == NULL) {
        82            return HAL_MEMORY_INVALID_ARGUMENT;
        83        }
        84
        85        if (!memory.initialized) {
        86            return HAL_MEMORY_NOT_INITIALIZED;
        87        }
        88
        89        *info = (hal_memory_info_t){
        90            .capacity = memory.capacity,
        91            .used = memory.used,
        92            .remaining = memory.capacity - memory.used,
        93            .sealed = memory.sealed,
        94        };
        95        return HAL_MEMORY_OK;
        96    }
         1    #include "uart/hal_uart.h"
         2    #include "hal_esp32_private.h"
         3    #include "hal_memory.h"
         4
         5    #include <stdbool.h>
         6    #include <stddef.h>
         7    #include <stdint.h>
         8
         9    #include "driver/gpio.h"
        10    #include "driver/uart.h"
        11    #include "esp_pm.h"
        12    #include "freertos/task.h"
        13    #include "sdkconfig.h"
        14
        15    /** @brief Persistent UART state with a finite transmit-wait budget. */
        16    struct hal_uart {
        17        uart_port_t port;                 /** SDK controller identifier. */
        18        hal_uart_data_width_t data_width; /** Payload validation rule. */
        19        TickType_t timeout;               /** Finite transmit budget in scheduler ticks. */
        20        bool initialized;                 /** True after complete construction. */
        21    #if CONFIG_PM_ENABLE
        22        esp_pm_lock_handle_t clock_lock; /** Keeps APB stable, including pending bytes after timeout. */
        23    #endif
        24    };
        25
        26    static bool claimed_controllers[SOC_UART_NUM];
        27
        28    _Static_assert(sizeof(hal_uart_t) <= UINT32_MAX, "UART object exceeds arena size range");
        29    _Static_assert(_Alignof(hal_uart_t) <= _Alignof(max_align_t), "UART object requires extended alignment");
        30
        31    static hal_status_t validate_config(const hal_uart_config_t *config, TickType_t *ticks) {
        32        if (config->instance >= SOC_UART_NUM || config->baudrate == 0 || config->timeout == 0 ||
        33            (config->data_width != HAL_UART_DATA_WIDTH_7 && config->data_width != HAL_UART_DATA_WIDTH_8) ||
        34            (config->parity != HAL_UART_PARITY_NONE && config->parity != HAL_UART_PARITY_EVEN && config->parity != HAL_UART_PARITY_ODD) ||
        35            (config->stop_count != HAL_UART_STOP_COUNT_1 && config->stop_count != HAL_UART_STOP_COUNT_2) ||
        36            (config->transmit_pin.port == config->receive_pin.port && config->transmit_pin.number == config->receive_pin.number)) {
        37            return HAL_STATUS_INVALID_ARGUMENT;
        38        }
        39        hal_status_t status = hal_esp32_pin_validate(config->transmit_pin);
        40        if (status != HAL_STATUS_OK) {
        41            return status;
        42        }
        43        status = hal_esp32_pin_validate(config->receive_pin);
        44        if (status != HAL_STATUS_OK) {
        45            return status;
        46        }
        47        if (!GPIO_IS_VALID_OUTPUT_GPIO(config->transmit_pin.number) || config->baudrate < 80 || config->baudrate > UART_BITRATE_MAX) {
        48            return HAL_STATUS_NOT_SUPPORTED;
        49        }
        50        return hal_esp32_convert_timeout_to_ticks(config->timeout, ticks);
        51    }
        52
        53    static hal_status_t configure_uart(hal_uart_t *object, const hal_uart_config_t *config) {
        54        gpio_config_t pin = {
        55            .pin_bit_mask = UINT64_C(1) << config->transmit_pin.number,
        56            .mode = GPIO_MODE_DISABLE,
        57            .pull_up_en = GPIO_PULLUP_DISABLE,
        58            .pull_down_en = GPIO_PULLDOWN_DISABLE,
        59            .intr_type = GPIO_INTR_DISABLE,
        60        };
        61        // Establish idle-high TX and a digital RX before connecting the UART matrix.
        62        if (gpio_config(&pin) != ESP_OK || gpio_set_level((gpio_num_t)config->transmit_pin.number, 1) != ESP_OK) {
        63            return HAL_STATUS_ERROR;
        64        }
        65        pin.pin_bit_mask = UINT64_C(1) << config->receive_pin.number;
        66        pin.mode = GPIO_MODE_INPUT;
        67        if (gpio_config(&pin) != ESP_OK) {
        68            return HAL_STATUS_ERROR;
        69        }
        70
        71        esp_err_t error = ESP_OK;
        72        bool installed = false;
        73    #if CONFIG_PM_ENABLE
        74        bool clock_acquired = false;
        75        error = esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, "hal_uart", &object->clock_lock);
        76        if (error == ESP_OK) {
        77            error = esp_pm_lock_acquire(object->clock_lock);
        78            clock_acquired = error == ESP_OK;
        79        }
        80    #endif
        81        if (error == ESP_OK) {
        82            // TX buffering is disabled: only copied FIFO bytes count as accepted.
        83            error = uart_driver_install(object->port, UART_HW_FIFO_LEN(object->port) + 1, 0, 0, NULL, 0);
        84            installed = error == ESP_OK;
        85        }
        86        if (error == ESP_OK) {
        87            const uart_config_t uart_config = {
        88                .baud_rate = (int)config->baudrate,
        89                .data_bits = config->data_width == HAL_UART_DATA_WIDTH_7 ? UART_DATA_7_BITS : UART_DATA_8_BITS,
        90                .parity = config->parity == HAL_UART_PARITY_NONE ? UART_PARITY_DISABLE : config->parity == HAL_UART_PARITY_EVEN ? UART_PARITY_EVEN
        91                                                                                                                                : UART_PARITY_ODD,
        92                .stop_bits = config->stop_count == HAL_UART_STOP_COUNT_1 ? UART_STOP_BITS_1 : UART_STOP_BITS_2,
        93                .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        94                .source_clk = UART_SCLK_APB,
        95            };
        96            error = uart_param_config(object->port, &uart_config);
        97        }
        98        if (error == ESP_OK) {
        99            error = uart_set_mode(object->port, UART_MODE_UART);
       100        }
       101        if (error == ESP_OK) {
       102            error = uart_set_line_inverse(object->port, UART_SIGNAL_INV_DISABLE);
       103        }
       104        if (error == ESP_OK) {
       105            error = uart_set_pin(object->port, config->transmit_pin.number, config->receive_pin.number, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
       106        }
       107        if (error == ESP_OK) {
       108            // The TX completion wait enables its own interrupt when needed.
       109            error = uart_disable_intr_mask(object->port, UINT32_MAX);
       110        }
       111        if (error == ESP_OK) {
       112            if (gpio_hold_dis((gpio_num_t)config->transmit_pin.number) != ESP_OK ||
       113                gpio_hold_dis((gpio_num_t)config->receive_pin.number) != ESP_OK) {
       114                error = ESP_FAIL;
       115            }
       116        }
       117        if (error != ESP_OK) {
       118            if (installed) {
       119                uart_driver_delete(object->port);
       120            }
       121    #if CONFIG_PM_ENABLE
       122            if (clock_acquired) {
       123                esp_pm_lock_release(object->clock_lock);
       124            }
       125            if (object->clock_lock != NULL) {
       126                esp_pm_lock_delete(object->clock_lock);
       127            }
       128    #endif
       129            return error == ESP_ERR_NO_MEM ? HAL_STATUS_NO_MEMORY : HAL_STATUS_ERROR;
       130        }
       131        return HAL_STATUS_OK;
       132    }
       133
       134    hal_status_t hal_uart_config(hal_uart_t **uart, const hal_uart_config_t *config) {
       135        if (uart == NULL) {
       136            return HAL_STATUS_INVALID_ARGUMENT;
       137        }
       138        *uart = NULL;
       139        if (config == NULL) {
       140            return HAL_STATUS_INVALID_ARGUMENT;
       141        }
       142        TickType_t ticks;
       143        hal_status_t status = validate_config(config, &ticks);
       144        if (status != HAL_STATUS_OK) {
       145            return status;
       146        }
       147        status = hal_esp32_pin_claim(config->transmit_pin);
       148        if (status != HAL_STATUS_OK) {
       149            return status;
       150        }
       151        status = hal_esp32_pin_claim(config->receive_pin);
       152        if (status != HAL_STATUS_OK) {
       153            hal_esp32_pin_release(config->transmit_pin);
       154            return status;
       155        }
       156        bool busy = claimed_controllers[config->instance] || uart_is_driver_installed((uart_port_t)config->instance);
       157    #if CONFIG_ESP_CONSOLE_UART
       158        busy = busy || config->instance == CONFIG_ESP_CONSOLE_UART_NUM;
       159    #endif
       160        if (busy) {
       161            hal_esp32_pin_release(config->transmit_pin);
       162            hal_esp32_pin_release(config->receive_pin);
       163            return HAL_STATUS_RESOURCE_BUSY;
       164        }
       165        void *storage = NULL;
       166        status = hal_esp32_memory_status(hal_memory_alloc((uint32_t)sizeof(hal_uart_t), &storage));
       167        if (status != HAL_STATUS_OK) {
       168            hal_esp32_pin_release(config->transmit_pin);
       169            hal_esp32_pin_release(config->receive_pin);
       170            return status;
       171        }
       172        claimed_controllers[config->instance] = true;
       173        hal_uart_t *object = storage;
       174        *object = (hal_uart_t){.port = (uart_port_t)config->instance, .data_width = config->data_width, .timeout = ticks};
       175        status = configure_uart(object, config);
       176        if (status != HAL_STATUS_OK) {
       177            return status;
       178        }
       179        object->initialized = true;
       180        *uart = object;
       181        return HAL_STATUS_OK;
       182    }
       183
       184    hal_status_t hal_uart_write(hal_uart_t *uart, const uint8_t *buffer, uint32_t size, uint32_t *transmitted_count) {
       185        if (transmitted_count == NULL) {
       186            return HAL_STATUS_INVALID_ARGUMENT;
       187        }
       188        *transmitted_count = 0;
       189        if (uart == NULL || (buffer == NULL && size != 0)) {
       190            return HAL_STATUS_INVALID_ARGUMENT;
       191        }
       192        if (!uart->initialized) {
       193            return HAL_STATUS_NOT_INITIALIZED;
       194        }
       195        if (uart->data_width == HAL_UART_DATA_WIDTH_7) {
       196            for (uint32_t i = 0; i < size; ++i) {
       197                if (buffer[i] > 0x7F) {
       198                    return HAL_STATUS_INVALID_ARGUMENT;
       199                }
       200            }
       201        }
       202        if (size == 0) {
       203            return HAL_STATUS_OK;
       204        }
       205
       206        TickType_t start = xTaskGetTickCount();
       207        while (*transmitted_count < size) {
       208            if ((TickType_t)(xTaskGetTickCount() - start) >= uart->timeout) {
       209                return HAL_STATUS_TIMEOUT;
       210            }
       211            // Exclusive, sequential use keeps the SDK's private TX mutex uncontended.
       212            int accepted = uart_tx_chars(uart->port, (const char *)(buffer + *transmitted_count), size - *transmitted_count);
       213            if (accepted < 0 || (uint32_t)accepted > size - *transmitted_count) {
       214                return HAL_STATUS_ERROR;
       215            }
       216            *transmitted_count += (uint32_t)accepted;
       217            if (accepted == 0) {
       218                vTaskDelay(1);
       219            }
       220        }
       221        TickType_t elapsed = (TickType_t)(xTaskGetTickCount() - start);
       222        TickType_t remaining = elapsed < uart->timeout ? uart->timeout - elapsed : 0;
       223        esp_err_t error = uart_wait_tx_done(uart->port, remaining);
       224        if (error == ESP_OK) {
       225            return HAL_STATUS_OK;
       226        }
       227        return error == ESP_ERR_TIMEOUT ? HAL_STATUS_TIMEOUT : HAL_STATUS_ERROR;
       228    }
    ✓ • 0ms

## Activity

    $ nl -ba CMakeLists.txt && nl -ba hal/interface/gpio/hal_gpio.h | sed -n '75,110p' && git ls-files '*test*' '*valid*' '*report*' '*sdkconfig*' '*README*' && git status --short
         1    cmake_minimum_required(VERSION 3.22)
         2
         3    set(HAL_ESP32_INTERFACE_DIR "${CMAKE_CURRENT_LIST_DIR}/hal/interface" CACHE PATH "Portable HAL interface directory")
         4    set(HAL_ESP32_MEMORY_DIR "${CMAKE_CURRENT_LIST_DIR}/hal/memory" CACHE PATH "HAL arena directory")
         5
         6    # ESP-IDF reads this file again to register the firmware as one component.
         7    if(DEFINED COMPONENT_NAME AND "${COMPONENT_DIR}" STREQUAL "${CMAKE_CURRENT_LIST_DIR}")
         8        if(NOT IDF_VERSION_MAJOR EQUAL 6 OR NOT IDF_VERSION_MINOR EQUAL 1)
         9            message(FATAL_ERROR "esp32_framework requires ESP-IDF 6.1.x")
        10        endif()
        11
        12        idf_component_register(
        13            SRCS
        14                "main/main.c"
        15                "bsp/esp32_devkit_v1/bsp.c"
        16                "app/button/app_button.c"
        17                "app/status_led/app_status_led.c"
        18                "app/acquisition/app_acquisition.c"
        19                "app/telemetry/app_telemetry.c"
        20                "app/gui/app_gui.c"
        21                "app/gui/render.c"
        22                "app/gui/bitmaps.c"
        23                "components/button/cmp_button.c"
        24                "components/led/cmp_led.c"
        25                "components/sht31d/cmp_sht31d.c"
        26                "components/ssd1306/cmp_ssd1306.c"
        27                "components/ssd1306/cmp_ssd1306_fonts.c"
        28                "components/telemetry/cmp_telemetry.c"
        29                "hal/ports/esp32-wroom-32/hal_esp32.c"
        30                "hal/ports/esp32-wroom-32/system/hal_system.c"
        31                "hal/ports/esp32-wroom-32/gpio/hal_gpio.c"
        32                "hal/ports/esp32-wroom-32/i2c/hal_i2c.c"
        33                "hal/ports/esp32-wroom-32/uart/hal_uart.c"
        34                "hal/ports/esp32-wroom-32/timer/hal_timer.c"
        35                "${HAL_ESP32_MEMORY_DIR}/hal_memory.c"
        36            INCLUDE_DIRS "app" "bsp" "components/button" "components/led" "components/sht31d" "components/ssd1306" "components/telemetry" "${HAL_ESP32_INTERFACE_DIR}"
        37            PRIV_INCLUDE_DIRS "hal/ports/esp32-wroom-32" "${HAL_ESP32_MEMORY_DIR}"
        38            PRIV_REQUIRES
        39                esp_driver_gpio esp_driver_i2c esp_driver_uart esp_driver_gptimer
        40                esp_hw_support esp_pm freertos esp_common
        41            REQUIRED_IDF_TARGETS esp32
        42        )
        43
        44        target_compile_features(${COMPONENT_LIB} PUBLIC c_std_11)
        45        return()
        46    endif()
        47
        48    set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}")
        49    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
        50    project(esp32_framework)
        75     */
        76    hal_status_t hal_gpio_config(hal_gpio_t **gpio, const hal_gpio_config_t *config);
        77
        78    /**
        79     * @brief Synchronously sample the physical level of a digital input.
        80     *
        81     * @param[in] gpio Valid GPIO handle configured as an input.
        82     * @param[out] level Receives false for low or true for high, only on success.
        83     *
        84     * @retval HAL_STATUS_OK The input level was read.
        85     * @retval HAL_STATUS_INVALID_ARGUMENT A required pointer is NULL.
        86     * @retval HAL_STATUS_NOT_INITIALIZED The resource is not ready.
        87     * @retval HAL_STATUS_NOT_SUPPORTED The handle is configured as an output.
        88     * @retval HAL_STATUS_ERROR The platform could not read the pin.
        89     */
        90    hal_status_t hal_gpio_read(hal_gpio_t *gpio, bool *level);
        91
        92    /**
        93     * @brief Synchronously set the physical level of a push-pull output.
        94     *
        95     * @param[in] gpio Valid GPIO handle configured as an output.
        96     * @param[in] level Requested electrical level: false is low and true is high.
        97     *
        98     * @retval HAL_STATUS_OK The output operation was issued successfully.
        99     * @retval HAL_STATUS_INVALID_ARGUMENT The handle is NULL.
       100     * @retval HAL_STATUS_NOT_INITIALIZED The resource is not ready.
       101     * @retval HAL_STATUS_NOT_SUPPORTED The handle is configured as an input.
       102     * @retval HAL_STATUS_ERROR The platform could not write the pin.
       103     */
       104    hal_status_t hal_gpio_write(hal_gpio_t *gpio, bool level);
       105
       106    #ifdef __cplusplus
       107    }
       108    #endif
       109
       110    #endif
    ?? proposta.pdf
    ✓ • 0ms

## Assistant

A prova de conceito contém os comportamentos previstos: aquisição periódica, duas telas, alternância pelo botão, preservação da última amostra e indicação de erro. A checagem de sintaxe também revelou um bloqueio de compilação: um cabeçalho da telemetria usa um caminho que os diretórios definidos no CMake não resolvem. Ainda não há, nesta árvore, os outros portes, as versões convencionais ou os registros de validação exigidos pela proposta.

## Activity

    $ rg -n '\b(ESP_|esp_|CONFIG_|FreeRTOS|TickType_t|vTask|xTask|HAL_[A-Z][A-Z0-9]*_|STM32|NRF_|nrf_|k_sleep|k_msleep|zephyr)\b|#\s*include.*(bsp|ports/|driver/|freertos/|zephyr/|stm32)' app components hal/interface -g '*.[ch]' && git diff --stat
    hal/interface/common/hal_pin.h:15: * Bank identifiers are zero-based: ESP32 uses 0; STM32 A/B use 0/1; nRF52840
    ✓ • 0ms

## Activity

    $ PYTHONDONTWRITEBYTECODE=1 python3 scripts/build.py -B /tmp/esp32-framework-review-build
    Erro: ESP-IDF não encontrado. Abra um terminal ESP-IDF 6.1 ou ative o ambiente com o script gerado pelo instalador (no EIM, activate_idf_<versão>.sh/.ps1). Para instalações tradicionais, use o export.sh/export.ps1 do SDK.
    ✗ (1) • 0ms

## Activity

    $ cc -std=c11 -fsyntax-only -Wno-format -Iapp -Ibsp -Icomponents -Icomponents/button -Icomponents/led -Icomponents/sht31d -Icomponents/ssd1306 -Icomponents/telemetry -Ihal/interface app/acquisition/app_acquisition.c app/button/app_button.c app/gui/app_gui.c app/gui/bitmaps.c app/gui/render.c app/status_led/app_status_led.c app/telemetry/app_telemetry.c components/button/cmp_button.c components/led/cmp_led.c components/sht31d/cmp_sht31d.c components/ssd1306/cmp_ssd1306.c components/ssd1306/cmp_ssd1306_fonts.c components/telemetry/cmp_telemetry.c bsp/esp32_devkit_v1/bsp.c hal/memory/hal_memory.c
    ✓ • 76ms

## Assistant

**O projeto atende parcialmente à proposta.** A arquitetura atual está alinhada ao desenho descrito no [proposta.pdf](/home/julia/Documentos/fabricio/esp32_framework/proposta.pdf), e boa parte da prova de conceito já está implementada para ESP32. Porém, há divergências nos contratos, um bloqueio de compilação e entregas ausentes que impedem considerar a proposta integralmente atendida.

Comparei as seções 3.1 a 3.6 do PDF com o código desta árvore. A avaliação abaixo distingue implementação encontrada de comportamento efetivamente validado em hardware.

A separação de responsabilidades está bem encaminhada:

- `app/` concentra aquisição, seleção de telas, indicação de estado e política de repetição.
- `components/` encapsula botão, LED, sensor, display e telemetria, recebendo suas dependências por parâmetros.
- `hal/interface/` utiliza tipos opacos e códigos de estado independentes dos fabricantes.
- O porte ESP32 implementa o acesso ao SDK; o BSP concentra pinos, polaridades, endereços e configurações.
- [main.c](/home/julia/Documentos/fabricio/esp32_framework/main/main.c:49) atua como raiz de composição, inicializando e conectando os módulos.

**O uso de FreeRTOS em `main.c` não viola a proposta:** ela permite que a integração utilize mecanismos da plataforma. Não encontrei dependências de SDK, BSP ou porte nos arquivos comuns de aplicação e componentes.

| Requisito do PDF | Situação atual | Avaliação |
|---|---|---|
| **RF01 — Independência dos SDKs** | Aplicação e componentes utilizam interfaces próprias e contratos comuns. | Atendido na inspeção do código. |
| **RF02 — Diferenças restritas à região específica** | Configurações físicas ficam no BSP; operações do SDK ficam no porte e na integração. | Atendido no recorte ESP32. |
| **RF03 — Porte reutilizável entre aplicações** | O porte não contém política da prova de conceito. | Estrutura compatível; reúso em outra aplicação não demonstrado. |
| **RF04 — Contratos observáveis** | Existem contratos documentados, mas temporização e I2C divergem do PDF. | Parcial. |
| **RF05 — RTOS fora do contrato central** | O código comum é processado por chamadas sucessivas. | Atendido. |
| **RF06 — Componentes com dependências injetadas** | Os cinco componentes recebem recursos já construídos. | Atendido na inspeção do código. |
| **RF07 — Versão e conformidade dos portes** | Não encontrei declaração de compatibilidade com versões dos contratos nem roteiro de conformidade. | Não atendido nesta árvore. |
| **RF08 — Mesma aplicação nas quatro placas** | Existe somente integração para ESP32. | Não demonstrado. |
| **RF09 — Mesmo porte nas duas STM32** | Não há porte STM32F4 nem os respectivos BSPs. | Ausente nesta árvore. |
| **RF10 — Implementações convencionais equivalentes** | Não encontrei as versões sem framework. | Ausente nesta árvore. |

O comportamento previsto na seção 3.5 está amplamente representado no código:

| Comportamento | Evidência encontrada |
|---|---|
| Aquisição periódica | Intervalo de 2 segundos, com espera de conversão controlada por estado. |
| Preservação da última leitura válida | A amostra armazenada só é substituída após leitura bem-sucedida. |
| Duas telas | Temperatura/umidade e diagnóstico com estado da aquisição e tempo ativo. |
| Alternância pelo botão | Detecção de borda após filtragem com padrão de 30 ms. |
| LED de estado | Apagado na inicialização, aceso quando pronto e piscando em falhas I2C. |
| Telemetria comum | Registros `SAMPLE` e `ERROR`, formatados pelo componente. |
| I2C compartilhado | Sensor e display recebem o mesmo handle, com endereços distintos. |
| Serialização do barramento | As operações são chamadas sequencialmente pelo laço principal. |

Esses mecanismos aparecem em [app_acquisition.c](/home/julia/Documentos/fabricio/esp32_framework/app/acquisition/app_acquisition.c:68), [render.c](/home/julia/Documentos/fabricio/esp32_framework/app/gui/render.c:45), [cmp_button.c](/home/julia/Documentos/fabricio/esp32_framework/components/button/cmp_button.c:45) e [app_status_led.c](/home/julia/Documentos/fabricio/esp32_framework/app/status_led/app_status_led.c:54). As ligações documentadas também correspondem aos circuitos externos previstos no PDF.

Há pontos concretos que precisam ser resolvidos:

1. **A configuração atual contém um bloqueio de compilação.**  
   [app_telemetry_private.h:16](/home/julia/Documentos/fabricio/esp32_framework/app/telemetry/app_telemetry_private.h:16) inclui `"telemetry/cmp_telemetry_types.h"`, mas o [CMakeLists.txt:36](/home/julia/Documentos/fabricio/esp32_framework/CMakeLists.txt:36) fornece `components/telemetry` como diretório de inclusão, sem fornecer `components`. Reproduzi o erro `No such file or directory` na checagem sintática com esses caminhos. O include deveria ser compatibilizado com a configuração de compilação.

2. **O temporizador oferece um contrato diferente do proposto.**  
   As seções 3.1, 3.3 e 3.5 exigem consulta ao tempo decorrido em milissegundos. A [interface atual](/home/julia/Documentos/fabricio/esp32_framework/hal/interface/timer/hal_timer.h:64) oferece configuração e início de notificações periódicas. A integração cria a contagem incrementando um contador no callback. O próprio contrato informa que períodos podem ser perdidos ou aglutinados: essa contagem **não garante equivalência com tempo físico decorrido**, afetando aquisição, debounce e tempo ativo. É necessário adequar a implementação ou revisar explicitamente essa exigência no texto.

3. **Falta a transação I2C combinada prevista no Quadro 3.**  
   A [interface I2C](/home/julia/Documentos/fabricio/esp32_framework/hal/interface/i2c/hal_i2c.h:64) fornece configuração, escrita e leitura separadas. Isso atende aos acessos usados atualmente, mas não ao conjunto mínimo de operações declarado no PDF. Também falta explicitar no contrato público a restrição de concorrência e de chamadas em interrupção.

4. **Falhas durante a inicialização encerram a execução antes do tratamento da aplicação.**  
   Em [main.c:79](/home/julia/Documentos/fabricio/esp32_framework/main/main.c:79), falhas de inicialização do sensor e do display acionam `assert`. Como essa inicialização já realiza comunicação I2C, uma falha nessa etapa não chega à política de repetição, indicação e registro implementada na aplicação. O tratamento previsto funciona para falhas durante a aquisição; o comportamento de falha na partida precisa ser definido e alinhado ao texto.

5. **A UART exige uma configuração de integração que não está documentada.**  
   O BSP seleciona UART0, enquanto o [porte](/home/julia/Documentos/fabricio/esp32_framework/hal/ports/esp32-wroom-32/uart/hal_uart.c:156) rejeita a instância usada pelo console do ESP-IDF. Se o console estiver configurado nessa UART, a inicialização falhará. Não encontrei um `sdkconfig.defaults` que estabeleça uma configuração compatível.

6. **Há uma fragilidade de portabilidade na formatação da GUI.**  
   [render.c:57](/home/julia/Documentos/fabricio/esp32_framework/app/gui/render.c:57) utiliza `%lu` com argumentos `uint32_t`. A checagem no computador gerou avisos de incompatibilidade de tipos. Como o tipo subjacente pode variar entre compiladores, convém usar `PRIu32`, como a telemetria já faz.

A seção 3.6 permanece pendente: não encontrei registros dos cenários funcionais, resultados de conformidade, comparação por hashes, cálculo de código compartilhado ou relatórios pareados de flash e RAM. O PDF condiciona a demonstração da solução a essas evidências nas **oito versões** — quatro com framework e quatro convencionais.

Não alterei os arquivos. Executei inspeção das dependências e checagem sintática do código comum; o build completo não pôde ser realizado porque o ESP-IDF não está disponível neste ambiente. Não validei o comportamento em hardware.

**A classificação adequada hoje é “implementação parcial da proposta, com a arquitetura e a prova de conceito concentradas no ESP32”.** Para avançar, as prioridades são corrigir o bloqueio de compilação, alinhar os contratos ao PDF, validar a versão ESP32 e completar os demais portes, versões convencionais e evidências experimentais.

## User

Essa proposta de implementação é de um TCC que tem como objetivos desenvolver e arquiteturar um framework que facilite a portabilidade e reusabilidade de código entre projetos.

Sabendo disso, analise criticamente as minhas colocações:
Sobre a implementação do timer, eu optei por implementar dessa forma para permitir que o usuário tenha mais flexibilidade na hora de configurar o timer, podendo escolher o período e um callback para ser chamada ao expirar esse período. Dessa forma ficaria a cargo da composição fazer essa seleção de período.

Sobre a implementação do i2c, eu optei por remover essa função de transação combinada, pois não é necessário para aplicação.

## Assistant

**As duas decisões são defensáveis para o objetivo do TCC, mas exigem justificativas diferentes.** Remover a transação combinada reduz o escopo da interface; substituir a consulta de tempo por callbacks muda o modelo de temporização oferecido pelo framework. Minha análise anterior identificou divergências em relação ao PDF — isso, por si só, não significa que as escolhas de implementação sejam inadequadas.

**Sobre o timer**, atribuir à composição a seleção do período e do callback é coerente com a arquitetura. A integração conhece os recursos da plataforma e conecta esses recursos ao código comum. Um timer periódico configurável também pode ser reutilizado em aplicações diferentes, sem incorporar ao porte decisões como intervalo de aquisição ou frequência de atualização da tela.

Entretanto, **flexibilidade de configuração não equivale automaticamente a portabilidade**. Quanto mais comportamentos a interface permite configurar, mais comportamentos os portes precisam oferecer de forma equivalente. Seu contrato precisa responder, por exemplo:

- Quais períodos e resoluções são suportados por cada porte?
- O que acontece quando o período solicitado não pode ser representado?
- O callback executa em interrupção em todas as plataformas?
- Uma execução atrasada provoca perda, aglutinação ou recuperação das expirações?
- Quais operações podem ser realizadas dentro do callback?

A interface atual já documenta parte dessas restrições, o que é positivo. A questão central é demonstrar que os outros portes conseguem respeitá-las.

Também existe uma diferença funcional importante: **uma notificação periódica e uma consulta ao tempo decorrido resolvem problemas relacionados, mas distintos**. Seu timer permite executar uma ação quando uma expiração é atendida; ele não garante, sozinho, uma contagem fiel do tempo físico. No código atual, incrementar um contador a cada callback produz uma contagem das notificações recebidas. Se alguma expiração não for entregue, o contador deixa de acompanhar o tempo decorrido.

Portanto, a justificativa “o usuário pode escolher o período” sustenta a configuração da HAL, mas não resolve completamente a necessidade de tempo ativo, debounce e aquisição periódica. Além disso, se o período deixar de ser 1 ms, o consumidor precisará receber uma contagem convertida corretamente para milissegundos.

Para o objetivo de reúso, há outra pergunta relevante: **cada projeto terá que reconstruir esse adaptador de tempo na composição?** Se vários projetos repetirem contador, sincronização e conversão de unidades, esse mecanismo pode merecer um serviço reutilizável de temporização. A composição continuaria escolhendo e conectando os recursos; o serviço encapsularia o mecanismo comum. Isso não obriga a abandonar seu timer por callback nem a oferecer duas APIs imediatamente.

Minha avaliação é que **o desenho do timer é arquiteturalmente válido**, mas a justificativa deveria apresentar a mudança como escolha de uma primitiva de notificações periódicas, explicar suas garantias e indicar como a aplicação obtém a referência temporal de que necessita. A proposta também deve deixar de prometer consulta ao tempo decorrido caso essa garantia não seja implementada.

**Sobre o I2C**, remover a transação combinada é coerente com o recorte declarado no próprio TCC: implementar os recursos necessários à prova de conceito. Um framework não precisa começar com todas as operações possíveis para ser modular e reutilizável.

Contudo, eu fortaleceria sua justificativa. Dizer apenas “não é necessário para a aplicação” pode dar a impressão de que a HAL foi desenhada exclusivamente para esse produto. Uma formulação mais precisa seria:

> A interface I2C foi limitada às operações necessárias aos componentes selecionados para a prova de conceito. A transação combinada ficou fora do escopo inicial, preservando-se uma interface de leitura e escrita independente dos dispositivos utilizados.

Essa distinção importa: **selecionar operações a partir dos casos de uso é delimitar escopo; incorporar endereços, comandos ou políticas desses dispositivos no porte seria acoplar a implementação à aplicação.** Seu código mantém essa separação.

A limitação precisa ficar explícita: uma escrita seguida de leitura, cada uma terminada com `STOP`, não substitui uma transação combinada com `repeated START`. Assim, componentes que exigirem essa sequência não poderão ser atendidos pelo contrato atual sem uma extensão. Isso limita a abrangência demonstrada pelo framework, mas não invalida seu reúso para os dispositivos suportados.

Eu manteria essa remoção, atualizaria o Quadro 3 e registraria a limitação. Não há necessidade de implementar uma operação sem uso apenas para ampliar a API.

Para o TCC, o argumento mais sólido é: **as interfaces foram deliberadamente delimitadas, e a portabilidade e o reúso serão avaliados dentro desse conjunto de capacidades**. A qualidade da solução dependerá de preservar o código comum e demonstrar contratos equivalentes nas plataformas selecionadas, sem apresentar essa evidência como suporte geral a qualquer aplicação.
