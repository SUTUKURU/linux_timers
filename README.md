# linux_timers
Implementing the linux timer library.

-> Here timer_lib is a linux timers implemeation and used as library. 
   So that it can be used to implement user specific timers funtionality.
-> timer_lib.c file contains the wrappers for actual OS timers and generic to use by application.
-> route_mgr is an application, which will add the routing entries into routing table.
   And uses the timer_lib functionality to expire the route entries from the DB once specified time expires.
   
   Note: Only implemented route entry add and delete after expiry. 
         Other functionality to be implemented.
