#include <stdio.h>
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
            // Falling edge detected (start bit)
            start_time = time_us_32();
            edge_detected = true;
        } else if (events & GPIO_IRQ_EDGE_RISE && edge_detected) {
            // Rising edge detected (end of bit)
            end_time = time_us_32();
            uint32_t bit_time = end_time - start_time;  // Measured bit time in microseconds
            edge_detected = false;

            // Store the bit time in the circular buffer
            bit_times[buffer_index] = bit_time;
            buffer_index = (buffer_index + 1) % BUFFER_SIZE;
            if (sample_count < BUFFER_SIZE) {
                sample_count++;
            }

            // Print the measured bit time
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

// Calculate the expected bit time based on the reference baud rate
float calculate_expected_bit_time() {
    return 1000000.0 / EXPECTED_BAUD_RATE;  // Convert baud rate to bit time in microseconds
}

// Calculate the percentage error between measured and expected bit times
float calculate_percentage_error(float measured_bit_time, float expected_bit_time) {
    return (expected_bit_time > 0) ? (fabs(measured_bit_time - expected_bit_time) / expected_bit_time) * 100 : 0;
}

int main() {
    stdio_init_all();

    // Enable UART and set initial configurations
    uart_init(UART_ID, 115200);  // Set an initial arbitrary baud rate
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Set up GPIO interrupts for detecting falling and rising edges on RX pin
    gpio_set_irq_enabled_with_callback(UART_RX_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    char charData;

    while (true) {
        // Check if enough samples are collected to calculate the average
        if (sample_count == BUFFER_SIZE) {
            float average_bit_time = calculate_average_bit_time();
            float baud_rate = 1000000.0 / average_bit_time;  // Convert microseconds to baud rate
            float expected_bit_time = calculate_expected_bit_time();
            float percentage_error = calculate_percentage_error(average_bit_time, expected_bit_time);

            // Print the results
            printf("Average Bit Time: %.2f us, Estimated Baud Rate: %.2f\n", average_bit_time, baud_rate);
            printf("Expected Bit Time: %.2f us, Percentage Error: %.2f%%\n", expected_bit_time, percentage_error);

            // Reset sample count for the next capture cycle
            sample_count = 0;
        }

        // Transmit 'A' to 'Z' continuously for testing
        static char currentLetter = 'A';
        uart_putc(UART_ID, currentLetter);
        printf("Sent: %c\n", currentLetter);

        // Increment the letter, loop back to 'A' after 'Z'
        currentLetter = (currentLetter == 'Z') ? 'A' : currentLetter + 1;

        // Check if data is available on the UART receiver
        if (uart_is_readable(UART_ID)) {
            charData = uart_getc(UART_ID);

            if (charData >= 'A' && charData <= 'Z') {
                // Convert uppercase to lowercase and print
                charData += 32;  // Convert to lowercase
                printf("Received and converted: %c\n", charData);
            } else if (charData == '1') {
                // If '1' is received, print '2' instead
                printf("Received: 1, Printing: 2\n");
            } else {
                printf("Received: %c\n", charData);
            }
        }

        // Delay of 1 second between transmissions
        sleep_ms(1000);
    }

    return 0;
}
