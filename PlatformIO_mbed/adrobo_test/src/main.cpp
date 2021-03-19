
#include "mbed.h"
#include "adrobo.h"

BusOut led(LED1,LED2,LED3,LED4,LED5,LED);

int main() {
    int no = 0;
    while(1) {
        led = 1 << no;
        no = (no >= 5) ? 0 : (no + 1);
        wait(0.4);
    }
}