# Contexto do framework

Para tarefas que criem, alterem ou avaliem módulos, dependências e responsabilidades do firmware, leia [a skill de arquitetura](.agents/skills/framework-architecture/SKILL.md). Ela distingue a proposta do TCC do estado atual desta árvore e indica as fronteiras entre aplicação, componentes, HAL, portes, BSP e integração.

# Estilo de comentários

Em arquivos `.c`, use sempre `//` nos comentários de implementação dentro de funções, inclusive nos comentários de várias linhas. A documentação de declarações de enums, tipos, variáveis e campos fica de fora dessa regra e mantém o padrão anterior: blocos `/** ... */` e comentários em linha começando com `/**`, sem `<`, mesmo quando a declaração estiver dentro de uma função.
