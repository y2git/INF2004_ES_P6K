#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/uart.h"

const uint BTN_PIN = 22; // button GP22
const uint UART1_TX_PIN = 8; // GPIO pin for UART1 TX
const uint UART1_RX_PIN = 9; // GPIO pin for UART1 RX

int main() {
    // Replace stdio_init_all with stdio_usb_init() if you experience random character outputs
    //stdio_init_all();
    stdio_usb_init();

    //Set buttons
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_set_pulls(BTN_PIN, true, false);

    //Set UART
    uart_init(uart1, 9600); // Initialize UART1 with a baud rate of 9600
    //uart_set_format(uart1, 8, 1, UART_PARITY_NONE);
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);

    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed.");
        return -1;
    }

    char alphabet = 'A'; // Start at 'A'

    while (true) {
        //Handle button press
        if (gpio_get(BTN_PIN)) {
            // Button is not pressed, send '1'
            printf("Sent: 1\n");
            uart_putc(uart1, '1');
        } else {
            // Button is pressed, send alphabet letters
            printf("Sent: %c\n", alphabet);  // Transmit current letter
            uart_putc(uart1, alphabet);
            alphabet++;  // Move to the next letter

            if (alphabet > 'Z') {
                alphabet = 'A';  // Loop back to 'A' after 'Z'
            }
        }
        \
        // Check for received data on UART1
        if (uart_is_readable(uart1)) {
            char received_char = uart_getc(uart1); // Read the received character

            if (received_char >= 'A' && received_char <= 'Z') {
                // Convert uppercase to lowercase
                received_char += 32; // 'a' - 'A' = 32
                printf("Received: %c\n", received_char);
            } else if (received_char == '1') {
                // Print '2' instead of '1'
                printf("Received: 2\n");
            }
        }
        else{
            printf("UART not readable\n");
        }

        sleep_ms(1000);  // 1-second delay between transmissions
    }
}
