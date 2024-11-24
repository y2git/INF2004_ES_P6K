#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"

// CGI handler which is run when a request for /led.cgi is detected
const char *cgi_led_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    // Check if an request for LED has been made (/led.cgi?led=x)
    if (strcmp(pcParam[0], "led") == 0)
    {
        // Look at the argument to check if LED is to be turned on (x=1) or off (x=0)
        if (strcmp(pcValue[0], "0") == 0)
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        else if (strcmp(pcValue[0], "1") == 0)
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    }

    // Send the index page back to the user
    return "/index.shtml";
}

// New CGI handler to process form data sent from the web page (/senddata.cgi?data=x)
const char *cgi_send_data_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    // Loop through the received parameters to find "data"
    for (int i = 0; i < iNumParams; i++)
    {
        if (strcmp(pcParam[i], "data") == 0)
        {
            // Copy the received data to the global variable
            strncpy(received_data, pcValue[i], sizeof(received_data) - 1);
            received_data[sizeof(received_data) - 1] = '\0'; // Null-terminate to avoid overflow

            // Replace '+' with spaces
            for (char *ptr = received_data; *ptr != '\0'; ptr++)
            {
                if (*ptr == '+')
                {
                    *ptr = ' '; // Replace '+' with ' '
                }
            }

            // Print the received data to the serial monitor
            printf("Received data from web: %s\n", received_data);
        }
    }

    // Return the index page after processing the form submission
    return "/index.shtml";
}

// New CGI for getidcode
const char *cgi_idcode_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    static char idcodeResponse[32];

    // Check if an idcode request has been made (/idcode.cgi?idcode=1)
    if (strcmp(pcParam[0], "idcode") == 0)
    {
        if (strcmp(pcValue[0], "1") == 0)
        {
            // Retrieve idcode from DUT
            const char *idcode = "12345";
            snprintf(idcodeResponse, sizeof(idcodeResponse), "/idcode.shtml?idcode=%s", idcode);

            // Print the received data to the serial monitor
            printf("Received idcode: %s\n", received_idcode);

            // Return the response with the idcode embedded
            return idcodeResponse;
        }
    }

    // Send the index page back to the user if no idcode request
    return "/index.shtml";
}

// tCGI Struct
// Add both CGI handlers for LED control and data submission
static const tCGI cgi_handlers[] = {
    {// HTML request for "/led.cgi" triggers cgi_led_handler
     "/led.cgi", cgi_led_handler},
    {// HTML request for "/senddata.cgi" triggers cgi_send_data_handler
     "/senddata.cgi", cgi_send_data_handler},
    {// HTML request for "/getidcode.cgi" triggers cgi_idcode_handler
     "/getidcode.cgi", cgi_idcode_handler}};

// Initialize CGI handlers
void cgi_init(void)
{
    // Register both CGI handlers
    http_set_cgi_handlers(cgi_handlers, sizeof(cgi_handlers) / sizeof(tCGI));
}
