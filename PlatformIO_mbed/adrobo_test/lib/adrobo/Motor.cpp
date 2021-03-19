#include "mbed.h"
#include "Motor.h"

#define max(a,b) (a > b ? a : b)
#define min(a,b) (a > b ? b : a)

Motor::Motor(PinName pin0, PinName pin1)
  : pwm0(pin0), pwm1(pin1), max_ratio_(0.5f)
{
    period(0.01f);
    write(0.0f);
}

void Motor::setMaxRatio(float max_ratio)
{
    max_ratio_ = max_ratio;
}

void Motor::period(float period)
{
    period_ = period;
    pwm0.period(period_);
    pwm1.period(period_);
}

void Motor::write(float value)
{
    value = min(max(value, -1.0f), 1.0f);
    value_ = value;
    pwm0 = value_ > 0.0f ?  value_ : 0.0f;
    pwm1 = value_ < 0.0f ? -value_ : 0.0f;
}

float Motor::read()
{
  return value_;
}
