#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/ecosense.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int is_eco = (argc > 1 && argv[1][0] == 'e');
    int sensor = is_eco ? SENSOR_ECO_TICK : SENSOR_NORM_TICK;

    if (is_eco)
        setecofriendly();

    while (1)
    {
        setsensor(sensor, 0);
         for(volatile int i = 0; i < 50000000; i++);
    }

    exit(0);
}