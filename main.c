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

#define NTP_SERVER_NAME "pool.ntp.org" // NTP server domain name

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


// main
void main()
{
    init_stdio();            // Initialize standard I/O
    init_wifi();             // Initialize and connect to Wi-Fi
    init_http_server();      // Initialize the web server
    init_ssi_cgi_handlers(); // Configure SSI and CGI handlers
    print_network_info();    // Print network interface information

    initialize_ntp_client();

    while (1)
    {
        tight_loop_contents();
    }
}
