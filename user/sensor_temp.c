#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/ecosense.h"
#include "user/user.h"

int
main(void)
{
    // Sensor processes are the eco-critical workload — mark them eco-friendly
    // so they get scheduling priority when eco mode fires.
    setecofriendly();

    int temp = 20;
    unsigned int seed = 1;

    while(1){
        seed = seed * 1103515245 + 12345;
        int change = (seed % 3) - 1;   // -1, 0, 1

        temp += change;

        if(temp < 0)
            temp = 0;
        if(temp > 40)
            temp = 40;

        setsensor(SENSOR_TEMP, temp);

        int delay;
        if(change == 0)
            delay = 180000000;
        else
            delay = 80000000;

        for(volatile int i = 0; i < delay; i++);
    }

    exit(0);
}