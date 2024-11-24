#include "lwip/apps/httpd.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwipopts.h"
#include "ssi.h"
#include "cgi.h"

#include "lwip/netif.h"  // Access network interface information
#include "lwip/udp.h"    // UDP for communication
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"

// Define the UDP port to use
#define UDP_PORT 1234
#define RETRY_LIMIT 5
#define TIMEOUT_MS 5000  // Timeout for ACK to 5 seconds

// Static IP configuration for home and PROLINK networks
#define HOME_WIFI_SSID "ROG_2.4GHZ"
#define PROLINK_WIFI_SSID "Prolink_2FED"

// Home network configuration
#define HOME_IP "192.168.50.51"
#define HOME_NETMASK "255.255.255.0"
#define HOME_GATEWAY "192.168.50.1"

// PROLINK network configuration
#define PROLINK_IP "192.168.1.101"
#define PROLINK_NETMASK "255.255.255.0"
#define PROLINK_GATEWAY "192.168.1.1"

static volatile bool ack_received = false;

// Function to set up static IP configuration based on SSID
void setup_static_ip(struct netif *netif, const char *ssid) {
    ip_addr_t ipaddr, netmask, gw;

    if (strcmp(ssid, HOME_WIFI_SSID) == 0) {
        printf("Connected to home Wi-Fi\n");
        IP4_ADDR(&ipaddr, 192, 168, 50, 51);  // Home IP
        IP4_ADDR(&netmask, 255, 255, 255, 0);  // Home subnet mask
        IP4_ADDR(&gw, 192, 168, 50, 1);        // Home gateway
    } else if (strcmp(ssid, PROLINK_WIFI_SSID) == 0) {
        printf("Connected to PROLINK Wi-Fi\n");
        IP4_ADDR(&ipaddr, 192, 168, 1, 101);   // PROLINK IP
        IP4_ADDR(&netmask, 255, 255, 255, 0);  // PROLINK subnet mask
        IP4_ADDR(&gw, 192, 168, 1, 1);         // PROLINK gateway
    } else {
        printf("Unknown Wi-Fi SSID\n");
        return;
    }

    netif_set_addr(netif, &ipaddr, &netmask, &gw);
    printf("Static IP Address set to: %s\n", ipaddr_ntoa(&netif->ip_addr));
}

// Function to connect to WiFi with automatic reconnection
bool connect_to_wifi(const char *ssid, const char *password) {
    int retries = 0;
    while (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 10000) != 0) {
        printf("Attempting to reconnect to Wi-Fi: %s (Attempt %d)\n", ssid, retries + 1);
        retries++;
        sleep_ms(5000);  // Wait 5 seconds before retrying
        if (retries >= 5) {
            printf("Wi-Fi connection failed after 5 attempts.\n");
            return false;
        }
    }
    printf("Connected to Wi-Fi: %s\n", ssid);
    return true;
}

// UDP receive callback for handling incoming commands and ACK
void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    if (p != NULL) {
        if (strcmp((char *)p->payload, "ACK") == 0) {
            printf("Received ACK from %s:%d\n", ipaddr_ntoa(addr), port);
            ack_received = true;
        } else {
            printf("Received command from %s:%d -> %s\n", ipaddr_ntoa(addr), port, (char *)p->payload);
        }
        pbuf_free(p);
    }
}

// Function to send data with ACK
void udp_send_data_with_ack(struct udp_pcb *pcb, const char *data, const char *ssid) {
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, strlen(data) + 1, PBUF_RAM);
    if (p != NULL) {
        memcpy(p->payload, data, strlen(data) + 1);

        ip_addr_t addr;

        // Set server IP based on the connected SSID
        if (strcmp(ssid, HOME_WIFI_SSID) == 0) {
            // Home Wi-Fi server IP
            IP4_ADDR(&addr, 192, 168, 50, 50);
            printf("Sending data to Home server at 192.168.50.50\n");
        } else if (strcmp(ssid, PROLINK_WIFI_SSID) == 0) {
            // PROLINK Wi-Fi server IP
            IP4_ADDR(&addr, 192, 168, 1, 100);
            printf("Sending data to PROLINK server at 192.168.1.100\n");
        } else {
            printf("Unknown SSID: %s, cannot set server IP.\n", ssid);
            pbuf_free(p);
            return;
        }

        int retries = 0;
        while (retries < RETRY_LIMIT) {
            ack_received = false;
