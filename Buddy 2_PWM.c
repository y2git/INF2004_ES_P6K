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
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"

// GPIO Definitions
#define PWM_PIN 7  // GP7 for PWM signal measurement
#define PWM_OUT_PIN 15 // GP15 for PWM signal output (adjust as needed)

// Global Variables for PWM Measurement
volatile uint64_t high_time_accumulated = 0;
volatile uint64_t low_time_accumulated = 0;
volatile uint32_t cycle_count = 0;
float pwm_frequency = 0.0f;
float pwm_duty_cycle = 0.0f;

// ISR for PWM measurement on GP7 using IRQ for rising and falling edges
void pwm_isr(uint gpio, uint32_t events) {
    static uint64_t last_time = 0;
    uint64_t current_time = time_us_64();

    if (events & GPIO_IRQ_EDGE_RISE) {  // Rising edge detected
        if (last_time > 0) {  // Ensure we process only after the first edge
            low_time_accumulated += current_time - last_time;  // Accumulate low time
        }
        last_time = current_time;  // Update last time for high time measurement
    } else if (events & GPIO_IRQ_EDGE_FALL) {  // Falling edge detected
        if (last_time > 0) {  // Ensure we process only after the first edge
            high_time_accumulated += current_time - last_time;  // Accumulate high time
            cycle_count++;  // Increment cycle count on each full period
        }
        last_time = current_time;  // Update last time for low time measurement
    }
}

// Function to process PWM data and calculate average frequency and duty cycle
void process_pwm_data() {
    if (cycle_count > 0) {  // Ensure we have captured at least one full cycle
        uint64_t total_period = high_time_accumulated + low_time_accumulated;
        if (total_period > 0) {
            pwm_duty_cycle = ((float)high_time_accumulated / total_period) * 100.0f;
            pwm_frequency = (1e6 * cycle_count) / total_period;  // Frequency in Hz
        } else {
            pwm_duty_cycle = 0.0f;
            pwm_frequency = 0.0f;
        }

        // Reset accumulated times and cycle count for next measurement
        high_time_accumulated = 0;
        low_time_accumulated = 0;
        cycle_count = 0;
    }
}

// Function to set up PWM output on a specified pin
void setup_pwm_output(uint gpio, float frequency, float duty_cycle) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);  // Set GPIO pin to PWM mode
    uint pwm_slice = pwm_gpio_to_slice_num(gpio);  // Get the PWM slice number for the pin

    uint32_t clock_freq = 125000000;  // Default clock frequency for the Pico
    uint32_t wrap_value = clock_freq / frequency - 1;  // Calculate the wrap value for the desired frequency
    pwm_set_wrap(pwm_slice, wrap_value);

    uint32_t duty = (uint32_t)(duty_cycle * (wrap_value + 1) / 100);  // Scale duty cycle to the range of wrap value
    pwm_set_gpio_level(gpio, duty);  // Set the PWM duty cycle

    pwm_set_enabled(pwm_slice, true);  // Enable PWM output
}

int main() {
    // Initialize stdio for serial communication
    stdio_init_all();
    sleep_ms(1000);  // Give time for serial initialization

    // Set up GPIO for PWM measurement on GP7 (input)
    gpio_init(PWM_PIN);
    gpio_set_dir(PWM_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(PWM_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &pwm_isr);

    // Set up PWM output on GP15 (output)
    setup_pwm_output(PWM_OUT_PIN, 1000.0f, 50.0f);  // 1 kHz PWM with 50% duty cycle

    // Main loop to process and display PWM data
    while (true) {
        process_pwm_data();  // Process and update PWM frequency and duty cycle
        printf("PWM Frequency: %.2f Hz, Duty Cycle: %.2f%%\n", pwm_frequency, pwm_duty_cycle);
        sleep_ms(1000);  // Print results every second
    }

    return 0;
}
