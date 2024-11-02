#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"

// Globals
uint xSwdClk = 2;
uint xSwdIO = 3;
bool swdDeviceFound = false;
uint32_t device_idcode = 0;  // Stores the actual ID code

// Definitions and Constants
#define LINE_RESET_CLK_CYCLES 52
#define SWD_DELAY 5
#define JTAG_TO_SWD_CMD 0xE79E
#define SWDP_ACTIVATION_CODE 0x1A
#define DHCSR_ADDR 0xE000EDF0
#define DHCSR_DBGKEY 0xA05F0000
#define DHCSR_HALT_CMD (DHCSR_DBGKEY | 0x00000003)    // HALT command: C_DEBUGEN | C_HALT
#define DHCSR_RESUME_CMD (DHCSR_DBGKEY | 0x00000001)  // RESUME command: C_DEBUGEN
#define DAP_TRANSFER_OK 0x01
#define DAP_TRANSFER_WAIT 0x02
#define DHCSR_C_HALT_BIT (1 << 17)
#define onboardLED 25
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

// Function Prototypes
void initSwdPins(void);
void swdInitialOp(void);
void swdClockPulse(void);
void swdSetReadMode(void);
void swdSetWriteMode(void);
void swdTurnAround(void);
bool swdReadAck(long command);
uint32_t swdReadDPIDR(void);
void swdWriteBits(long value, int length);
uint32_t swdDisplayDeviceDetails(uint32_t idcode);
void swdResetLineSWD(void);
void swdArmWakeUp(void);
uint32_t* get_idcode(void);


// Example initialise
void initSwdPins(void) {
    gpio_init(xSwdClk);
    gpio_init(xSwdIO);
    gpio_set_dir(xSwdClk, GPIO_OUT);
    gpio_set_dir(xSwdIO, GPIO_OUT);
    printf("Pins initialized.\n");
    fflush(stdout);
}

// Clock Pulse for SWD
void swdClockPulse(void) {
    gpio_put(xSwdClk, 0);
    sleep_us(SWD_DELAY);  // Use sleep_us for finer timing
    gpio_put(xSwdClk, 1);
    sleep_us(SWD_DELAY);
}

// Set SWDIO as Input
void swdSetReadMode(void) {
    gpio_set_dir(xSwdIO, GPIO_IN);
}

// Set SWDIO as Output
void swdSetWriteMode(void) {
    gpio_set_dir(xSwdIO, GPIO_OUT);
    swdClockPulse();
}

// Switch SWD Direction to Read
void swdTurnAround(void) {
    swdSetReadMode();
    swdClockPulse();
}

// Write a Single Bit to SWD
void swdWriteBit(bool value) {
    gpio_put(xSwdIO, value);
    swdClockPulse();
}

// Write Multiple Bits to SWD
void swdWriteBits(long value, int length) {
    for (int i = 0; i < length; i++) {
        swdWriteBit(bitRead(value, i));
    }
}

// Read a Value from SWD
long swdRead(int bit_count) {
    long buffer = 0;
    for (int x = 0; x < bit_count; x++) {
        bool value = gpio_get(xSwdIO);
        bitWrite(buffer, x, value);
        swdClockPulse();
    }
    return buffer;
}

// Read Acknowledgment from SWD
bool swdReadAck(long command) {
    bool bit1 = gpio_get(xSwdIO);
    swdClockPulse();
    bool bit2 = gpio_get(xSwdIO);
    swdClockPulse();
    bool bit3 = gpio_get(xSwdIO);
    swdClockPulse();
    if (bit1 && !bit2 && !bit3) {
        printf("Command [0x%lx]: ACK OK\n", command);
        fflush(stdout);
        return true;
    }
    printf("Command [0x%lx]: ACK NOT OK\n", command);
    fflush(stdout);
    return false;
}

// Display Device Details for IDCODE
uint32_t swdDisplayDeviceDetails(uint32_t idcode) {
    printf("0x%08X\n", idcode);
    device_idcode = idcode;
    fflush(stdout);
    return device_idcode;
}

// Read the DPIDR Register and return ID code
uint32_t swdReadDPIDR(void) {
    uint32_t idcode = swdRead(32);
    device_idcode = swdDisplayDeviceDetails(idcode);  // Update device_idcode
    return idcode;
}

// Reset the SWD line
void swdResetLineSWD(void) {
    gpio_put(xSwdIO, 1);  // Set SWDIO high
    for (int x = 0; x < LINE_RESET_CLK_CYCLES + 10; x++) {
        swdClockPulse();
    }
    gpio_put(xSwdIO, 0);  // Send reset pulses
    for (int i = 0; i < 4; i++) {
        swdClockPulse();
    }
    gpio_put(xSwdIO, 1);
}

// Wake up the SWD interface
void swdArmWakeUp(void) {
    swdSetWriteMode();
    for (int x = 0; x < 8; x++) {
        swdClockPulse();  // Reset to selection alert sequence
    }

    // Send selection alert sequence
    swdWriteBits(0x92, 8);
    swdWriteBits(0xF3, 8);
    swdWriteBits(0x09, 8);
    swdWriteBits(0x62, 8);
    swdWriteBits(0x95, 8);
    swdWriteBits(0x2D, 8);
    swdWriteBits(0x85, 8);
    swdWriteBits(0x86, 8);
    swdWriteBits(0xE9, 8);
    swdWriteBits(0xAF, 8);
    swdWriteBits(0xDD, 8);
    swdWriteBits(0xE3, 8);
    swdWriteBits(0xA2, 8);
    swdWriteBits(0x0E, 8);
    swdWriteBits(0xBC, 8);
    swdWriteBits(0x19, 8);

    // Idle bits
    swdWriteBits(0x00, 4);
    swdWriteBits(SWDP_ACTIVATION_CODE, 8);
}

// Initialize SWD and read IDCODE
void swdInitialOp(void) {
    printf("Initializing SWD and reading IDCODE...\n");
    fflush(stdout);

    swdSetWriteMode();
    swdArmWakeUp();
    swdResetLineSWD();
    swdWriteBits(JTAG_TO_SWD_CMD, 16);
    swdResetLineSWD();
    swdWriteBits(0x00, 4);
    swdWriteBits(0xA5, 8);  // readIdCode command (0xA5)
    swdTurnAround();
    
    if (swdReadAck(0xA5)) {
        swdDeviceFound = true;
        printf("Device found!\n");
        fflush(stdout);
        swdReadDPIDR();  // Read and set the Device ID
    } else {
        printf("No device found.\n");
        fflush(stdout);
    }
}

// Get the device ID code
uint32_t* get_idcode() {
    return &device_idcode;
    
}

#endif
