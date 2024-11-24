// Buddy 2: Digital/Analog Signal Monitor and Analysis 
/*
1. Analog Signal Monitoring:
    - signal digitization
    - frequency analysis (FFT)
    - signal metrics: rms, peak-to-peak, snr
2. Digital Signal Monitoring:
    - PWM detection
*/
/* Week 10 Deliverables: Capture 10 pulses accurately,
   measure and display the Analog/PWM frequency and duty cycle
*/

// Capture a sequence of 10 digital pulses on input pin GP2 (stored on microSD) 
// Exact 10 can be reproduced on output pin GP3 (executed twice) 
/* Analyze and Display Analog (GP26) and PWM (GP7) frequency (for both Analog and PWM)
and duty cycle (for PWM) */

//Updated from week 10
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define PULSE_PIN 2   // Pulses captured here

volatile uint32_t last_pulse_time = 0;  // Store the timestamp of the latest pulse
volatile uint32_t pulse_count = 0;      // Start counting from 1

// Function to convert microseconds timestamp to a human-readable time string
void format_time(uint32_t timestamp_us, char *buffer, size_t buffer_size) {
    uint32_t total_ms = timestamp_us / 1000;
    uint32_t hours = (total_ms / 3600000) % 24;
    uint32_t minutes = (total_ms / 60000) % 60;
    uint32_t seconds = (total_ms / 1000) % 60;
    uint32_t milliseconds = total_ms % 1000;

    snprintf(buffer, buffer_size, "%02u:%02u:%02u:%03u", hours, minutes, seconds, milliseconds);
}

// Interrupt Service Routine (ISR) for pulse measurement
void pulse_isr(uint gpio, uint32_t events) {
    uint32_t current_time = time_us_32();  // Capture the current time in microseconds
    last_pulse_time = current_time;  // Store the latest pulse timestamp
    pulse_count++;  // Increment the pulse count each time a pulse is captured
}

int main() {
    stdio_init_all();
    sleep_ms(1000);  // Allow time for serial initialization

    // Initialize pulse count to 1 before entering the loop
    pulse_count = 0;  // Ensure pulse counting starts from 1

    // Disable the interrupt temporarily to avoid initial pulses being counted
    gpio_set_irq_enabled(PULSE_PIN, GPIO_IRQ_EDGE_RISE, false);

    // Wait for a moment to ensure we don't capture any pulses before the first print
    sleep_ms(500);  // Allow time to stabilize

    // Enable the interrupt with the callback for capturing pulses
    gpio_set_irq_enabled_with_callback(PULSE_PIN, GPIO_IRQ_EDGE_RISE, true, &pulse_isr);

    printf("Waiting for pulses to be captured on GP2...\n");

    while (true) {
        // Check if a new pulse timestamp has been captured and print it
        if (last_pulse_time != 0) {
            char time_string[30];
            format_time(last_pulse_time, time_string, sizeof(time_string));

            // Print the formatted time along with the pulse count (starting from 1)
            printf("%s -> Pulse %d\n", time_string, pulse_count);

            last_pulse_time = 0;  // Reset the timestamp after it has been printed
        }

        tight_loop_contents();  // Let the CPU idle between updates
    }

    return 0;
}
