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
//#include "lib/kissfft/kiss_fft.h" // KISS FFT library

// GPIO Definitions
#define ADC_PIN 26   // GP26 for ADC input
//#define PULSE_OUTPUT_PIN 3        // GP3 for digital pulse output
#define ANALOG_PIN 26               // GP26 for analog signal
#define PWM_PIN 7                   // GP7 for PWM signal measurement
#define PULSE_PIN 2                 // GP2 for pulse input

// Constants for measurements
#define NUM_PULSES 10             // Number of digital pulses to capture
#define PLAYBACK_COUNT 2          // Number of times to playback captured pulses
#define ADC_SAMPLE_RATE_MS 5

//#define FFT_SIZE 256              // FFT size for frequency analysis
//#define ADC_SAMPLING_RATE 5000    // ADC sampling rate for FFT


// Global Variables
volatile int pulse_count = 0;
volatile uint32_t pulse_times[NUM_PULSES];
volatile bool pulse_complete = false;
uint16_t adc_result = 0;
float analog_frequency = 0.0;
float pwm_frequency = 0.0;
float pwm_duty_cycle = 0.0;

//uint32_t pulse_timestamps[NUM_PULSES]; // Array to store timestamps of captured pulses
//uint8_t pulse_count = 0;               // Counter for captured pulses
//bool recording = true;                 // Flag to indicate recording state
//static int16_t adc_samples[FFT_SIZE];  // Store ADC samples globally for analysis

// Interrupt handler for capturing digital pulses on GP2
void pulse_capture_callback(uint gpio, uint32_t events) {
    if (recording && pulse_count < NUM_PULSES) {
        pulse_timestamps[pulse_count++] = to_ms_since_boot(get_absolute_time());
    }
}

// Playback captured pulses on GP3
//void playback_pulses() {
//    for (int i = 0; i < PLAYBACK_COUNT; i++) {
//        for (int j = 0; j < NUM_PULSES - 1; j++) {
//            gpio_put(PULSE_OUTPUT_PIN, 1);
//            sleep_ms(pulse_timestamps[j + 1] - pulse_timestamps[j]);
//            gpio_put(PULSE_OUTPUT_PIN, 0);
//        }
//    }
//}

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

