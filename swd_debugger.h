
#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"

// Globals
uint xSwdClk = 2;
uint xSwdIO = 3;
bool swdDeviceFound = false;
char cmd;

// Example initialise
void initSwdPins(void)
{
    gpio_init(xSwdClk);
    gpio_init(xSwdIO);
    gpio_set_dir(xSwdClk, GPIO_OUT);
    gpio_set_dir(xSwdIO, GPIO_OUT);
    printf("Pins initialized.\n");
    fflush(stdout);
}

// Example ID code
const char *device_idcode = "12345";

const char *get_idcode()
{
    return device_idcode;
}

#endif