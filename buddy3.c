#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "ff.h"  // FatFS library for file handling

// Pin definitions
#define PULSE_OUTPUT_PIN 3
#define BUTTON_PIN 22
#define SD_CARD_SPI_PORT spi0
#define SD_CARD_MISO_PIN 16
#define SD_CARD_MOSI_PIN 19
#define SD_CARD_SCK_PIN 18
#define SD_CARD_CS_PIN 17
#define UART_PORT uart0
#define UART_RX_PIN 1

// File path on the SD card
#define PULSE_FILE_PATH "pulses.txt"

// UART baud rate variables
uint32_t detected_baud_rate = 0;

// FATFS object for SD card
FATFS fs;

// Function prototypes
void initialize_pins();
void initialize_sd_card();
void play_pulses();
uint32_t detect_baud_rate();
void uart_rx_callback();

int main() {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0); // Disable buffering for immediate output

    printf("Initializing pins...\n");
    initialize_pins();

    printf("Initializing SD card...\n");
    initialize_sd_card();

    printf("Detecting baud rate...\n");
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
            printf("Button pressed. Playing pulse sequence.\n");
            play_pulses();
            sleep_ms(100); // Debounce delay
        }

        sleep_ms(100); // Idle delay
    }
}

// Initialize GPIOs and UART
void initialize_pins() {
    gpio_init(PULSE_OUTPUT_PIN);
    gpio_set_dir(PULSE_OUTPUT_PIN, GPIO_OUT);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
}

// Initialize the SD card
void initialize_sd_card() {
    // Mount SD card and check if it's accessible
    if (f_mount(&fs, "", 1) != FR_OK) {
        printf("SD Card mount failed.\n");
    }
    else {
        printf("SD Card mounted successfully.\n");
    }
}

// Read pulses from the file and output them on PULSE_OUTPUT_PIN
void play_pulses() {
    FIL file;
    char line[32];
    //UINT br;

    if (f_open(&file, PULSE_FILE_PATH, FA_READ) == FR_OK) {
        int pulse_count = 0;
        while (pulse_count < 10 && f_gets(line, sizeof(line), &file)) {
            int pulse_length = atoi(line);
            gpio_put(PULSE_OUTPUT_PIN, 1);
            sleep_us(pulse_length);
            gpio_put(PULSE_OUTPUT_PIN, 0);
            sleep_us(pulse_length);
            pulse_count++;
        }
        f_close(&file);
    }
    else {
        printf("Failed to open pulse file.\n");
    }
}

// Custom baud rate detection function
uint32_t detect_baud_rate() {
    absolute_time_t start, end;
    uint32_t duration_us;
    absolute_time_t timeout = make_timeout_time_ms(1000); // 1-second timeout

    // Temporarily configure UART RX pin as GPIO input for baud rate detection
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(UART_RX_PIN, GPIO_IN);

    printf("Waiting for start bit...\n");

    // Wait for the line to go low (start bit) or timeout
    while (gpio_get(UART_RX_PIN) == 1) {
        if (absolute_time_diff_us(get_absolute_time(), timeout) <= 0) {
            printf("Baud rate detection timeout. No start bit detected.\n");
            return 9600; // Return a default baud rate on timeout
        }
        tight_loop_contents();
    }
    printf("Start bit detected, measuring baud rate...\n");

    // Measure the duration of the low pulse (start bit duration)
    start = get_absolute_time();
    while (gpio_get(UART_RX_PIN) == 0) {
        tight_loop_contents();
    }
    end = get_absolute_time();
    printf("End of start bit detected.\n");

    duration_us = absolute_time_diff_us(start, end);

    // Calculate baud rate: Baud rate = (1 / duration in seconds) * bits per second
    uint32_t baud_rate = 1000000 / duration_us; // Convert microseconds to Hz
    printf("Calculated baud rate: %u\n", baud_rate);

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
