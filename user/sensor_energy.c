#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/ecosense.h"
#include "user/user.h"

int
main(void)
{
    int energy = 70;
    unsigned int seed = 3;

    while(1){
        seed = seed * 1103515245 + 12345;
        int change = (seed % 7) - 3;   // -3 to +3

        energy += change;

        if(energy < 0)
            energy = 0;
        if(energy > 100)
            energy = 100;

        setsensor(SENSOR_ENERGY, energy);

        int delay = 100000000;
        if(change == 0)
            delay = 180000000;
        else
            delay = 80000000;

        for(volatile int i = 0; i < delay; i++);
    }

    exit(0);
}






