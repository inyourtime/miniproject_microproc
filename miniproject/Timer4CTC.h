/*
 * Timer4 with CTC mode 4 (TOP is OCR4A)
 */

#ifndef Timer4CTC_H
#define Timer4CTC_H

#include "Arduino.h"
#include <avr/io.h>

#define TIMER4_RESOLUTION 65535UL

class Timer4CTC {
  public:
    /******************************
     * Configuration *
    ******************************/
    Timer4CTC() {}
    void initialize(unsigned long milliseconds = 1000) { // Setup timer mode and period
      TCCR4A = 0;
      TCCR4B = 0;
      TCCR4B |= 0x08; // CTC mode 4
      setPeriod(milliseconds);
    }
    void setPeriod(unsigned long milliseconds = 1000) { // Setup period
      const unsigned long cycles = F_CPU/1000 * milliseconds;
      if (cycles < TIMER4_RESOLUTION) {
        clockSelectBits = 0x01;
        period = cycles - 1;
      } else if (cycles < TIMER4_RESOLUTION * 8) {
        clockSelectBits = 0x02;
        period = (cycles / 8) - 1;
      } else if (cycles < TIMER4_RESOLUTION * 64) {
        clockSelectBits = 0x03;
        period = (cycles / 64) - 1;
      } else if (cycles < TIMER4_RESOLUTION * 256) {
        clockSelectBits = 0x04;
        period = (cycles / 256) - 1;
      } else if (cycles < TIMER4_RESOLUTION * 1024) {
        clockSelectBits = 0x05;
        period = (cycles / 1024) - 1;
      } else {
        clockSelectBits = 0x05;
        period = TIMER4_RESOLUTION - 1;
      }
      OCR4A = period;
    }
    
    /******************************
     * Run Control *
    ******************************/
    void start() {
      TCNT4 = 0;
      resume();
    }
    void stop() {
      TCCR4B &= ~(0x07);
    }
    void restart() {
      start();
    }
    void resume() {
      TCCR4B |= clockSelectBits;
    }

    /******************************
     * Interrupt Function *
    ******************************/
    void attachInterrupt(void (*isr)()) {
      isrCallback = isr;
      TIMSK4 |= 1<<OCIE4A;
    }
    void detachInterrupt() {
      TIMSK4 = 0;
    }
    static void (*isrCallback)();
    static void isrDefaultUnused();
  
  private:
    uint16_t period;
    uint8_t clockSelectBits;
};

//extern Timer4 timer4;

#endif
