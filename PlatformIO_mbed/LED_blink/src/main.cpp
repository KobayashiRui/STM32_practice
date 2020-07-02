#include <mbed.h>

int main() {

  // put your setup code here, to run once:
  DigitalOut myled(PB_3);
  DigitalOut leds[] = {DigitalOut(PA_12), DigitalOut(PB_7), DigitalOut(PB_6), DigitalOut(PF_0), DigitalOut(PF_1)};
  for(int i = 0; i<5; i++){
    leds[i] = 0;
  }

  while(1) {
    leds[1] = 1;
    wait(0.5);
    leds[1] = 0;
    wait(0.5);
    //for(int i = 0; i < 4; i++) {
    //  leds[i] = 1;
    //  wait(0.5);
    //}
    //for(int i = 0; i < 4; i++) {
    //  leds[i] = 0;
    //  wait(0.5);
    //}
  }
}