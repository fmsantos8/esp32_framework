# Versão utilizada na avaliação do TCC2

Esta publicação fixa a composição do firmware avaliado em 02/11/2026 e ensaiado em 07–08/11/2026. Aplicação, componentes, contratos e portes são obtidos pelos commits registrados nos submódulos. Os endereços em `.gitmodules` usam HTTPS para permitir obtenção pública; essa alteração de endereço não modifica os fontes C/H nem as opções de compilação.

```sh
git clone --recurse-submodules https://github.com/fmsantos8/esp32_framework.git
```

A configuração comparada é **default (-Og)**. Nos STM32, a configuração da avaliação utiliza `-Og -g3` e newlib-nano. A construção da HAL utiliza assertions habilitadas, conforme o projeto.

As evidências, instruções de preparação dos SDKs e correspondência entre os arquivos históricos e a publicação estão em [testes_framework](https://github.com/fmsantos8/testes_framework). As revisões Git de origem dos builds permanecem no manifesto histórico; o manifesto de publicação identifica os commits posteriores que preservam esse conteúdo.
