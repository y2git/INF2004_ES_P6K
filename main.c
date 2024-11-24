#include "lwip/apps/httpd.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwipopts.h"
#include "ssi.h"
#include "cgi.h"
#include "swd_debugger.h"

#include "lwip/netif.h" // Access network interface information

#include "lwip/apps/sntp.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include <time.h>
#include <stdio.h>
#include <math.h>

#include "sd_card.h"
#include "ff.h"

#include "hardware/adc.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "lib/kissfft/kiss_fft.h"

#define NTP_SERVER_NAME "pool.ntp.org" // NTP server domain name

// SD Card and File Variables
FRESULT fr;           // Holds the result of FatFS function calls, such as mounting or opening files.
FATFS fs;             // Filesystem object for the SD card.
FIL fil;              // File object used for reading/writing files.
int ret;              // Stores return values
char buf[100];        // Buffer used for reading data from the SD card.

// GPIO Definitions
#define ADC_PIN 26  // GP26 for ADC input

// Constants for ADC sampling
#define ADC_SAMPLE_RATE_MS 1  // 1 ms sampling rate for 1 kHz sampling
#define ADC_THRESHOLD 2048    // Threshold for zero-crossing (mid-scale for 12-bit ADC)
#define FFT_SIZE 128          // FFT size, should be a power of 2
#define SAMPLE_COUNT 1000     // Number of samples for RMS and SNR calculation

// Global Variables (for ADC)
uint16_t adc_buffer[FFT_SIZE] = {0};  // Buffer for storing ADC samples
float analog_frequency = 0.0;
uint16_t adc_result = 0;
uint16_t last_adc_result = 0;
uint32_t last_crossing_time = 0;
float rms_value = 0.0;
float peak_to_peak = 0.0;
float snr_value = 0.0;
uint32_t sample_index = 0;

// Initialize standard input/output
void init_stdio()
{
    stdio_init_all();
}

// Initialize and connect to the Wi-Fi network
void init_wifi()
{
    cyw43_arch_init();
    cyw43_arch_enable_sta_mode();

    // Connect to the Wi-Fi network, looping until connected
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0)
    {
        printf("Attempting to connect...\n");
    }
    printf("Connected! \n");
}

// Initialize the web server
void init_http_server()
{
    httpd_init();
    printf("HTTP server initialized\n");
}

// Configure SSI and CGI handlers
void init_ssi_cgi_handlers()
{
    ssi_init();
    printf("SSI Handler initialized\n");
    cgi_init();
    printf("CGI Handler initialized\n");
}

// Print network interface information
void print_network_info()
{
    struct netif *netif = &cyw43_state.netif[0];

    // Check if the network interface is up and print the IP address
    if (netif_is_up(netif))
    {
        printf("Network interface is up. IP Address: %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
    }
    else
    {
        printf("Network interface is down.\n");
    }
}

// Function to set the system time using seconds and microseconds from SNTP
void set_time(uint32_t seconds, uint32_t microseconds) {
    struct timeval tv;
    tv.tv_sec = seconds;         // Set the seconds part from SNTP
    // tv.tv_sec = seconds + 5 * 3600;   // Adjust to UTC+5 (Add 5 hours)
    tv.tv_usec = microseconds;   // Set the microseconds part

    // Set the system time using the obtained values
    if (settimeofday(&tv, NULL) == 0) {
        printf("System time successfully updated to %ld seconds since the Epoch\n", (long)tv.tv_sec);
    } else {
        printf("Failed to set system time\n");
    }
}

void ntp_sync_callback(struct timeval *tv) {
    printf("NTP time synchronized successfully!\n");
}

void initialize_ntp_client() {
    int retry_count = 0;
    const int max_retries = 5;

    ip_addr_t dnsserver;
    IP4_ADDR(&dnsserver, 8, 8, 8, 8);
    dns_setserver(0, &dnsserver);

    while (retry_count < max_retries) {
        printf("Initializing NTP client (Attempt %d)...\n", retry_count + 1);

        sntp_setservername(0, NTP_SERVER_NAME);
        sntp_init();

        printf("NTP client initialized, syncing time from %s...\n", NTP_SERVER_NAME);

        for (int i = 0; i < 10; i++) {
            sleep_ms(1000);
            time_t now = time(NULL);
            if (now > 1609459200) {
                printf("Time synchronized successfully!\n");
                return;
            }
        }

        printf("Failed to synchronize time, retrying...\n");
        sntp_stop();
        retry_count++;
    }

    if (retry_count == max_retries) {
        printf("Failed to synchronize time after %d attempts.\n", max_retries);
    }
}

// buddy1 stuff ==================================
void init_sdcard(){
    // Initialize SD card
    if (!sd_init_driver()) {
        printf("ERROR: Could not initialize SD card\r\n");
    }

    // Mount drive
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) {
        printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
    }
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
// buddy1 stuff ==================================

// buddy2 stuff ==================================
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
// buddy2 stuff ==================================

// main
void main()
{
    init_stdio();            // Initialize standard I/O
    init_wifi();             // Initialize and connect to Wi-Fi
    init_http_server();      // Initialize the web server
    init_ssi_cgi_handlers(); // Configure SSI and CGI handlers
    print_network_info();    // Print network interface information

    initialize_ntp_client();

    init_sdcard();

    // Timer for ADC sampling every 1 ms (1 kHz sampling rate)
    repeating_timer_t adc_timer;
    add_repeating_timer_ms(ADC_SAMPLE_RATE_MS, sample_adc_callback, NULL, &adc_timer);

    while (1)
    {
        printf("ADC Value: %d, Analog Frequency: %.2f Hz, RMS: %.2f, Peak-to-Peak: %.2f, SNR: %.2f dB\n",
            adc_result, analog_frequency, rms_value, peak_to_peak, snr_value);
        sleep_ms(200);  // Print results every 200 ms
        tight_loop_contents();
    }
}
