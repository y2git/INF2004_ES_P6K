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

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/timer.h"

// GPIO Definitions
#define ADC_PIN 26   // GP26 for ADC input
#define PWM_PIN 7    // GP7 for PWM signal measurement
#define PULSE_PIN 2  // GP2 for pulse input

// Constants for measurements
#define NUM_PULSES 10
#define ADC_SAMPLE_RATE_MS 5

// Global Variables
volatile int pulse_count = 0;
volatile uint32_t pulse_times[NUM_PULSES];
volatile bool pulse_complete = false;
uint16_t adc_result = 0;
float analog_frequency = 0.0;
float pwm_frequency = 0.0;
float pwm_duty_cycle = 0.0;

// ADC Sampling Timer Callback
bool sample_adc_callback(repeating_timer_t* rt) {
    static uint16_t last_adc_result = 0;
    static uint32_t last_crossing_time = 0;

    adc_result = adc_read();  // Read ADC value

    // Detect zero crossings for frequency calculation
    if ((adc_result > 2048 && last_adc_result <= 2048) || (adc_result < 2048 && last_adc_result >= 2048)) {
        uint32_t current_time = time_us_32();
        if (last_crossing_time > 0) {
            uint32_t period_us = current_time - last_crossing_time;
            analog_frequency = 1e6 / (float)period_us;  // Frequency in Hz
        }
        last_crossing_time = current_time;
    }
    last_adc_result = adc_result;

    return true;
}

// PWM Measurement on GP7 using IRQ for rising and falling edges
void pwm_isr(uint gpio, uint32_t events) {
    static uint32_t rise_time = 0, fall_time = 0, period_time = 0;

    if (events & GPIO_IRQ_EDGE_RISE) {
        rise_time = time_us_32();
        if (fall_time > 0) {
            period_time = rise_time - fall_time;
            if (period_time > 0) {
                pwm_frequency = 1e6 / period_time;  // Frequency in Hz
            }
        }
    }
    else if (events & GPIO_IRQ_EDGE_FALL) {
        fall_time = time_us_32();
        if (rise_time > 0) {
            uint32_t high_time = fall_time - rise_time;
            if (period_time > 0) {
                pwm_duty_cycle = (float)high_time / period_time * 100.0f;  // Duty cycle as percentage
            }
        }
    }
}

// Pulse Input Measurement on GP2
void pulse_isr(uint gpio, uint32_t events) {
    static uint32_t last_time = 0;
    uint32_t current_time = time_us_32();

    if (pulse_count < NUM_PULSES) {
        pulse_times[pulse_count] = current_time - last_time;
        last_time = current_time;
        pulse_count++;
        printf("Pulse %d captured at time %u us\n", pulse_count, current_time);
    }
    else {
        pulse_complete = true;
    }
}

char* buddy2() {
    // Setup ADC on GP26
    adc_init();
    adc_gpio_init(ADC_PIN);         // Initialize GP26 for ADC input
    adc_select_input(0);            // Select ADC channel 0 (GP26)

    // Timer for ADC sampling every 5ms (200 Hz sampling rate)
    repeating_timer_t adc_timer;
    add_repeating_timer_ms(ADC_SAMPLE_RATE_MS, sample_adc_callback, NULL, &adc_timer);

    // Setup PWM measurement on GP7
    gpio_set_irq_enabled_with_callback(PWM_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &pwm_isr);

    // Setup digital pulse input on GP2
    gpio_set_irq_enabled_with_callback(PULSE_PIN, GPIO_IRQ_EDGE_RISE, true, &pulse_isr);

    // Notify that the system is waiting for pulses
    printf("Waiting for pulses to be captured on GP2...\n");

    // Wait for pulse capture to complete
    while (!pulse_complete) {
        tight_loop_contents();  // Keeps the CPU busy
    }

    // Check if all pulses were captured
    if (pulse_count < NUM_PULSES) {
        printf("Warning: Only %d out of %d pulses were captured. Some pulses may have been missed.\n", pulse_count, NUM_PULSES);
    }
    else {
        printf("All %d pulses successfully captured.\n", NUM_PULSES);
    }

    // Display and analyze results
    printf("\n--- Week 10: ADC & PWM Results ---\n");
    static char results[5000] = "";
    const float conversion_factor = 3.3f / (1 << 12);
    // Store and display results for each pulse
    for (int i = 0; i < pulse_count; i++) {
        // Append formatted output for each pulse into the text buffer
        snprintf(results + strlen(results), sizeof(results) - strlen(results),
            "Pulse %d:\nRaw ADC Value: %d\nConverted Voltage: %.2f V\nPulse Width: %u us\n"
            "PWM Frequency: %.2f Hz\nDuty Cycle: %.2f%%\nAnalog Frequency: %.2f Hz\n\n",
            i + 1, adc_result, adc_result * conversion_factor, pulse_times[i],
            pwm_frequency, pwm_duty_cycle, analog_frequency);
    }
    //printf(results);
    return results;
}
