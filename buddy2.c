#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "kiss_fft.h" // KISS FFT library

#define PULSE_INPUT_PIN 2         // GP2 for digital pulse input
#define PULSE_OUTPUT_PIN 3        // GP3 for digital pulse output
#define ANALOG_PIN 26             // GP26 for analog signal
#define PWM_PIN 7                 // GP7 for PWM signal

#define NUM_PULSES 10             // Number of digital pulses to capture
#define PLAYBACK_COUNT 2          // Number of times to playback captured pulses
#define FFT_SIZE 256              // FFT size for frequency analysis
#define ADC_SAMPLING_RATE 5000    // ADC sampling rate for FFT

uint32_t pulse_timestamps[NUM_PULSES]; // Array to store timestamps of captured pulses
uint8_t pulse_count = 0;               // Counter for captured pulses
bool recording = true;                 // Flag to indicate recording state
static int16_t adc_samples[FFT_SIZE];  // Store ADC samples globally for analysis

// Interrupt handler for capturing digital pulses on GP2
void pulse_capture_callback(uint gpio, uint32_t events) {
    if (recording && pulse_count < NUM_PULSES) {
        pulse_timestamps[pulse_count++] = to_ms_since_boot(get_absolute_time());
    }
}

// Playback captured pulses on GP3
void playback_pulses() {
    for (int i = 0; i < PLAYBACK_COUNT; i++) {
        for (int j = 0; j < NUM_PULSES - 1; j++) {
            gpio_put(PULSE_OUTPUT_PIN, 1);
            sleep_ms(pulse_timestamps[j + 1] - pulse_timestamps[j]);
            gpio_put(PULSE_OUTPUT_PIN, 0);
        }
    }
}

// Analog frequency measurement using KISS FFT
float measure_analog_frequency() {
    kiss_fft_cpx in[FFT_SIZE], out[FFT_SIZE];
    kiss_fft_cfg cfg = kiss_fft_alloc(FFT_SIZE, 0, NULL, NULL);

    for (int i = 0; i < FFT_SIZE; i++) {
        adc_samples[i] = adc_read();
        in[i].r = adc_samples[i];
        in[i].i = 0;
        sleep_us(1000000 / ADC_SAMPLING_RATE);
    }

    kiss_fft(cfg, in, out);
    float max_magnitude = 0.0;
    int dominant_index = 0;
    for (int i = 1; i < FFT_SIZE / 2; i++) {
        float magnitude = sqrt(out[i].r * out[i].r + out[i].i * out[i].i);
        if (magnitude > max_magnitude) {
            max_magnitude = magnitude;
            dominant_index = i;
        }
    }

    free(cfg);
    return (float)dominant_index * ADC_SAMPLING_RATE / FFT_SIZE;
}

// Calculate RMS value of the signal
float calculate_rms(int16_t *samples, int size) {
    float sum_sq = 0.0;
    for (int i = 0; i < size; i++) {
        sum_sq += samples[i] * samples[i];
    }
    return sqrt(sum_sq / size);
}

// Calculate Peak-to-Peak voltage of the signal
float calculate_peak_to_peak(int16_t *samples, int size) {
    int16_t max_val = samples[0];
    int16_t min_val = samples[0];
    for (int i = 1; i < size; i++) {
        if (samples[i] > max_val) max_val = samples[i];
        if (samples[i] < min_val) min_val = samples[i];
    }
    return (float)(max_val - min_val);
}

// Calculate Signal-to-Noise Ratio (SNR)
float calculate_snr(float signal_power, float noise_power) {
    return 10 * log10(signal_power / noise_power);
}

// PWM frequency and duty cycle measurement on GP7
void measure_pwm_signal() {
    static uint32_t last_time = 0;
    static uint32_t period = 0;

    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (gpio_get(PWM_PIN) && last_time != 0) {
        period = current_time - last_time;
    }
    last_time = current_time;

    if (period > 0) {
        float frequency = 1000.0 / period;
        float duty_cycle = (float)(period / 2) / period * 100;

        printf("PWM Frequency: %.2f Hz\n", frequency);
        printf("PWM Duty Cycle: %.2f%%\n", duty_cycle);
    }
}

void setup_pins() {
    gpio_init(PULSE_INPUT_PIN);
    gpio_set_dir(PULSE_INPUT_PIN, GPIO_IN);
    gpio_pull_down(PULSE_INPUT_PIN);
    gpio_set_irq_enabled_with_callback(PULSE_INPUT_PIN, GPIO_IRQ_EDGE_RISE, true, &pulse_capture_callback);

    gpio_init(PULSE_OUTPUT_PIN);
    gpio_set_dir(PULSE_OUTPUT_PIN, GPIO_OUT);
    
    adc_init();
    adc_gpio_init(ANALOG_PIN);
}

int main() {
    stdio_init_all();
    setup_pins();

    while (1) {
        if (recording && pulse_count >= NUM_PULSES) {
            recording = false;
            playback_pulses();
        }
        
        float analog_freq = measure_analog_frequency();
        printf("Analog Frequency: %.2f Hz\n", analog_freq);
        
        printf("Raw ADC Values: ");
        for (int i = 0; i < FFT_SIZE; i++) {
            printf("%d ", adc_samples[i]);
        }
        printf("\n");
        
        float rms_value = calculate_rms(adc_samples, FFT_SIZE);
        float peak_to_peak = calculate_peak_to_peak(adc_samples, FFT_SIZE);
        float signal_power = rms_value * rms_value;
        float noise_power = 0.01;
        float snr = calculate_snr(signal_power, noise_power);

        printf("RMS: %.2f V\n", rms_value);
        printf("Peak-to-Peak: %.2f V\n", peak_to_peak);
        printf("SNR: %.2f dB\n", snr);

        measure_pwm_signal();

        sleep_ms(1000);
    }

    return 0;
}
