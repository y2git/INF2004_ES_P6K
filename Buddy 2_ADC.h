#ifndef BUDDY2_ADC_H
#define BUDDY2_ADC_H

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

bool sample_adc_callback(repeating_timer_t* rt);

#endif // BUDDY2_ADC_H
