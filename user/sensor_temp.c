#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
    int temp = 20;
    int seed = 1;

    while(1){
        seed = seed * 1103515245 + 12345;
        int change = (seed % 3) - 1;

        temp = temp + change;

        printf("Temperature Sensor: %d C\n", temp);

        for(volatile int i = 0; i < 100000000; i++);
    }

    exit(0);
}

