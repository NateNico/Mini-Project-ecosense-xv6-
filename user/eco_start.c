#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static int
spawn(char *prog, char *argv[])
{
    int pid = fork();
    if(pid == 0){
        exec(prog, argv);
        printf("eco_start: failed to exec %s\n", prog);
        exit(1);
    }
    return pid;
}

int
main(void)
{
    int pids[10];
    int n = 0;

    pids[n++] = spawn("sensor_temp",   (char*[]){"sensor_temp",   0});
    pids[n++] = spawn("sensor_air",    (char*[]){"sensor_air",    0});
    pids[n++] = spawn("sensor_energy", (char*[]){"sensor_energy", 0});
    pids[n++] = spawn("eco_monitor",   (char*[]){"eco_monitor",   0});
    pids[n++] = spawn("sim_worker",    (char*[]){"sim_worker", "eco",  0});
    pids[n++] = spawn("sim_worker",    (char*[]){"sim_worker", "eco",  0});
    pids[n++] = spawn("sim_worker",    (char*[]){"sim_worker", "eco",  0});
    pids[n++] = spawn("sim_worker",    (char*[]){"sim_worker", "norm", 0});
    pids[n++] = spawn("sim_worker",    (char*[]){"sim_worker", "norm", 0});
    pids[n++] = spawn("sim_worker",    (char*[]){"sim_worker", "norm", 0});

    printf("--- EcoSense demo running. Press Enter to stop. ---\n");

    char buf[1];
    read(0, buf, 1);

    for(int i = 0; i < n; i++)
        kill(pids[i]);
    for(int i = 0; i < n; i++)
        wait(0);

    printf("eco_start: all processes stopped.\n");
    exit(0);
}