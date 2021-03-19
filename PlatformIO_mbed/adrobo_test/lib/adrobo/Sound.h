// Chiba Institute of Technology

#ifndef MBED_SOUND_H
#define MBED_SOUND_H

#include "mbed.h"
#include "SoftwarePWM.h"

/** Class to control a motor on any pin, without using pwm pin
 *
 * Example:
 * @code
 * // Motor Control
 * #include "mbed.h"
 * #include "Sound.h"
 *
 * Motor motor(xp31, xp32, xp33);
 * Motor motor(xp34, xp35, xp36);
 *
 * int main(){
 * while(1) {
 *     for (int pos = 1000; pos < 2000; pos += 25) {
 *         Servo1.SetPosition(pos);
 *         wait_ms(20);
 *     }
 *     for (int pos = 2000; pos > 1000; pos -= 25) {
 *         Servo1.SetPosition(pos);
 *         wait_ms(20);
 *     }
 * }
 * @endcode
 */

typedef enum {
    M_C2 = 130,
    M_D2 = 146,
    M_E2 = 164,
    M_F2 = 174,
    M_G2 = 195,
    M_A3 = 220,
    M_B3 = 246,
    M_C4 = 261,
    M_D4 = 293,
    M_E4 = 329,
    M_F4 = 349,
    M_G4 = 391,
    M_A4 = 440,
    M_B4 = 493,
    M_C5 = 523,
    M_D5 = 587,
    M_E5 = 659,
    M_F5 = 698,
    M_G5 = 783,
    M_A5 = 880,
    M_B5 = 987,
    M_C6 = 1046,
    M_D6 = 1174,
    M_E6 = 1328,
    M_F6 = 1396,
    M_G6 = 1567,
    M_A6 = 1760,
    M_B6 = 1975,
} Freq;

class Sound
{
public:
    /** Create a new SoftwarePWM object on any mbed pin
      *
      * @param Pin Pin on mbed to connect PWM device to
     */
    Sound(PinName Ppwm);

    void write(float value);

    float read();
    
//#ifdef MBED_OPERATORS
    /** A operator shorthand for write()
     */
    Sound& operator= (float value) {
        write(value);
        return *this;
    }

    Sound& operator= (Sound& rhs) {
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
    SoftwarePWM pwm;
    float frequency_;
    float period_;
};

#endif
