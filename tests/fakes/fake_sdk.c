/**
 * @file fake_sdk.c
 * @author Fabricio Santos (fmsantos8@ucs.br)
 * @brief Simulated SDK behavior for host contract checks.
 *
 */
#include "fake_sdk.h"
#include <assert.h>
#include <string.h>

int64_t fake_time, fake_clock_step, fake_uart_wait_duration;
esp_err_t fake_gpio_error, fake_uart_wait_error, fake_i2c_error;
size_t fake_i2c_calls, fake_i2c_device_count, fake_start_count, fake_stop_count, fake_read_count;
uint8_t fake_addresses[2];
uint32_t fake_initial_level;
size_t fake_uart_accepted;
size_t fake_uart_chunk=128;
int fake_last_timeout;
static bool installed[UART_NUM_MAX];
struct fake_bus {int instance;};
struct fake_device {int instance;};
static struct fake_bus buses[I2C_NUM_MAX];
static struct fake_device devices[I2C_NUM_MAX];

int64_t esp_timer_get_time(void) {int64_t result=fake_time; fake_time+=fake_clock_step; return result;}
esp_err_t gpio_set_level(gpio_num_t pin, uint32_t level) {(void)pin; fake_initial_level=level; return fake_gpio_error;}
int gpio_get_level(gpio_num_t pin) {(void)pin; return 1;}
esp_err_t gpio_config(const gpio_config_t *config) {(void)config; return fake_gpio_error;}
bool uart_is_driver_installed(uart_port_t controller) {return installed[controller];}
esp_err_t uart_driver_install(uart_port_t controller, int receive_size, int transmit_size,
                              int queue_size, void *queue, int flags)
{(void)receive_size;(void)queue_size;(void)queue;(void)flags; assert(transmit_size==0); installed[controller]=true;return ESP_OK;}
esp_err_t uart_driver_delete(uart_port_t controller) {installed[controller]=false;return ESP_OK;}
esp_err_t uart_param_config(uart_port_t controller, const uart_config_t *config)
{(void)controller;assert(config->data_bits==7 || config->data_bits==8);return ESP_OK;}
esp_err_t uart_set_pin(uart_port_t controller, int transmit, int receive, int rts, int cts)
{(void)controller;(void)transmit;(void)receive;(void)rts;(void)cts;return ESP_OK;}
int uart_tx_chars(uart_port_t controller, const char *buffer, uint32_t size)
{(void)controller;(void)buffer;size_t accepted=size<fake_uart_chunk?size:fake_uart_chunk;fake_uart_accepted+=accepted;return (int)accepted;}
esp_err_t uart_wait_tx_done(uart_port_t controller, uint32_t ticks)
{(void)controller;assert(ticks<portMAX_DELAY);fake_time+=fake_uart_wait_duration;return fake_uart_wait_error;}
esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *config, i2c_master_bus_handle_t *bus)
{if(fake_i2c_error!=ESP_OK)return fake_i2c_error; buses[config->i2c_port].instance=config->i2c_port;*bus=&buses[config->i2c_port];return ESP_OK;}
esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t bus) {(void)bus;return ESP_OK;}
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus, const i2c_device_config_t *config, i2c_master_dev_handle_t *device)
{assert(config->device_address==I2C_DEVICE_ADDRESS_NOT_USED);devices[bus->instance].instance=bus->instance;*device=&devices[bus->instance];++fake_i2c_device_count;return ESP_OK;}
esp_err_t i2c_master_execute_defined_operations(i2c_master_dev_handle_t device,
    i2c_operation_job_t *operations, size_t count, int timeout)
{
    (void)device;assert(timeout>0);fake_last_timeout=timeout;++fake_i2c_calls;
    fake_start_count=fake_stop_count=fake_read_count=0;
    bool expect_address=false;size_t address_count=0;
    for(size_t index=0;index<count;++index) {
        i2c_operation_job_t *operation=&operations[index];
        if(operation->command==I2C_MASTER_CMD_START) {++fake_start_count;expect_address=true;}
        else if(operation->command==I2C_MASTER_CMD_STOP) {++fake_stop_count;}
        else if(operation->command==I2C_MASTER_CMD_WRITE && expect_address) {
            assert(operation->write.ack_check && operation->write.total_bytes==1 && address_count<2);
            fake_addresses[address_count++]=*operation->write.data;expect_address=false;
        } else if(operation->command==I2C_MASTER_CMD_READ) {
            ++fake_read_count;memset(operation->read.data,0x42,operation->read.total_bytes);
            if(index+1<count && operations[index+1].command==I2C_MASTER_CMD_STOP)
                assert(operation->read.ack_value==I2C_NACK_VAL && operation->read.total_bytes==1);
        }
    }
    return fake_i2c_error;
}
