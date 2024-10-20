#include <stdio.h>
#include "pico/stdlib.h"

int main() {

    char buf[100];

    stdio_init_all();
    sleep_ms(5000);
    printf("alive\n");

    
    while (true) {
        buf[0] = getchar();
        printf("Received: %c (%d)\r\n", buf[0], buf[0]);
        //printf("z\n");
        sleep_ms(1000);
    }

}