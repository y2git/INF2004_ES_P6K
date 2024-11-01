#include <stdio.h>
#include "pico/stdlib.h"
#include "sd_card.h"
#include "ff.h"

// Initialise Variables
FRESULT fr; //Holds the result of FatFS function calls, such as mounting or opening files.
FATFS fs; //filesystem object for the SD card.
FIL fil; //file object used for reading/writing files.
int ret; //Stores return values
char buf[100]; //A buffer used for reading data from the SD card.
char filename[]; //filename to be created

void readfile(char filename[]){
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

void writefile(char filename[]){
    // Open file for writing ()
    fr = f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        while (true);
    }

    // Write something to file
    printf("Writing to file '%s':\r\n", filename);
    ret = f_printf(&fil, "This is another test\r\n");
    if (ret < 0) {
        printf("ERROR: Could not write to file (%d)\r\n", ret);
        f_close(&fil);
        while (true);
    }
    //second print statment to test can delete if not needed
    ret = f_printf(&fil, "of writing to an SD card.\r\n");
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

int asd() {
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