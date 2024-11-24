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

//Updated from week 10, FFT, RMS, Peak-to Peak, SNR added
#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "lib/kissfft/kiss_fft.h"

// GPIO Definitions
#define ADC_PIN 26  // GP26 for ADC input

// Constants for ADC sampling
#define ADC_SAMPLE_RATE_MS 1  // 1 ms sampling rate for 1 kHz sampling
#define ADC_THRESHOLD 2048    // Threshold for zero-crossing (mid-scale for 12-bit ADC)
#define FFT_SIZE 128          // FFT size, should be a power of 2
#define SAMPLE_COUNT 1000     // Number of samples for RMS and SNR calculation

// Global Variables
uint16_t adc_buffer[FFT_SIZE] = {0};  // Buffer for storing ADC samples
float analog_frequency = 0.0;
uint16_t adc_result = 0;
uint16_t last_adc_result = 0;
uint32_t last_crossing_time = 0;
float rms_value = 0.0;
float peak_to_peak = 0.0;
float snr_value = 0.0;
uint32_t sample_index = 0;

// Function Declarations
void calculate_metrics();
void fft_analysis();

// ADC Sampling Timer Callback
bool sample_adc_callback(repeating_timer_t* rt) {
    // Read ADC value
    adc_result = adc_read();

    // Store ADC value in the buffer for FFT
    adc_buffer[sample_index] = adc_result;

    // Increment the sample index and wrap around if necessary
    sample_index = (sample_index + 1) % FFT_SIZE;

    // Detect zero crossings for frequency calculation
    if ((adc_result > ADC_THRESHOLD && last_adc_result <= ADC_THRESHOLD) ||
        (adc_result < ADC_THRESHOLD && last_adc_result >= ADC_THRESHOLD)) {
        uint32_t current_time = time_us_32();
        if (last_crossing_time > 0) {
            uint32_t period_us = current_time - last_crossing_time;
            analog_frequency = 1e6 / (float)period_us;  // Frequency in Hz
        }
        last_crossing_time = current_time;
    }

    // Calculate RMS, peak-to-peak, and SNR every 1000 samples
    if (sample_index == 0) {
        calculate_metrics();
    }

    last_adc_result = adc_result;
    return true;
}

// Calculate RMS, Peak-to-Peak, and SNR
void calculate_metrics() {
    float sum_square = 0.0;
    float max_value = 0.0;
    float min_value = 4095.0; // Maximum possible ADC value is 4095 for 12-bit resolution
    float signal_noise = 0.0;
    float signal_sum = 0.0;

    // Calculate RMS, Peak-to-Peak, and Signal Noise
    for (int i = 0; i < FFT_SIZE; i++) {  // Looping over FFT_SIZE samples for metric calculation
        uint16_t sample = adc_buffer[i];
        sum_square += (float)(sample * sample);

        if (sample > max_value) max_value = sample;
        if (sample < min_value) min_value = sample;

        signal_sum += sample;
    }

    rms_value = sqrt(sum_square / FFT_SIZE);  // Use FFT_SIZE instead of SAMPLE_COUNT
    peak_to_peak = max_value - min_value;

    // Calculate Signal-to-Noise Ratio (SNR)
    signal_noise = signal_sum / FFT_SIZE;  // Signal mean for FFT_SIZE
    float noise_variance = 0.0;
    for (int i = 0; i < FFT_SIZE; i++) {  // Correcting loop size to match FFT_SIZE
        noise_variance += (adc_buffer[i] - signal_noise) * (adc_buffer[i] - signal_noise);
    }
    noise_variance /= FFT_SIZE;  // Use FFT_SIZE instead of SAMPLE_COUNT
    snr_value = 20 * log10(signal_noise / sqrt(noise_variance));

    // Perform FFT for frequency analysis
    fft_analysis();
}

// Perform FFT analysis on the digitized signal
void fft_analysis() {
    // Prepare input buffer for KISS FFT (complex numbers)
    kiss_fft_cpx fft_input[FFT_SIZE];
    for (int i = 0; i < FFT_SIZE; i++) {
        fft_input[i].r = (float)adc_buffer[i] - 2048.0;  // Centering the signal for 12-bit ADC (0-4095 range)
        fft_input[i].i = 0.0;  // Imaginary part is zero for real signals
    }

    // Create a kiss_fft_cfg (configuration) object
    kiss_fft_cfg cfg = kiss_fft_alloc(FFT_SIZE, 0, NULL, NULL);

    // Perform the FFT
    kiss_fft(cfg, fft_input, fft_input);

    // Find the dominant frequency
    float max_magnitude = 0.0;
    int max_index = 0;
    for (int i = 0; i < FFT_SIZE / 2; i++) { // Only positive frequencies
        float real = fft_input[i].r;
        float imag = fft_input[i].i;
        float magnitude = sqrt(real * real + imag * imag);
        if (magnitude > max_magnitude) {
            max_magnitude = magnitude;
            max_index = i;
        }
    }
    float dominant_frequency = (float)max_index * (1000.0 / FFT_SIZE);  // Sampling rate is 1 kHz
    printf("Dominant Frequency: %.2f Hz\n", dominant_frequency);
}

int main() {
    // Initialize stdio for serial communication
    stdio_init_all();
    sleep_ms(1000);  // Give time for serial initialization

    // Setup ADC on GP26
    adc_init();
    adc_gpio_init(ADC_PIN);  // Initialize GP26 for ADC input
    adc_select_input(0);     // Select ADC channel 0 (GP26)

    // Timer for ADC sampling every 1 ms (1 kHz sampling rate)
    repeating_timer_t adc_timer;
    add_repeating_timer_ms(ADC_SAMPLE_RATE_MS, sample_adc_callback, NULL, &adc_timer);

    // Main loop to keep the program running
    while (true) {
        printf("ADC Value: %d, Analog Frequency: %.2f Hz, RMS: %.2f, Peak-to-Peak: %.2f, SNR: %.2f dB\n",
            adc_result, analog_frequency, rms_value, peak_to_peak, snr_value);
        sleep_ms(200);  // Print results every 200 ms
    }

    return 0;
}
