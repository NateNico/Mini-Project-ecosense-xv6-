#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/ecosense.h"
#include "user/user.h"

int
main(void)
{
    int air = 50;
    unsigned int seed = 2;

    while(1){
        seed = seed * 1103515245 + 12345;
        int change = (seed % 5) - 2;   // -2 to +2

        air += change;

        if(air < 0)
            air = 0;
        if(air > 100)
            air = 100;

        setsensor(SENSOR_AIR, air);

        int delay = 100000000;
        if(change == 0)
            delay = 180000000;
        else
            delay = 80000000;

        for(volatile int i = 0; i < delay; i++);
    }

    exit(0);
}







