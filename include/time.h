/* time.h
   Adapted from Linux kernel include/uapi/linux/time.h
   Adapted from Linux kernel include/uapi/linux/timex.h
   Adapted from Linux kernel include/uapi/linux/timerfd.h
 */

#ifndef TIME_H
#define TIME_H

#include <io.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLOCK_REALTIME			0
#define CLOCK_MONOTONIC			1
#define CLOCK_PROCESS_CPUTIME_ID	2
#define CLOCK_THREAD_CPUTIME_ID		3
#define CLOCK_MONOTONIC_RAW		4
#define CLOCK_REALTIME_COARSE		5
#define CLOCK_MONOTONIC_COARSE		6
#define CLOCK_BOOTTIME			7
#define CLOCK_REALTIME_ALARM		8
#define CLOCK_BOOTTIME_ALARM		9
#define CLOCK_TAI			11

#define TFD_CLOEXEC O_CLOEXEC
#define TFD_NONBLOCK O_NONBLOCK

typedef int clockid_t;

struct timespec {
  long tv_sec; /* seconds */
  long tv_nsec; /* nanoseconds */
};

struct timeval {
  long tv_sec; /* seconds */
  long tv_usec; /* microseconds */
};

struct itimerspec {
  struct timespec it_interval; /* Interval for periodic timer */
  struct timespec it_value; /* Initial expiration */
};

struct timex {
  unsigned int modes; /* mode selector */
  long offset; /* time offset (usec) */
  long freq; /* frequency offset (scaled ppm) */
  long maxerror; /* maximum error (usec) */
  long esterror; /* estimated error (usec) */
  int status; /* clock command/status */
  long constant; /* pll time constant */
  long precision; /* clock precision (usec) (read only) */
  long tolerance; /* clock frequency tolerance (ppm)
		   * (read only)
		   */
  struct timeval time; /* (read only, except for ADJ_SETOFFSET) */
  long tick; /* (modified) usecs between clock ticks */
  
  long ppsfreq; /* pps frequency (scaled ppm) (ro) */
  long jitter; /* pps jitter (us) (ro) */
  int shift; /* interval duration (s) (shift) (ro) */
  long stabil; /* pps stability (scaled ppm) (ro) */
  long jitcnt; /* jitter limit exceeded (ro) */
  long calcnt; /* calibration intervals (ro) */
  long errcnt; /* calibration errors (ro) */
  long stbcnt; /* stability limit exceeded (ro) */

  int tai; /* TAI offset (ro) */

  int  :32; int  :32; int  :32; int  :32;
  int  :32; int  :32; int  :32; int  :32;
  int  :32; int  :32; int  :32;
};

int nanosleep (const struct timespec * req, struct timespec * rem);
int clock_nanosleep (clockid_t clockid, int flags, const struct timespec * request, struct timespec * remain);

int clock_gettime (clockid_t clockid, struct timespec * tp);
int clock_settime (clockid_t clockid, const struct timespec * tp);
int clock_getres (clockid_t clockid, struct timespec * res);

int adjtimex (struct timex * buf);
int clock_adjtime (clockid_t clockid, struct timex * buf);

fd_t timerfd_create (clockid_t clockid, int flags);
int timerfd_settime (fd_t fd, int flags, const struct itimerspec * new_value, struct itimerspec * old_value);
int timerfd_gettime (fd_t fd, struct itimerspec * curr_value);

#ifdef __cplusplus
}
#endif

#endif
