#include <driver/gpio.h>  
#include "driver/i2c_master.h" 
#include "driver/i2c.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "driver/uart.h"
#include <string.h>
#include <math.h>
#include "kalman_filter.h"
#include "lte.h"

// I2C pin configuration
#define I2C_SCL_PIN GPIO_NUM_42   // I2C clock (SCL) pin
#define I2C_SDA_PIN GPIO_NUM_21   // I2C data (SDA) pin
#define I2C_FREQUENCY 100000     // I2C frequency in Hz

// I2C address of the microphone
#define MIC_I2C_ADDRESS 0x1A  // Replace with your microphone's I2C address

// Buffer for storing captured audio samples (assumed 32-bit data for simplicity)
#define SAMPLE_BUFFER_SIZE 512
int32_t sample_buffer[SAMPLE_BUFFER_SIZE];

// --- UART Configuration for EC25/EG25-G ---
#define UART_NUM UART_NUM_1
#define TX_PIN GPIO_NUM_17
#define RX_PIN GPIO_NUM_16
#define UART_BAUD_RATE 115200
#define UART_BUFFER_SIZE (1024 * 2)

static const char *TAG = "ESP32_Kalman";

// --- Initialize I2C Master ---
void init_i2c_master() {
    // Configure the I2C master
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .clk_flags = 0,
    };

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x54,
        .scl_speed_hz = 400000,
    };

    // Initialize I2C master configuration
    i2c_param_config(I2C_NUM_0, &i2c_config);

    // Install the I2C driver
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

// Function to read microphone data via I2C
esp_err_t read_microphone_data(uint8_t* data, size_t length) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t ret;

    // Write the register address to start reading data
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MIC_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true);  // Example register address
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send I2C write command");
        return ret;
    }

    // Now read the data from the microphone
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MIC_I2C_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, length, I2C_MASTER_ACK);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return ret;
}

// Task for reading and processing I2C data
void i2c_capture_task(void *param) {
    while (true) {
        // Read raw data from the microphone into the buffer
        esp_err_t ret = read_microphone_data(reinterpret_cast<uint8_t*>(sample_buffer), sizeof(sample_buffer));
        
        if (ret == ESP_OK) {
            size_t sample_count = sizeof(sample_buffer) / sizeof(int32_t);
            
            // Log the captured samples
            for (size_t i = 0; i < sample_count; ++i) {
                ESP_LOGI(TAG, "Sample[%d]: %ld", static_cast<int>(i), sample_buffer[i]);
            }
        } else {
            ESP_LOGE(TAG, "Error reading microphone data");
        }

        // Prevent task starvation by adding a small delay
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// --- LTE Initialization Task ---
void lte_task(void *param) {
    initialise_lte(param, TAG);
}

// --- Initialize UART for Quectel EC25/EG25-G ---
void init_uart() {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, UART_BUFFER_SIZE, UART_BUFFER_SIZE, 0, NULL, 0);
}

void sensor_fusion_task(void *param) {
    while (true) {
        float radar_measurement = generate_radar_measurement();
        float sound_measurement = generate_sound_measurement();
        float fused_measurement = (radar_measurement + sound_measurement) / 2.0;
        kalman_filter(fused_measurement);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Main application entry point
extern "C" void app_main() {
    // Initialize I2C interface
    ESP_LOGI(TAG, "Initializing I2C master...");
    init_i2c_master();

    // Initialize LTE UART
    init_uart();

    // Start LTE Task
    xTaskCreate(lte_task, "LTE Task", 4096, NULL, 5, NULL);

    // Start the task for capturing I2C data
    ESP_LOGI(TAG, "Starting I2C capture task...");
    xTaskCreate(i2c_capture_task, "I2C Capture Task", 4096, NULL, 1, NULL);

    xTaskCreate(sensor_fusion_task, "Sensor Fusion Task", 4096, NULL, 1, NULL);
    
}
