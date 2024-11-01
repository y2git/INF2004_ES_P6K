#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "ff_sd_spi.h" // CarlK3's library for SD card SPI handling

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

// Function prototypes
void initialize_pins();
void initialize_sd_card();
void play_pulses();
void detect_baud_rate();
void uart_rx_callback();

int main() {
    stdio_init_all();
    initialize_pins();
    initialize_sd_card();

    // Set up the UART for baud rate detection
    uart_init(UART_PORT, 9600); // Start with a default baud rate
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    irq_set_exclusive_handler(UART0_IRQ, uart_rx_callback);
    uart_set_irq_enables(UART_PORT, true, false);

    printf("System Initialized.\n");

    // Main loop
    while (1) {
        if (!gpio_get(BUTTON_PIN)) {
            // Button pressed, play pulses
            play_pulses();
        }

        // Call baud rate detection (you may want to call this less frequently in real applications)
        detect_baud_rate();
        sleep_ms(500);
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

void initialize_sd_card() {
    // Initialize the SD card via CarlK3's library
    if (sd_init_derived(SD_CARD_SPI_PORT, SD_CARD_MISO_PIN, SD_CARD_MOSI_PIN, SD_CARD_SCK_PIN, SD_CARD_CS_PIN) != FR_OK) {
        printf("SD Card initialization failed.\n");
    }

    // Mount the file system
    if (f_mount(&fs, "", 1) != FR_OK) {
        printf("SD Card mount failed.\n");
    } else {
        printf("SD Card mounted successfully.\n");
    }
}

// Read pulses from the file and output them on PULSE_OUTPUT_PIN
void play_pulses() {
    FIL file;
    char line[32];
    UINT br;

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
    } else {
        printf("Failed to open pulse file.\n");
    }
}

// Detect UART baud rate
void detect_baud_rate() {
    uint32_t baud_rate = uart_get_baudrate(UART_PORT);
    if (baud_rate != detected_baud_rate) {
        detected_baud_rate = baud_rate;
        printf("Detected baud rate: %u\n", detected_baud_rate);
    }
}

// UART RX interrupt handler (e.g., for handling received data if needed)
void uart_rx_callback() {
    while (uart_is_readable(UART_PORT)) {
        char ch = uart_getc(UART_PORT);
        // Handle received character
    }
}