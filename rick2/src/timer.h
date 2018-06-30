#ifndef TIMER_H
#define TIMER_H

#include <windows.h>
#include <stdio.h>

typedef class Timer* timer_ptr; 
class Timer {
  private:    
    double PCFreq;
    __int64 CounterStart;

  public:    
	  Timer();    // class constructor	    
	  ~Timer();   // class desructor

    void   StartCounter();
    double GetCounter();
};

#endif // TIMER_H
