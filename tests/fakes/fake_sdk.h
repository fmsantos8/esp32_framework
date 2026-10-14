/**
 * @file fake_sdk.h
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Simulated SDK declarations for host contract checks.
 *
 */
#ifndef TEST_FAKE_SDK_H
#define TEST_FAKE_SDK_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef int esp_err_t;
enum {ESP_OK, ESP_FAIL, ESP_ERR_TIMEOUT, ESP_ERR_INVALID_ARG, ESP_ERR_NOT_SUPPORTED,
      ESP_ERR_NO_MEM, ESP_ERR_INVALID_STATE, ESP_ERR_NOT_FOUND, ESP_ERR_INVALID_RESPONSE};
typedef uint32_t TickType_t;
#define configTICK_RATE_HZ 100
#define portMAX_DELAY UINT32_MAX
#define SOC_UART_FIFO_LEN 128
#define SOC_UART_BITRATE_MAX 5000000
#define GPIO_IS_VALID_GPIO(pin) ((pin) < 40 && (pin) != 24 && ((pin) < 28 || (pin) > 31))
#define GPIO_IS_VALID_OUTPUT_GPIO(pin) (GPIO_IS_VALID_GPIO(pin) && (pin) < 34)
typedef int gpio_num_t;
enum {GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, GPIO_PULLUP_ENABLE,
      GPIO_PULLDOWN_DISABLE, GPIO_PULLDOWN_ENABLE, GPIO_INTR_DISABLE};
typedef struct {uint64_t pin_bit_mask; int mode, pull_up_en, pull_down_en, intr_type;} gpio_config_t;
esp_err_t gpio_set_level(gpio_num_t pin, uint32_t level);
int gpio_get_level(gpio_num_t pin);
esp_err_t gpio_config(const gpio_config_t *config);
int64_t esp_timer_get_time(void);

typedef int uart_port_t;
enum {UART_NUM_MAX=3, UART_DATA_7_BITS=7, UART_DATA_8_BITS=8,
      UART_PARITY_DISABLE=0, UART_PARITY_EVEN=2, UART_PARITY_ODD=3,
      UART_STOP_BITS_1=1, UART_STOP_BITS_2=2, UART_HW_FLOWCTRL_DISABLE=0,
      UART_SCLK_DEFAULT=0, UART_PIN_NO_CHANGE=-1};
typedef struct {int baud_rate, data_bits, parity, stop_bits, flow_ctrl, source_clk;} uart_config_t;
bool uart_is_driver_installed(uart_port_t controller);
esp_err_t uart_driver_install(uart_port_t controller, int receive_size, int transmit_size,
                              int queue_size, void *queue, int flags);
esp_err_t uart_driver_delete(uart_port_t controller);
esp_err_t uart_param_config(uart_port_t controller, const uart_config_t *config);
esp_err_t uart_set_pin(uart_port_t controller, int transmit, int receive, int rts, int cts);
int uart_tx_chars(uart_port_t controller, const char *buffer, uint32_t size);
esp_err_t uart_wait_tx_done(uart_port_t controller, uint32_t ticks);

enum {I2C_NUM_MAX=2, I2C_CLK_SRC_DEFAULT=0, I2C_ADDR_BIT_LEN_7=0,
      I2C_DEVICE_ADDRESS_NOT_USED=0xffff, I2C_ACK_VAL=0, I2C_NACK_VAL=1};
typedef struct fake_bus *i2c_master_bus_handle_t;
typedef struct fake_device *i2c_master_dev_handle_t;
typedef struct {
    int i2c_port, sda_io_num, scl_io_num, clk_source, glitch_ignore_cnt;
    struct {bool enable_internal_pullup;} flags;
} i2c_master_bus_config_t;
typedef struct {int dev_addr_length; uint16_t device_address; uint32_t scl_speed_hz;} i2c_device_config_t;
typedef enum {I2C_MASTER_CMD_START, I2C_MASTER_CMD_WRITE, I2C_MASTER_CMD_READ, I2C_MASTER_CMD_STOP} i2c_master_command_t;
typedef struct {
    i2c_master_command_t command;
    union {
        struct {bool ack_check; const uint8_t *data; size_t total_bytes;} write;
        struct {int ack_value; uint8_t *data; size_t total_bytes;} read;
    };
} i2c_operation_job_t;
esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *config, i2c_master_bus_handle_t *bus);
esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t bus);
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus, const i2c_device_config_t *config,
                                   i2c_master_dev_handle_t *device);
esp_err_t i2c_master_execute_defined_operations(i2c_master_dev_handle_t device,
    i2c_operation_job_t *operations, size_t count, int timeout);

extern int64_t fake_time;
extern int64_t fake_clock_step;
extern int64_t fake_uart_wait_duration;
extern esp_err_t fake_gpio_error;
extern esp_err_t fake_uart_wait_error;
extern esp_err_t fake_i2c_error;
extern size_t fake_i2c_calls;
extern size_t fake_i2c_device_count;
extern size_t fake_start_count, fake_stop_count, fake_read_count;
extern uint8_t fake_addresses[2];
extern uint32_t fake_initial_level;
extern size_t fake_uart_accepted;
extern size_t fake_uart_chunk;
extern int fake_last_timeout;
#endif
