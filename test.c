#include <stdio.h>
#include "pico/stdlib.h"
#include "buddy1.h"
int main() {
    char buf[100]; //A buffer used for reading data from the SD card.
    char filename[] = "test03.txt"; //filename to be created
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
    writefile(filename);
    readfile(filename);

    // Unmount drive
    f_unmount("0:");

    // Loop forever doing nothing
    while (true) {
        sleep_ms(1000);
    }

}