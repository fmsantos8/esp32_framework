# Pinout — ESP32 DevKit V1

| Função | Pino do ESP32 | Ligação externa |
| --- | --- | --- |
| Alimentação | `3V3` | VCC do SHT31-D e do display SSD1306; resistor de pull-up do botão |
| Terra | `GND` | GND dos módulos, do botão, do LED e do adaptador serial externo, se usado |
| I²C0 SDA | `GPIO21` | SDA do SHT31-D e SDA do SSD1306 |
| I²C0 SCL | `GPIO22` | SCL do SHT31-D e SCL do SSD1306 |
| UART0 TX | `GPIO1` | RX do adaptador serial externo |
| UART0 RX | `GPIO3` | TX do adaptador serial externo |
| Botão | `GPIO18` | Botão normalmente aberto para GND; resistor externo de `10 kΩ` entre `GPIO18` e `3V3` |
| LED | `GPIO19` | Resistor externo de `1 kΩ` em série com o ânodo do LED; cátodo para GND |

## Barramento I²C

O SHT31-D e o SSD1306 compartilham `GPIO21` (SDA) e `GPIO22` (SCL) no barramento I²C0 a `100 kHz`. Os endereços I²C de 7 bits são `0x44` para o SHT31-D e `0x3C` para o SSD1306.

## Botão e LED

O botão é ativo em nível baixo: solto, o resistor de `10 kΩ` mantém `GPIO18` em `3,3 V`; pressionado, o botão liga o pino ao GND. O LED é ativo em nível alto: `GPIO19` em nível alto acende o LED através do resistor de `1 kΩ`.

## Comunicação serial

A UART0 usa `115200 8N1`. Para monitoramento pela conexão USB da DevKit V1, utilize a interface serial da própria placa.