#ifndef BUDDY2_PWM_H
#define BUDDY2_PWM_H

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

// Function prototypes
void pwm_isr(uint gpio, uint32_t events);
void process_pwm_data();
void setup_pwm_output(uint gpio, float frequency, float duty_cycle);

#endif // BUDDY2_PWM_H
