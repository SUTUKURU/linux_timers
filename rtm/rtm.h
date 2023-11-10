#ifndef _RTM_H_
#define _RTM_H_

#include <stdint.h>
#include <stdbool.h>
#include "../timer_lib/timerlib.h"

#define RT_TABLE_EXP_TIME   30  /* 30 sec */

typedef struct rt_entry_keys_{
    char dest[16];
    char mask;
} rt_entry_keys_t;

typedef struct rt_entry_{
    /* A Structure which represents only the keys of the Routing Table.*/
    rt_entry_keys_t rt_entry_keys;
    char gw_ip[16];
    char oif[32];
    uint32_t time_to_expire; /* time left to delete the entry */
    struct rt_entry_ *prev;
    struct rt_entry_ *next;
    Timer_t *exp_timer; /* Timer structure to expire the route entry */
} rt_entry_t;

/* Routing table DB */
typedef struct rt_table_{
    rt_entry_t *head;
} rt_table_t;

void rt_init_rt_table(rt_table_t *rt_table);
bool rt_add_new_rt_entry(rt_table_t *rt_table,
                        char *dest_ip, char mask, char *gw_ip, char *oif,
                        void (*timer_cb)(Timer_t*, void *));
bool rt_delete_rt_entry(rt_table_t *rt_table,
                        char *dest_ip, char mask);
bool rt_update_rt_entry(rt_table_t *rt_table,
                        char *dest_ip, char mask,
                        char *new_gw_ip, char *new_oif);
void rt_clear_rt_table(rt_table_t *rt_table);
void rt_free_rt_table(rt_table_t *rt_table);
void rt_dump_rt_table(rt_table_t *rt_table);

static inline void 
rt_entry_remove(rt_table_t *rt_table, rt_entry_t *rt_entry)
{
    if(!rt_entry->prev){
        if(rt_entry->next){
            rt_entry->next->prev = 0;
            rt_table->head = rt_entry->next;
            rt_entry->next = 0;
            return;
        }
        rt_table->head = NULL;
        return;
    }
    if(!rt_entry->next){
        rt_entry->prev->next = 0;
        rt_entry->prev = 0;
        return;
    }

    rt_entry->prev->next = rt_entry->next;
    rt_entry->next->prev = rt_entry->prev;
    rt_entry->prev = 0;
    rt_entry->next = 0;
}

#define ITERTAE_RT_TABLE_BEGIN(rt_table_ptr, rt_entry_ptr)                \
{                                                                         \
    rt_entry_t *_next_rt_entry = 0;                                       \
    for((rt_entry_ptr) = (rt_table_ptr)->head;                            \
            (rt_entry_ptr);                                               \
            (rt_entry_ptr) = _next_rt_entry) {                            \
        _next_rt_entry = (rt_entry_ptr)->next;

#define ITERTAE_RT_TABLE_END(rt_table_ptr, rt_entry_ptr)  }}

#endif /* _RTM_H_ */
