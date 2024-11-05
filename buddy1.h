#include <stdio.h>
#include "pico/stdlib.h"
#include "sd_card.h"
#include "ff.h"

// SD Card and File Variables
FRESULT fr;           // Holds the result of FatFS function calls, such as mounting or opening files.
FATFS fs;             // Filesystem object for the SD card.
FIL fil;              // File object used for reading/writing files.
int ret;              // Stores return values
char buf[100];        // Buffer used for reading data from the SD card.

#define PULSE_OUTPUT_PIN 3    // GPIO pin to output pulses
#define BUTTON_PIN 22         // GPIO pin for button input

// Function to initialize pulse output pin
static inline void init_pulse_output() {
    gpio_init(PULSE_OUTPUT_PIN);
    gpio_set_dir(PULSE_OUTPUT_PIN, GPIO_OUT);
}

// Function to initialize button pin with a pull-up resistor
static inline void init_button() {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
}

// Function to read a file and print its content over serial
void readfile(char filename[]) {
    // Open file for reading
    fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        while (true);
    }

    // Print every line in file over serial
    printf("Reading from file '%s':\r\n", filename);
    printf("---\r\n");
    while (f_gets(buf, sizeof(buf), &fil)) {
        printf(buf);
    }
    printf("\r\n---\r\n");

    // Close file
    fr = f_close(&fil);
    if (fr != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", fr);
        while (true);
    }
}

// Function to write text to a file on the SD card
void writefile(char filename[], char text[]) {
    // Open file for writing (overwrites existing content)
    fr = f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        while (true);
    }

    // Write text to file
    printf("Writing to file '%s':\r\n", filename);
    ret = f_printf(&fil, "%s\r\n", text);
    if (ret < 0) {
        printf("ERROR: Could not write to file (%d)\r\n", ret);
        f_close(&fil);
        while (true);
    }
    
    // Close file
    fr = f_close(&fil);
    if (fr != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", fr);
        while (true);
    }
}

// Function to generate pulses based on data read from a file
void generate_pulses_from_file(char filename[]) {
    uint32_t pulse_duration;
    
    // Open the file for reading
    fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK) {
        printf("ERROR: Could not open file (%s)\r\n", filename);
        return;
    }

    // Read each line and interpret as a pulse duration in microseconds
    while (f_gets(buf, sizeof(buf), &fil)) {
        pulse_duration = (uint32_t)atoi(buf); // Convert string to integer (microseconds)

        // Generate pulse on PULSE_OUTPUT_PIN
        gpio_put(PULSE_OUTPUT_PIN, 1);    // Start pulse
        sleep_us(pulse_duration);         // Hold for pulse duration
        gpio_put(PULSE_OUTPUT_PIN, 0);    // End pulse
        sleep_us(100);                    // Small delay between pulses (100 µs)
    }

    // Close the file after pulse generation is complete
    fr = f_close(&fil);
    if (fr != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", fr);
    }
}

// Interrupt handler for button press
void button_press_callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_PIN) {
        generate_pulses_from_file("pulses.txt");  // Replace "pulses.txt" with the actual filename if different
    }
}

// Setup function to initialize GPIO and attach interrupt handler
void setup() {
    init_pulse_output();
    init_button();
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &button_press_callback);
}
