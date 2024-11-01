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
    setup_pins();

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
    printf("--- Week 10: ADC & PWM Results ---\n");
    for (int i = 0; i < NUM_PULSES; i++) {
        uint16_t adc_value = adc_read();
        float voltage = (adc_value / 4095.0) * 3.3;
        unsigned int pulse_width = pulse_timestamps[i + 1] - pulse_timestamps[i];
        measure_pwm_signal();
        
        printf("Pulse %d:\n", i + 1);
        printf("  Raw ADC Value: %u\n", adc_value);
        printf("  Converted Voltage: %.2f V\n", voltage);
        printf("  Pulse Width: %u microseconds\n", pulse_width);
        printf("  PWM Frequency: %.2f Hz\n", analog_freq);
        printf("  Duty Cycle: %.2f %%\n", (pulse_width / (float)pulse_timestamps[i]) * 100);
    }
    printf("RMS: %.2f V\n", rms_value);
    printf("Peak-to-Peak: %.2f V\n", peak_to_peak);
    printf("SNR: %.2f dB\n", snr);


    // Unmount drive
    f_unmount("0:");

    // Loop forever doing nothing
    while (true) {
        sleep_ms(1000);
    }
}