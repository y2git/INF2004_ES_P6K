#include <stdio.h>
#include "pico/stdlib.h"
#include "sd_card.h"
#include "ff.h"
#include "buddy1.h"
#include "buddy2.h"

int main() {
    char buf[100]; //A buffer used for reading data from the SD card.
    char filename[] = "test.txt"; //filename to be created
    char text[] = "blah blah blah"; //text to write to file
    //text = 'a';
    // Initialize chosen serial port
    stdio_init_all();

    printf("alive\n");
    // Wait for user to press 'enter' to continue
    // Baud rate 115200 on serial monitor
    // Line ending LF, CR or CRLF on serial monitor to send newline
    // or change start key to something else
    printf("\r\nSD card test. Press 'enter' to start.\r\n");
    while (true) {
        printf("Received: %c (%d)\r\n", buf[0], buf[0]);
        buf[0] = getchar();
        if ((buf[0] == '\r') || (buf[0] == '\n')) {
            break;
        }
    }

    // Initialize SD card
    if (!sd_init_driver()) {
        printf("ERROR: Could not initialize SD card\r\n");
    }

    // Mount drive
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) {
        printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
    }

    //main code
    writefile(filename, text);
    readfile(filename);

    //buddy2 stuff
    sleep_ms(1000); // Give time for serial initialization
   
    // setup_pins();
    // Setup ADC on GP26
    adc_init();
    adc_gpio_init(ADC_PIN);         // Initialize GP26 for ADC input
    adc_select_input(0);            // Select ADC channel 0 (GP26)

    //if (recording && pulse_count >= NUM_PULSES) {
    //    recording = false;
     //   playback_pulses();
    //}
    
    // Global Variables
    volatile int pulse_count = 0;
    volatile uint32_t pulse_times[NUM_PULSES];
    volatile bool pulse_complete = false;
    uint16_t adc_result = 0;
    float analog_frequency = 0.0;
    float pwm_frequency = 0.0;
    float pwm_duty_cycle = 0.0;



    //float analog_freq = measure_analog_frequency();
    //printf("Analog Frequency: %.2f Hz\n", analog_freq);
    
    //printf("Raw ADC Values: ");
    //for (int i = 0; i < FFT_SIZE; i++) {
    //    printf("%d ", adc_samples[i]);
    //}
    //printf("\n");
    
    //float rms_value = calculate_rms(adc_samples, FFT_SIZE);
    //float peak_to_peak = calculate_peak_to_peak(adc_samples, FFT_SIZE);
    //float signal_power = rms_value * rms_value;
    //float noise_power = 0.01;
    //float snr = calculate_snr(signal_power, noise_power);
    
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
    char text[4096] = "";
    const float conversion_factor = 3.3f / (1 << 12);
    // Store and display results for each pulse
    for (int i = 0; i < pulse_count; i++) {
        // Append formatted output for each pulse into the text buffer
        snprintf(text + strlen(text), sizeof(text) - strlen(text),
            "Pulse %d:\nRaw ADC Value: %d\nConverted Voltage: %.2f V\nPulse Width: %u us\n"
            "PWM Frequency: %.2f Hz\nDuty Cycle: %.2f%%\nAnalog Frequency: %.2f Hz\n\n",
            i + 1, adc_result, adc_result * conversion_factor, pulse_times[i],
            pwm_frequency, pwm_duty_cycle, analog_frequency);
    }

    // Print the entire content of the text buffer
     printf("%s", text);

    //printf("RMS: %.2f V\n", rms_value);
    //printf("Peak-to-Peak: %.2f V\n", peak_to_peak);
    //printf("SNR: %.2f dB\n", snr);


    // Unmount drive
    f_unmount("0:");

    // Loop forever doing nothing
    while (true) {
        sleep_ms(1000);
    }
}