#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
    char *temp_argv[] = {"sensor_temp", 0};
    char *air_argv[] = {"sensor_air", 0};
    char *energy_argv[] = {"sensor_energy", 0};
    char *monitor_argv[] = {"eco_monitor", 0};

    if(fork() == 0){
        exec("sensor_temp", temp_argv);
        printf("failed to start sensor_temp\n");
        exit(1);
    }

    if(fork() == 0){
        exec("sensor_air", air_argv);
        printf("failed to start sensor_air\n");
        exit(1);
    }

    if(fork() == 0){
        exec("sensor_energy", energy_argv);
        printf("failed to start sensor_energy\n");
        exit(1);
    }

    if(fork() == 0){
        exec("eco_monitor", monitor_argv);
        printf("failed to start eco_monitor\n");
        exit(1);
    }

    wait(0);
    wait(0);
    wait(0);
    wait(0);

    exit(0);
}


