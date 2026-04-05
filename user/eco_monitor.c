#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/ecosense.h"
#include "user/user.h"

static void
print_alerts(struct ecosense_data *d)
{
    int none = 1;

    if(d->temp_alert){
        printf("HIGH TEMP ");
        none = 0;
    }
    if(d->air_alert){
        printf("POOR AIR ");
        none = 0;
    }
    if(d->energy_alert){
        printf("HIGH ENERGY ");
        none = 0;
    }
    if(none)
        printf("NONE");

    printf("\n");
}

int
main(void)
{
    struct ecosense_data d;

    while(1){
        if(getsensors(&d) < 0){
            printf("ecosense_monitor: getsensors failed\n");
            exit(1);
        }

        printf("\n=== EcoSense Dashboard ===\n");
        printf("Temperature : %d C\n", d.temperature);
        printf("Air Quality : %d\n", d.air_quality);
        printf("Energy Usage: %d\n", d.energy_usage);
        printf("Alerts      : ");
        print_alerts(&d);

        for(volatile int i = 0; i < 200000000; i++);
    }

    exit(0);
}


