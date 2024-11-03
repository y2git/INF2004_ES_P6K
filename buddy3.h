#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "ff.h"
#include "sd_card.h"

// Pin definitions
#define PULSE_OUTPUT_PIN 3
#define BUTTON_PIN 20
#define UART_PORT uart0
#define UART_RX_PIN 1

// File path on the SD card
#define PULSE_FILE_PATH "test.txt"

// UART baud rate variables
uint32_t detected_baud_rate = 0;

// Initialise Variables
FRESULT fr; //Holds the result of FatFS function calls, such as mounting or opening files.
FATFS fs; //filesystem object for the SD card.
FIL fil; //file object used for reading/writing files.
int ret; //Stores return values
char buf[100]; //A buffer used for reading data from the SD card.


// Initialize GPIOs and UART
void initialize_pins() {
    gpio_init(PULSE_OUTPUT_PIN);
    gpio_set_dir(PULSE_OUTPUT_PIN, GPIO_OUT);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
}

// Read pulses from the file and output them on PULSE_OUTPUT_PIN
void play_pulses() {
    char line[32];
    //UINT br;

    if (f_open(&fil, PULSE_FILE_PATH, FA_READ) == FR_OK) {
        int pulse_count = 0;
        while (pulse_count < 10 && f_gets(line, sizeof(line), &fil)) {
            int pulse_length = atoi(line);
            gpio_put(PULSE_OUTPUT_PIN, 1);
            sleep_us(pulse_length);
            gpio_put(PULSE_OUTPUT_PIN, 0);
            sleep_us(pulse_length);
            pulse_count++;
        }
        f_close(&fil);
    }
    else {
        printf("Failed to open pulse file.\n");
    }
}

// Custom baud rate detection function
uint32_t detect_baud_rate() {
    absolute_time_t start, end;
    uint32_t duration_us;

    // Temporarily configure UART RX pin as GPIO input for baud rate detection
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(UART_RX_PIN, GPIO_IN);

    // Wait for the line to go low (start bit)
    while (gpio_get(UART_RX_PIN) == 1) {
        tight_loop_contents();
    }

    // Measure the duration of the low pulse (start bit duration)
    start = get_absolute_time();
    while (gpio_get(UART_RX_PIN) == 0) {
        tight_loop_contents();
    }
    end = get_absolute_time();

    duration_us = absolute_time_diff_us(start, end);

    // Calculate baud rate: Baud rate = (1 / duration in seconds) * bits per second
    // For standard UART, 1 bit period = start bit duration
    uint32_t baud_rate = 1000000 / duration_us; // Convert microseconds to Hz

    return baud_rate;
}

// UART RX interrupt handler (e.g., for handling received data if needed)
void uart_rx_callback() {
    while (uart_is_readable(UART_PORT)) {
        char ch = uart_getc(UART_PORT);
        // Handle received character if needed
        printf("Received: %c\n", ch);
    }
}


int buddy3() {
    initialize_pins();
    printf("start");
    // Detect baud rate on UART RX pin
    detected_baud_rate = detect_baud_rate();
    printf("Detected baud rate: %u\n", detected_baud_rate);

    // Initialize UART with the detected baud rate
    uart_init(UART_PORT, detected_baud_rate);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Set up UART RX interrupt for handling incoming data
    irq_set_exclusive_handler(UART0_IRQ, uart_rx_callback);
    uart_set_irq_enables(UART_PORT, true, false);

    printf("System Initialized.\n");

    // Main loop
    while (1) {
        if (!gpio_get(BUTTON_PIN)) {
            // Button pressed, play pulses
            play_pulses();
            sleep_ms(200); // Debounce delay
        }

        sleep_ms(500); // Idle delay
    }
}

