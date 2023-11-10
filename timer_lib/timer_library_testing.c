#include <stdio.h>
#include <string.h>
#include "timer_lib.h"

static void
user_callback_fn(Timer_t *timer, void *user_data)
{
    time_t t;
    time(&t);
    printf("%s ", ctime(&t));
    printf("%s() invoked: name=%s counter=%u threshold=%u exp_time=%lu interval=%lu\n",
            __FUNCTION__, (char *)user_data, timer->invocation_counter, timer->threshold,
            timespec_to_millisec(&timer->ts.it_value), timespec_to_millisec(&timer->ts.it_interval));
}

/* Main API to Test the timerlib functionality */
int main(int argc, char *argv[])
{
    char *name;
    Timer_t *timer;
    int choice;

    name = (char *)calloc(1, 8*sizeof(char));
    strcpy(name, "subbu");

    timer = initialize_timer (user_callback_fn, 5000, 10000, 0, name, false);
    if (!timer)
    {
        printf("Error: Failed to intialize the timer\n");
        return -1;
    }

    /* Invoke the timer */
    start_timer(timer);

    printf("1. Pause timer\n");
    printf("2. Resume timer\n");
    printf("3. Restart timer\n");
    printf("4. Reschedule timer\n");
    printf("5. Cancel timer\n");
    printf("6. Delete timer\n");
    printf("7. Get remaining time\n");
    printf("8. Print timer state\n");

    choice = 0;
    while(1)
    {
        scanf("%d", &choice);
        switch (choice)
        {
            case 1:
                pause_timer(timer);
                break;
            case 2:
                resume_timer(timer);
                break;
            case 3:
                restart_timer(timer);
                break;
            case 4:
                reschedule_timer(timer, timer->exp_timer, timer->sec_exp_timer);
                break;
            case 5:
                cancel_timer(timer);
                break;
            case 6:
            {
                if (timer) 
                {
                    timer_delete_user_data(timer); /* Before delting the timer, free the user data (argument of callback) */
                    delete_timer(timer);
                    printf("Timer deleted\n");
                    return 0;
                }
            }
            case 7:
                printf("Remaining time = %lu\n", timer_get_remaining_time_in_msec(timer));
                break;
            case 8:
                print_timer(timer);
                break;
            default:
                printf("Wrong choice: Pls enter choice between 1-8\n");
                break;
        }
    }

    //pause();
    return 0;
}
