#include "Sound.h"

Sound::Sound(PinName pin)
  : pwm(pin)
{
    this->write(0);
    pwm.Enable(0, 1000000);
}

void Sound::write(float frequency)
{
    frequency_ = frequency;
    if (frequency_ != 0){
        int us = (int)(1000000.0f / frequency_);
        pwm.Disable();
        pwm.Enable(us/2, us);
    } else {
        pwm.Disable();
    }
}

float Sound::read()
{
  return frequency_;
}
