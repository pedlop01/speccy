#include "timer.h" // class's header file

// class constructor
Timer::Timer() {
  PCFreq = 0.0;
  CounterStart = 0;
}

// class destructor
Timer::~Timer() {
}

void Timer::StartCounter()
{
  LARGE_INTEGER li;
  if(!QueryPerformanceFrequency(&li))
    printf("QueryPerformanceFrequency failed!\n");

  PCFreq = double(li.QuadPart)/1000.0;

  QueryPerformanceCounter(&li);
  CounterStart = li.QuadPart;
}

double Timer::GetCounter()
{
  LARGE_INTEGER li;
  QueryPerformanceCounter(&li);

  return double(li.QuadPart-CounterStart)/PCFreq;
}
