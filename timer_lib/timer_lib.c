/******************************************************************************
 * Author: Subramanyam
 * This file contains functionality of the timer library implemetation.
 * -> Which provides more control over timers.
 *      - start, stop, pause, resume, delete, reschedule, back-off timer
 *      - provides the simplified interface
 *      - Provide the scopre for more complex timer development.
 * -> We can create our own timers, which are actually wrapper on posix timers.
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <memory.h>
#include "timer_lib.h"

char* print_timer_state_str(TIMER_STATE_T state)
{
    switch (state)
    {
        case TIMER_INIT:
            return "TIMER_INIT";
        case TIMER_RUNNING:
            return "TIMER_RUNNING";
        case TIMER_PAUSED:
            return "TIMER_PAUSED";
        case TIMER_RESUMED:
            return "TIMER_RESUMED";
        case TIMER_CANCELLED:
            return "TIMER_CANCELLED";
        case TIMER_DELETED:
            return "TIMER_DELETED";
        default:
            return "Unknown state";
    }
}

void timer_fill_itimerspec (struct timespec *ts, unsigned long milli_sec)
{
    memset(ts, 0, sizeof(struct timespec));
    if (!milli_sec)
        return;

    unsigned long sec = milli_sec/1000;
    ts->tv_sec = sec;

    unsigned long remaining_msec = milli_sec % 1000;
    ts->tv_nsec = remaining_msec * (1000000);
}

unsigned long timespec_to_millisec (struct timespec *ts)
{
    unsigned long milli_sec = 0;
    milli_sec = ts->tv_sec *1000;
    milli_sec += ts->tv_nsec/1000000;
    return milli_sec;
}

static void timer_callback_wrapper(union sigval arg)
{
    Timer_t *timer = (Timer_t *)(arg.sival_ptr);
    timer->invocation_counter++;

    if (timer->threshold &&
        (timer->invocation_counter > timer->threshold))
    {
        cancel_timer(timer);
        return;
    }

    /* Invoking thfunctional API to do functionality */
    (timer->timer_cb)(timer, timer->user_arg);

    if (timer->exp_backoff) {
        if (timer->exp_backoff_time <= 0)
          return;
        reschedule_timer(timer, timer->exp_backoff_time *= 2, 0);
    } else {
        reschedule_timer(timer, timer->exp_timer, timer->sec_exp_timer);
    }
}

/* Function:Initialize (construct) the timer data structure.
 *
 * Input:   timer_cb : Timer callback with user data and user size
 *          exp_timer: First expiration time interval in msec
 *          sec_exp_timer: Subsequent expiration time interval in msec
 *          threshold: Max no.of expirations, 0-for inifinite
 *          user_arg: Argument to timer callback
 *          exp_backoff: Is Timer Exp backoff
 * Output:  Returns Timer_t pointer.
 */
Timer_t* initialize_timer (void (*timer_cb)(Timer_t *, void *),
                            unsigned long exp_timer,
                            unsigned long sec_exp_timer,
                            uint32_t threshold,
                            void *user_arg,
                            bool exp_backoff)
{
    struct sigevent evp;
    Timer_t *timer = NULL;

    timer = calloc(1, sizeof(Timer_t));
    if (!timer) {
        printf("Error: calloc failed to allocate memory for timer\n");
        return NULL;
    }

    timer->posix_timer = calloc(1, sizeof(timer_t));
    if (!timer->posix_timer) {
        printf("Error: calloc failed to allocate memory for posix_timer\n");
        return NULL;
    }

    timer->user_arg = user_arg;
    timer->exp_timer = exp_timer;
    timer->sec_exp_timer = sec_exp_timer;
    timer->threshold = threshold;
    timer->exp_backoff = exp_backoff;
    timer->timer_cb = timer_cb;
    timer_set_state(timer, TIMER_INIT);

    assert(timer->timer_cb); /*Sanity check */

    /*  */
    memset(&evp, 0, sizeof(struct sigevent));
    evp.sigev_value.sival_ptr = (void *)(timer);
    evp.sigev_notify = SIGEV_THREAD;
    /* Wrapper API internally invokes the user specified API when timer expires */
    evp.sigev_notify_function = timer_callback_wrapper;

    int rc = timer_create (CLOCK_REALTIME, &evp, timer->posix_timer);
    assert(rc >= 0);
  /* Initialize the first expiration timer */
    timer_fill_itimerspec(&timer->ts.it_value, timer->exp_timer);

    if (!timer->exp_backoff) {
        /* sec_exp_timer = 0 if oneshot timer else periodic timer with sec_exp_timer as interval */
        timer_fill_itimerspec(&timer->ts.it_interval, timer->sec_exp_timer);
        timer->exp_backoff_time = 0;
    } else {
        timer->exp_backoff_time = timespec_to_millisec(&timer->ts.it_value);
        timer_fill_itimerspec(&timer->ts.it_interval, 0);
    }

    return timer;
}

/*
 * This API as wrapper for timer_settime().
 * If the timer_spec values are zero then this timer stops the running timer.
 * If timer_spec values are proper then timer starts.
 */
void resurrect_timer (Timer_t *timer)
{
    int rc;
    rc = timer_settime(*(timer->posix_timer), 0, &timer->ts, NULL);
    assert(rc>=0);
}

void start_timer (Timer_t *timer)
{
    resurrect_timer(timer);
    timer_set_state(timer, TIMER_RUNNING);
}

unsigned long timer_get_remaining_time_in_msec(Timer_t *timer)
{
    struct itimerspec remaining_time;
    if (timer->timer_state == TIMER_DELETED ||
        timer->timer_state == TIMER_CANCELLED) {
        return ~0;
    }

    memset(&remaining_time, 0 , sizeof(struct itimerspec));
    /* OS provides this api to get the timers remaining time */
    timer_gettime(*(timer->posix_timer), &remaining_time);

           /* conevert that to milli sec */
    return timespec_to_millisec(&remaining_time.it_value);
}

void pause_timer(Timer_t *timer)
{
    if (timer->timer_state == TIMER_PAUSED)
        return;

    /* get the reamining time of the timer */
    timer->remaining_time = timer_get_remaining_time_in_msec(timer);

    /* set the ts.it_value, ts.it_value to '0' to pause the timer */
    timer_fill_itimerspec(&timer->ts.it_value, 0);
    timer_fill_itimerspec(&timer->ts.it_interval, 0);

    resurrect_timer(timer);
    timer_set_state (timer, TIMER_PAUSED);
}

void resume_timer(Timer_t *timer)
{
    if (timer->timer_state != TIMER_PAUSED)
    {
        printf("Timer is not in TIMER_PAUSED state\n");
        return;
    }

    /* Fill the remaining time to resume and time interval values */
    timer_fill_itimerspec(&timer->ts.it_value, timer->remaining_time);
    timer_fill_itimerspec(&timer->ts.it_interval, timer->sec_exp_timer);
    timer->remaining_time = 0; /* reset time_remaining */

    resurrect_timer(timer);
    /* set to TIMER_RESUMED, once expires the remainig_time then state needs to change to RUNNING */
    timer_set_state (timer, TIMER_RESUMED);
}

void delete_timer(Timer_t *timer)
{
    int rc;
    rc = timer_delete(*(timer->posix_timer));
    assert(rc >= 0);

    /* User arg need to be freed by Application */
    timer->user_arg = NULL; 
    timer_set_state(timer, TIMER_DELETED);

    free(timer->posix_timer);
    timer->posix_timer = NULL;

    free(timer);
    timer = NULL;
}

void cancel_timer(Timer_t *timer)
{
    TIMER_STATE_T timer_curr_state;
    timer_curr_state = timer_get_current_state(timer);

    if(timer_curr_state == TIMER_INIT || timer_curr_state == TIMER_DELETED)
    {
        return; /* No operation */
    }

    /* Only Paused or running timer can be cancelled */
    timer_fill_itimerspec(&timer->ts.it_value, 0);
    timer_fill_itimerspec(&timer->ts.it_interval, 0);
    timer->remaining_time = 0;
    timer->invocation_counter = 0;

    resurrect_timer(timer);
    timer_set_state(timer, TIMER_CANCELLED);
}

void restart_timer(Timer_t *timer)
{
    assert(timer->timer_state != TIMER_DELETED);
    cancel_timer(timer);

    timer_fill_itimerspec(&timer->ts.it_value, timer->exp_timer);
    if(!timer->exp_backoff)
        timer_fill_itimerspec(&timer->ts.it_interval, timer->sec_exp_timer);
    else
        timer_fill_itimerspec(&timer->ts.it_interval, 0);

    timer->invocation_counter = 0;
    timer->remaining_time = 0;
    timer->exp_backoff_time = timer->exp_timer;

    resurrect_timer(timer);
    timer_set_state(timer, TIMER_RUNNING);
}

void reschedule_timer(Timer_t *timer,
                        unsigned long exp_time,
                        unsigned long sec_exp_time)
{
    uint32_t invocation_counter;
    TIMER_STATE_T timer_state;

    timer_state = timer_get_current_state(timer);

    /* deleted timer can't be reschduled */
    if(timer_state == TIMER_DELETED)
        assert(0);

    invocation_counter = timer->invocation_counter;
    if(timer_state != TIMER_CANCELLED)
        cancel_timer(timer);
    timer->invocation_counter = invocation_counter;

    timer_fill_itimerspec(&timer->ts.it_value, exp_time);
    if(!timer->exp_backoff)
    {
        timer_fill_itimerspec(&timer->ts.it_interval, sec_exp_time);
        timer->exp_backoff_time = 0;
    }
    else
    {
        timer_fill_itimerspec(&timer->ts.it_interval, 0);
        timer->exp_backoff_time = exp_time;
    }

    timer->remaining_time = 0;
    resurrect_timer(timer);
    timer_set_state(timer, TIMER_RUNNING);
}

bool is_timer_running(Timer_t *timer)
{
    TIMER_STATE_T timer_state;
    timer_state = timer_get_current_state(timer);
    if(timer_state == TIMER_RUNNING || timer_state == TIMER_RESUMED)
    {
        return true;
    }
    return false;
}

void print_timer(Timer_t *timer)
{
    if (timer) {
        printf("Counter = %u, time remaining = %lu, state = %s\n",
                timer->invocation_counter,
                timer_get_remaining_time_in_msec(timer),
                print_timer_state_str(timer->timer_state));
    }
}

