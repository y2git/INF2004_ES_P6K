#ifndef BUDDY3_H
#define BUDDY3_H

#include "pico/stdlib.h"
#include "buddy1.h"  // Include buddy1 to access SD card functions for reading pulses

#define PULSE_OUTPUT_PIN 3
#define BUTTON_PIN 22

// Function to initialize pulse output on PULSE_OUTPUT_PIN
static inline void init_pulse_output() {
    gpio_init(PULSE_OUTPUT_PIN);
    gpio_set_dir(PULSE_OUTPUT_PIN, GPIO_OUT);
}

// Function to initialize button input on BUTTON_PIN with a pull-up resistor
static inline void init_button() {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
}

// Function to generate pulses based on data read from SD card
static inline void generate_pulses_from_sd() {
    uint32_t pulse_duration;

    // Initialize the SD card
    if (sd_card_init() != 0) {
        // Handle initialization error (e.g., log or return)
        return;
    }

    // Loop to read pulse data from SD card and output on PULSE_OUTPUT_PIN
    while (buddy1_read_pulse(&pulse_duration)) {  // Replace buddy1_read_pulse with actual function
        gpio_put(PULSE_OUTPUT_PIN, 1); // Start pulse
        sleep_us(pulse_duration);      // Duration of pulse
        gpio_put(PULSE_OUTPUT_PIN, 0); // End pulse
        sleep_us(pulse_duration);      // Interval between pulses
    }

    // Close the SD card after pulse generation is complete
    sd_card_close();
}

// Interrupt handler for button press
static inline bool button_press_callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_PIN) {
        generate_pulses_from_sd();
    }
    return true;
}

// Setup function to initialize GPIO and attach interrupt handler
static inline void setup() {
    init_pulse_output();
    init_button();
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &button_press_callback);
}

#endif // BUDDY3_H
