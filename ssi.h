#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"

// SSI tags
const char *ssi_tags[] = {"volt", "temp", "led", "idcode", "data", "timeinfo"};

// Global variable to store the received ID code
char received_idcode[100];
char received_data[100];

u16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen)
{
    size_t printed;
    switch (iIndex)
    {
    case 0: // volt
    {
        const float voltage = adc_read() * 3.3f / (1 << 12);
        printed = snprintf(pcInsert, iInsertLen, "%f", voltage);
    }
    break;

    case 1: // temp
    {
        const float voltage = adc_read() * 3.3f / (1 << 12);
        const float tempC = 27.0f - (voltage - 0.706f) / 0.001721f;
        printed = snprintf(pcInsert, iInsertLen, "%f", tempC);
    }
    break;

    case 2: // led
    {
        bool led_status = cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN);
        printed = snprintf(pcInsert, iInsertLen, led_status ? "ON" : "OFF");
    }
    break;

    case 3: // device-id
    {
        printed = snprintf(pcInsert, iInsertLen, "%s", received_idcode);
    }
    break;

    case 4: // data
    {
        printed = snprintf(pcInsert, iInsertLen, "%s", received_data);
    }
    break;

    case 5: // ntptime
    {
        time_t now = time(NULL);           // Get the current time in UTC
        now += 8 * 3600;                   // Add 8 hours to convert to UTC+8 (Singapore time)
        struct tm *tm_info = gmtime(&now); // Use gmtime to convert to a struct tm

        if (tm_info)
        {
            printf("TIME: %Y-%m-%d %H:%M:%S", tm_info);
            // Format the time as "YYYY-MM-DD HH:MM:SS"
            printed = strftime(pcInsert, iInsertLen, "%Y-%m-%d %H:%M:%S", tm_info);
        }
        else
        {
            printed = snprintf(pcInsert, iInsertLen, "NTP Sync Failed");
        }
    }
    break;

    default:
        printed = 0;
        break;
    }

    return (u16_t)printed;
}

// Initialise the SSI handler
void ssi_init()
{
    // Initialise ADC (internal pin)
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    http_set_ssi_handler(ssi_handler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));
}
