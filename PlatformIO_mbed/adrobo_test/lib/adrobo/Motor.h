// Chiba Institute of Technology

#ifndef MBED_MOTOR_H
#define MBED_MOTOR_H

#include "mbed.h"

/** Class to control a L293E motor driver on PWM out pin
 *
 * Example:
 * @code
 * // Motor Control
 * #include "mbed.h"
 * #include "Motor.h"
 * 
 * Motor motor(A1, A2);
 * 
 * int main(){
 *     while(1) {
 *         for (int i = -100; i < 100; i += 1) {
 *             motor = (float)i/100.0f;
 *             wait_ms(20);
 *         }
 *     }
 * }
 * @endcode
 */

class Motor
{
public:
    /** Create a new SoftwarePWM object on pwm pin
      *
      * @param Pin Pin on mbed to connect PWM device to
     */
    Motor(PinName pin0, PinName pin1);
    
    void setMaxRatio(float max_ratio);

    void period(float period);

    void write(float value);

    float read();
    
//#ifdef MBED_OPERATORS
    /** A operator shorthand for write()
     */
    Motor& operator= (float value) {
        write(value);
        return *this;
    }

    Motor& operator= (Motor& rhs) {
        write(rhs.read());
        return *this;
    }

    /** An operator shorthand for read()
     */
    operator float() {
        return read();
    }
//#endif

private:
    PwmOut pwm0;
    PwmOut pwm1;
    float value_;
    float period_;
    float max_ratio_;
};

#endif
