#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "math.h"

#define UART_ID uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define BUFFER_SIZE 10  // Size of the circular buffer for rolling average
#define EXPECTED_BAUD_RATE 115200  // Reference baud rate for error calculation

// Circular buffer for storing bit times
uint32_t bit_times[BUFFER_SIZE] = {0};
int buffer_index = 0;
int sample_count = 0;
uint32_t start_time = 0;
uint32_t end_time = 0;
volatile bool edge_detected = false;

// GPIO interrupt callback function for detecting edges
void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == UART_RX_PIN) {
        if (events & GPIO_IRQ_EDGE_FALL) {
            start_time = time_us_32();
            edge_detected = true;
        } else if (events & GPIO_IRQ_EDGE_RISE && edge_detected) {
            end_time = time_us_32();
            uint32_t bit_time = end_time - start_time;
            edge_detected = false;

            bit_times[buffer_index] = bit_time;
            buffer_index = (buffer_index + 1) % BUFFER_SIZE;
            if (sample_count < BUFFER_SIZE) {
                sample_count++;
            }

            printf("Measured Bit Time: %u us\n", bit_time);
        }
    }
}

// Calculate rolling average of the bit times
float calculate_average_bit_time() {
    uint32_t sum = 0;
    for (int i = 0; i < sample_count; i++) {
        sum += bit_times[i];
    }
    return (sample_count > 0) ? (float)sum / sample_count : 0;
}

float calculate_expected_bit_time() {
    return 1000000.0 / EXPECTED_BAUD_RATE;
}

void identify_protocol() {
    float average_bit_time = calculate_average_bit_time();
    float expected_uart_bit_time = calculate_expected_bit_time();
    float expected_i2c_bit_time = 1000000.0 / 100000;
    float expected_spi_bit_time = 1000000.0 / 500000;

    printf("Performing protocol identification...\n");
    if (fabs(average_bit_time - expected_uart_bit_time) < 5) {
        printf("Identified protocol: UART\n");
    } else if (fabs(average_bit_time - expected_i2c_bit_time) < 5) {
        printf("Identified protocol: I2C\n");
    } else if (fabs(average_bit_time - expected_spi_bit_time) < 5) {
        printf("Identified protocol: SPI\n");
    } else {
        printf("Protocol not identified\n");
    }
}

void handle_web_commands(const char *command) {
    if (strcmp(command, "IDENTIFY_PROTOCOL") == 0) {
        identify_protocol();
    } else if (strcmp(command, "PRINT_STATUS") == 0) {
        printf("Command received: PRINT_STATUS\n");
        printf("Current Sample Count: %d\n", sample_count);
    } else {
        printf("Unknown command: %s\n", command);
    }
}

int main() {
    stdio_init_all();
    printf("Initialization complete\n");

    uart_init(UART_ID, 115200);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    gpio_set_irq_enabled_with_callback(UART_RX_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    char charData;
    char testCommand[20];
    int commandIndex = 0;

    while (true) {
        // Check if enough samples are collected to calculate the average
        if (sample_count == BUFFER_SIZE) {
            float average_bit_time = calculate_average_bit_time();
            float baud_rate = 1000000.0 / average_bit_time;
            float expected_bit_time = calculate_expected_bit_time();
            float percentage_error = (expected_bit_time > 0) ? (fabs(average_bit_time - expected_bit_time) / expected_bit_time) * 100 : 0;

            printf("Average Bit Time: %.2f us, Estimated Baud Rate: %.2f\n", average_bit_time, baud_rate);
            printf("Expected Bit Time: %.2f us, Percentage Error: %.2f%%\n", expected_bit_time, percentage_error);
            identify_protocol();
            sample_count = 0;
        }

        // Transmit 'A' to 'Z' continuously for testing
        static char currentLetter = 'A';
        uart_putc(UART_ID, currentLetter);
        printf("Sent: %c\n", currentLetter);
        currentLetter = (currentLetter == 'Z') ? 'A' : currentLetter + 1;

        // Check if data is available on the UART receiver
        if (uart_is_readable(UART_ID)) {
            charData = uart_getc(UART_ID);
            if (charData >= 'A' && charData <= 'Z') {
                charData += 32;
                printf("Received and converted: %c\n", charData);
            } else if (charData == '1') {
                printf("Received: 1, Printing: 2\n");
            } else {
                printf("Received: %c\n", charData);
            }
        }

        // Non-blocking command reading from serial input
        int c = getchar_timeout_us(0);  // Timeout of 0 for non-blocking
        if (c != PICO_ERROR_TIMEOUT) {
            if (c == '\n' || c == '\r') {
                testCommand[commandIndex] = '\0';  // Null-terminate the string
                handle_web_commands(testCommand);
                commandIndex = 0;  // Reset for the next command
            } else if (commandIndex < sizeof(testCommand) - 1) {
                testCommand[commandIndex++] = (char)c;
            }
        }

        sleep_ms(1000);
    }

    return 0;
}
