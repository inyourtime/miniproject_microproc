#include "Timer3CTC.h"
#include "Timer4CTC.h"

Timer3CTC Timer3;
Timer4CTC Timer4;
 
void (*Timer3CTC::isrCallback)() = Timer3CTC::isrDefaultUnused;
void (*Timer4CTC::isrCallback)() = Timer4CTC::isrDefaultUnused;


ISR (TIMER3_COMPA_vect)
{
  Timer3.isrCallback();
}

void Timer3CTC::isrDefaultUnused()
{
}

ISR (TIMER4_COMPA_vect)
{
  Timer4.isrCallback();
}

void Timer4CTC::isrDefaultUnused()
{
}
