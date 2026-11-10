#ifndef BANANA_DOSBOX_TIMEB_H
#define BANANA_DOSBOX_TIMEB_H
#include <sys/time.h>
struct timeb { time_t time; unsigned short millitm; short timezone, dstflag; };
static inline int ftime(struct timeb *out) {
    struct timeval t;
    if (gettimeofday(&t, 0)) return -1;
    out->time=t.tv_sec; out->millitm=t.tv_usec/1000;
    out->timezone=0; out->dstflag=0; return 0;
}
#endif
