/***************************************************************************************************
 * This expalains about how to ageout the routing table entry.
 ***************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "rtm.h"

static rt_table_t rt; /* Glabal variable of routing table */

int main(int argc, char **argv)
{
    rt_init_rt_table(&rt); /* Initialize the routing table */

    /* Adding entries to routing table */
    rt_add_new_rt_entry(&rt, "100.1.1.1", 32, "10.1.1.1", "eth0", rt_entry_delete_on_timer_expiry);
    rt_add_new_rt_entry(&rt, "100.1.1.2", 32, "10.1.1.2", "eth1", rt_entry_delete_on_timer_expiry);
    rt_add_new_rt_entry(&rt, "100.1.1.3", 32, "10.1.1.3", "eth2", rt_entry_delete_on_timer_expiry);
    rt_add_new_rt_entry(&rt, "100.1.1.4", 32, "10.1.1.4", "eth3", rt_entry_delete_on_timer_expiry);
    rt_add_new_rt_entry(&rt, "100.1.1.5", 32, "10.1.1.5", "eth4", rt_entry_delete_on_timer_expiry);

    while(1)
    {
        int choice;
        printf("Enter Choice 1.Add rt entry 2.Update rt entry 3.Delete rt entry 4 Dump rt table: \n");
        scanf("%d", &choice);
        fflush(stdin);
        switch(choice)
        {
           case 1:
                {
                    char dest[16];
                    uint8_t mask;
                    char oif[32];
                    char gw[16];
                    printf("Enter Destination :");
                    scanf("%s", dest);
                    printf("Mask : ");
                    scanf("%hhd", &mask);
                    printf("Enter oif name :");
                    scanf("%s", oif);
                    printf("Enter Gateway IP :");
                    scanf("%s", gw);
                    if(!rt_add_new_rt_entry(&rt, dest, mask, gw, oif, rt_entry_delete_on_timer_expiry))
                    {
                        printf("Error : Could not add an entry\n");
                    }
                }
                break;
            case 2:
                break;
            case 3:
                break;
            case 4:
                rt_dump_rt_table(&rt);
            default:
                break;
        }
    }
    return 0;
}

