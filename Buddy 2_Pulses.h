#ifndef BUDDY2_Pulses_H
#define BUDDY2_Pulses_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define PULSE_PIN 2   // Pulses captured here

volatile uint32_t last_pulse_time = 0;  // Store the timestamp of the latest pulse
volatile uint32_t pulse_count = 0;      // Start counting from 1

// Function prototypes
void format_time(uint32_t timestamp_us, char *buffer, size_t buffer_size);
void pulse_isr(uint gpio, uint32_t events);


#endif // BUDDY2_Pulses_H