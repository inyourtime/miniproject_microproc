/*
 * Timer3 with CTC mode 4 (TOP is OCR3A)
 */

#ifndef Timer3CTC_H
#define Timer3CTC_H

#include "Arduino.h"
#include <avr/io.h>

#define TIMER3_RESOLUTION 65535UL

class Timer3CTC {
  public:
    /******************************
     * Configuration *
    ******************************/
    Timer3CTC() {}
    void initialize(unsigned long milliseconds = 1000) { // Setup timer mode and period
      TCCR3A = 0;
      TCCR3B = 0;
      TCCR3B |= 0x08; // CTC mode 4
      setPeriod(milliseconds);
    }
    void setPeriod(unsigned long milliseconds = 1000) { // Setup period
      const unsigned long cycles = F_CPU/1000 * milliseconds;
      if (cycles < TIMER3_RESOLUTION) {
        clockSelectBits = 0x01;
        period = cycles - 1;
      } else if (cycles < TIMER3_RESOLUTION * 8) {
        clockSelectBits = 0x02;
        period = (cycles / 8) - 1;
      } else if (cycles < TIMER3_RESOLUTION * 64) {
        clockSelectBits = 0x03;
        period = (cycles / 64) - 1;
      } else if (cycles < TIMER3_RESOLUTION * 256) {
        clockSelectBits = 0x04;
        period = (cycles / 256) - 1;
      } else if (cycles < TIMER3_RESOLUTION * 1024) {
        clockSelectBits = 0x05;
        period = (cycles / 1024) - 1;
      } else {
        clockSelectBits = 0x05;
        period = TIMER3_RESOLUTION - 1;
      }
      OCR3A = period;
    }
    
    /******************************
     * Run Control *
    ******************************/
    void start() {
      TCNT3 = 0;
      resume();
    }
    void stop() {
      TCCR3B &= ~(0x07);
    }
    void restart() {
      start();
    }
    void resume() {
      TCCR3B |= clockSelectBits;
    }

    /******************************
     * Interrupt Function *
    ******************************/
    void attachInterrupt(void (*isr)()) {
      isrCallback = isr;
      TIMSK3 |= 1<<OCIE3A;
    }
    void detachInterrupt() {
      TIMSK3 = 0;
    }
    static void (*isrCallback)();
    static void isrDefaultUnused();
  
  private:
    uint16_t period;
    uint8_t clockSelectBits;
};

//extern Timer3 timer3;

#endif
