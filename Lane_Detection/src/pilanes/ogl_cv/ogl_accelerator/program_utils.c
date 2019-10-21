// Henry Bergin 2019
#include <time.h>
#include "program_utils.h"

static long diff_ns(struct timespec start, struct timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
  return temp.tv_nsec;
}

struct timespec start_time, end_time;

void start_profiler_timer()
{
  clock_gettime(CLOCK_REALTIME, &start_time);
}

long stop_profiler_timer()
{
  clock_gettime(CLOCK_REALTIME, &end_time);
  long read_exec_time_ms = diff_ns(start_time, end_time)/1000000;
  return read_exec_time_ms;
}
