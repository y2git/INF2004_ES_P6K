#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/timer.h"

// Pin definitions
#define UART_RX_PIN 4       // GP4 as UART RX
#define UART_TX_PIN 5       // GP5 as UART TX
#define UART_PORT uart0
#define BUTTON_PIN 20       // GP20 as the button for changing baud rates

// Standard baud rates to cycle through
const uint32_t STANDARD_BAUD_RATES[] = {4800, 9600, 19200, 38400, 57600, 115200};
const int NUM_BAUD_RATES = sizeof(STANDARD_BAUD_RATES) / sizeof(STANDARD_BAUD_RATES[0]);

// Global variables
volatile uint32_t current_baud_rate_index = 0; // To track the current baud rate index

// Function prototypes
void initialize_uart(uint32_t baud_rate);
void initialize_pins();
void handle_button_press();
void uart_rx_callback();
void detect_baud_rate_and_error();

// Main function
int main() {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0); // Disable buffering for immediate output

    printf("Initializing pins...\n");
    initialize_pins();

    // Start with the first baud rate in the list
    initialize_uart(STANDARD_BAUD_RATES[current_baud_rate_index]);

    printf("Press button on GP20 to change the UART baud rate...\n");

    while (1) {
        // Check for button press to change the baud rate
        handle_button_press();

        // Perform baud rate detection in the main loop
        detect_baud_rate_and_error();

        sleep_ms(100); // Idle delay
    }
}

// Initialize GPIOs and UART
void initialize_pins() {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
}

// Initialize UART on the specified baud rate and set up RX interrupt
void initialize_uart(uint32_t baud_rate) {
    uart_init(UART_PORT, baud_rate);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_PORT, false, false);
    uart_set_format(UART_PORT, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_PORT, true);
    printf("UART initialized with baud rate: %u\n", baud_rate);

    // Set up UART RX interrupt
    //irq_set_exclusive_handler(UART0_IRQ, uart_rx_callback);
    //irq_set_enabled(UART0_IRQ, true);
    //uart_set_irq_enables(UART_PORT, true, false); // Enable RX interrupt, disable TX interrupt
}

// Handle button press to cycle through baud rates
void handle_button_press() {
    static bool button_pressed = false;

    if (!gpio_get(BUTTON_PIN) && !button_pressed) {  // Button press detected
        button_pressed = true;  // Set the flag to prevent multiple detections

        // Increment baud rate index
        current_baud_rate_index = (current_baud_rate_index + 1) % NUM_BAUD_RATES;

        // Get the new baud rate from the list
        uint32_t new_baud_rate = STANDARD_BAUD_RATES[current_baud_rate_index];

        // Reinitialize UART with the new baud rate
        initialize_uart(new_baud_rate);

        printf("Baud rate changed to: %u\n", new_baud_rate);

        sleep_ms(500); // Debounce delay
    } else if (gpio_get(BUTTON_PIN)) {
        // Reset the button_pressed flag when the button is released
        button_pressed = false;
    }
}

// UART RX interrupt callback function
//void uart_rx_callback() {
    // Check if there is data available in the RX FIFO
    //while (uart_is_readable(UART_PORT)) {
        // Read a character from the UART
        //char received_char = uart_getc(UART_PORT);

        // Display the received character
        //printf("Received: %c\n", received_char);

        // Check for any framing or parity errors
        //uint32_t errors = uart_get_hw(UART_PORT)->rsr;
        //if (errors & UART_UARTDR_FE_BITS) {
            //printf("Framing error detected!\n");
            //uart_get_hw(UART_PORT)->rsr = UART_UARTDR_FE_BITS; // Clear framing error flag
        //}
        //if (errors & UART_UARTDR_PE_BITS) {
            //printf("Parity error detected!\n");
            //uart_get_hw(UART_PORT)->rsr = UART_UARTDR_PE_BITS; // Clear parity error flag
        //}
    //}
//}

// Detect the UART baud rate and display the error margin and measurement time
void detect_baud_rate_and_error() {
    absolute_time_t start, end;
    uint32_t duration_us;
    float closest_error = 100.0;
    uint32_t detected_baud_rate = 9600;  // Default to 9600 if no closer match found

    // Send ASCII characters continuously for timing measurements
    static int ascii_char = 32; // Start with space character (ASCII 32)

    // If we have reached the end of printable ASCII, loop back
    if (ascii_char > 126) ascii_char = 32;

    uart_putc(UART_PORT, ascii_char++); // Send next ASCII character

    // Wait for a start of a byte and measure the time taken for an entire byte
    if (!uart_is_readable(UART_PORT)) return;  // Avoid blocking if no data

    // Capture the time for the beginning of the byte
    start = get_absolute_time();

    // Read one byte to mark the end of timing
    char byte_received = uart_getc(UART_PORT);

    // Capture the time after receiving the byte
    end = get_absolute_time();

    // Calculate duration in microseconds for one byte
    duration_us = absolute_time_diff_us(start, end);

    // Display the measured time
    printf("Time measured for byte: %u us\n", duration_us);

    // One byte (assuming 8N1) has 10 bits (start + 8 data + stop)
    float bit_duration_us = duration_us / 10.0;

    // Compare measured bit duration to standard baud rates
    for (int i = 0; i < NUM_BAUD_RATES; i++) {
        uint32_t baud = STANDARD_BAUD_RATES[i];
        float expected_bit_duration = 1000000.0 / baud;

        // Calculate error margin as a percentage
        float current_error_margin = ((float)abs((int)(bit_duration_us - expected_bit_duration)) / expected_bit_duration) * 100.0;

        // Find the closest matching baud rate
        if (current_error_margin < closest_error) {
            closest_error = current_error_margin;
            detected_baud_rate = baud;
        }
    }

    // Display detected baud rate and error margin
    printf("Detected baud rate: %u with error margin: %.2f%%\n", detected_baud_rate, closest_error);
}