/**
 * @file clocks.cpp
 * @author Simone Comari
 * @date 14 Sep 2018
 * @brief File containing definitions of functions and class declared in clocks.h.
 */

#include "clocks.h"

namespace grabrt
{

uint64_t Sec2NanoSec(const double seconds)
{
  return static_cast<uint64_t>(seconds * 1000000000);
}

double NanoSec2Sec(const long nanoseconds)
{
  return static_cast<double>(nanoseconds) * 0.000000001;
}

/////////////////////////////////////////////////
/// ThreadClock Class Methods
/////////////////////////////////////////////////

void ThreadClock::Reset()
{
  //  printf("[%s] RESET\n", name_.c_str());
  clock_gettime(CLOCK_MONOTONIC, &time_);
}

double ThreadClock::Elapsed() const
{
  static struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &end);
  return end.tv_sec - time_.tv_sec +
         (end.tv_nsec - time_.tv_nsec) / static_cast<double>(kNanoSec2Sec);
}

void ThreadClock::Next()
{
  time_.tv_sec += (static_cast<uint64_t>(time_.tv_nsec) + period_nsec_) / kNanoSec2Sec;
  time_.tv_nsec = (static_cast<uint64_t>(time_.tv_nsec) + period_nsec_) % kNanoSec2Sec;
}

bool ThreadClock::WaitUntilNext()
{
  Next();
  if (Elapsed() > 0)
    return false;
  int ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time_, NULL);
  if (ret != 0)
    HandleErrorEnWrapper(ret, "clock_nanosleep ");
  return true;
}

struct timespec ThreadClock::GetNextTime()
{
  Next();
  return GetCurrentTime();
}

void ThreadClock::DispCurrentTime() const
{
  printf("%s status:\n\ttime =\t%lu.%ld sec\n\tperiod =\t%lu\n", name_.c_str(),
         time_.tv_sec, time_.tv_nsec, period_nsec_);
}

void ThreadClock::DispNextTime()
{
  Next();
  DispCurrentTime();
}

[[noreturn]] void ThreadClock::HandleErrorEnWrapper(const int en, const char* msg) const
{
  std::string full_msg = "[";
  full_msg.append(name_);
  full_msg.append("] ");
  full_msg.append(msg);
  HandleErrorEn(en, full_msg.c_str());
}

} // end namespace grabrt
