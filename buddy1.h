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

void writefile(char filename[], char text[]){
    // Open file for writing ()
    fr = f_open(&fil, filename, FA_WRITE | FA_OPEN_APPEND);
    if (fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        while (true);
    }

    // Write something to file
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