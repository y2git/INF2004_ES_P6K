#include "lwip/apps/httpd.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwipopts.h"
#include "ssi.h"
#include "cgi.h"
#include "swd_debugger.h"

#include "lwip/netif.h" // Access network interface information


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


// main
void main()
{
    init_stdio();            // Initialize standard I/O
    init_wifi();             // Initialize and connect to Wi-Fi
    init_http_server();      // Initialize the web server
    init_ssi_cgi_handlers(); // Configure SSI and CGI handlers
    print_network_info();    // Print network interface information
    stdio_init_all();
    sleep_ms(1000); // Allow time for serial connection to initialize
    initSwdPins();
    swdInitialOp();
    swdInitialOp();
    printf("0x%08X\n",device_idcode);

    while (1)
    {
        tight_loop_contents();
    }
}
