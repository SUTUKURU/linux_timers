/*****************************************************************************
 * provides the declaration for timer_lib.c
 * ***************************************************************************/
#ifndef _TIMER_LIB_H_
#define _TIMER_LIB_H_

#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum TIMER_STATE_ {
    TIMER_INIT = 0,
    TIMER_RUNNING,
    TIMER_CANCELLED,
    TIMER_PAUSED,
    TIMER_RESUMED,
    TIMER_DELETED
} TIMER_STATE_T;

/* User defined Wrapper timer structure */
typedef struct Timer_ {
    timer_t     *posix_timer;        /* posix timer working at core */
    void        *user_arg;          /* Argument to timer_call_back (application memory)  */
    unsigned long exp_timer;        /* in milli-sec, When this time expires timer fires */
    unsigned long sec_exp_timer;    /* in milli-sec, For periodic interval */
    uint32_t    threshold;          /* No.of times to invoke timer callback(optional- implementation specific) */
    void (*timer_cb)(struct Timer_ *, void *); /* Timer callback API */
    bool        exp_backoff;        /* Timer is exponential backoff or not */

    /* dynamic attributes of timer, used for calculation or manipulation */
    unsigned long remaining_time;  /* Time left for paused timer for next expiration */
    uint32_t    invocation_counter; /* number of times the timer expired so far */

    struct itimerspec   ts; /* schedule the timer (specify exp & sec_exp time values) */
    unsigned long exp_backoff_time; /* Exponential backoff time interval */
    TIMER_STATE_T   timer_state; /* Current state of timer (ex: running, pause, cancel...etc */
} Timer_t;

/*------------------------------------Timer Library APIs------------------------------- */
static inline void 
timer_set_state(Timer_t *timer, TIMER_STATE_T timer_state)
{
    timer->timer_state = timer_state;
}

static inline TIMER_STATE_T
timer_get_current_state(Timer_t *timer)
{
    return timer->timer_state;
}

static inline void
timer_delete_user_data(Timer_t *timer)
{
    if(timer->user_arg) {
        free(timer->user_arg);
        timer->user_arg = NULL;
    }
}

Timer_t* initialize_timer(void (*timer_cb)(Timer_t *, void *),
                          unsigned long exp_timer,
                          unsigned long sec_exp_timer,
                          uint32_t threshold,
                          void *user_arg,
                          bool exp_backoff);
void resurrect_timer(Timer_t *timer); /* resurrect means raise from dead */
void start_timer(Timer_t *timer);
void cancel_timer(Timer_t *timer);
void restart_timer(Timer_t *timer);
void pause_timer(Timer_t *timer);
void delete_timer(Timer_t *timer);
void reschedule_timer(Timer_t *timer, 
                      unsigned long exp_time,
                      unsigned long sec_exp_time);
void print_timer(Timer_t *timer);
unsigned long timer_get_remaining_time_in_msec(Timer_t *timer);
unsigned long timespec_to_millisec(struct timespec *ts);
void timer_fill_itimerspec(struct timespec *ts,
                           unsigned long msec);
bool is_timer_running(Timer_t *timer);
char* print_timer_state_str(TIMER_STATE_T state);

#endif /* _TIMER_LIB_H_ */
