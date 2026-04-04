#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/ecosense.h"
#include "user/user.h"

static void
print_alerts(struct ecosense_data *d)
{
    int none = 1;
    if(d->temp_alert){   printf("HIGH TEMP ");   none = 0; }
    if(d->air_alert){    printf("POOR AIR ");    none = 0; }
    if(d->energy_alert){ printf("HIGH ENERGY "); none = 0; }
    if(none) printf("NONE");
    printf("\n");
}

int
main(void)
{
    setecofriendly();

    struct ecosense_data d;
    int last_eco_mode  = -1;  // -1 = first run
    int eco_base       = 0;
    int norm_base      = 0;
    int skip_next      = 1;   // skip scheduling display after a transition

    while(1){
        if(getsensors(&d) < 0){
            printf("eco_monitor: getsensors failed\n");
            exit(1);
        }

        int truly_active = d.eco_mode &&
                           (d.temp_alert || d.air_alert || d.energy_alert);

        // Detect mode transition
        if(truly_active != last_eco_mode){
            eco_base      = d.eco_ticks;
            norm_base     = d.norm_ticks;
            last_eco_mode = truly_active;
            skip_next     = 1;  // skip this dashboard's scheduling section
        } else {
            skip_next = 0;
        }

        int eco_since  = d.eco_ticks  - eco_base;
        int norm_since = d.norm_ticks - norm_base;

        printf("\n==================================\n");
        printf("=== EcoSense Dashboard          ===\n");
        printf("==================================\n");
        printf("Temperature : %d C\n",  d.temperature);
        printf("Air Quality : %d\n",    d.air_quality);
        printf("Energy Usage: %d\n",    d.energy_usage);
        printf("Alerts      : ");       print_alerts(&d);
        printf("Eco Mode    : %s\n",    truly_active ? "ACTIVE" : "off");
        printf("----------------------------------\n");

        if(!skip_next){
           // printf("Scheduling since last mode change:\n");
            printf("  [ECO ] ticks: %d  (avg/worker: %d)\n",
                   eco_since, eco_since / 3);
            printf("  [NORM] ticks: %d  (avg/worker: %d)\n",
                   norm_since, norm_since / 3);
            if(truly_active && norm_since > 0)
                printf("  ECO running %dx faster than NORM\n",
                       eco_since / norm_since);
            else if(truly_active && norm_since == 0)
                printf("  NORM completely starved\n");
        } else {
            printf("Scheduling  : (mode just changed, gathering data...)\n");
        }

        printf("==================================\n");

        pause(1);
    }

    exit(0);
}