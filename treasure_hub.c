#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdbool.h>

pid_t monitor_pid = -1;
bool monitor_active = false;

// handler cand monitorul se inchide
void sigchld_handler(int sig)
{
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid == monitor_pid)
    {
        printf("[hub] Monitorul s-a oprit\n");
        monitor_pid = -1;
        monitor_active = false;
    }
}

void start_monitor()
{
    if (monitor_active)
    {
        printf("[hub] Monitorul ruleaza deja (PID: %d)\n", monitor_pid);
        return;
    }

    monitor_pid = fork();

    if (monitor_pid < 0)
    {
        perror("Eroare la fork()");
        exit(-1);
    }

    if (monitor_pid == 0)
    {
        // Procesul fiu -> devine monitor
        execl("./monitor", "monitor", NULL);
        perror("Eroare la exec monitor");
        exit(-1);
    }

    printf("[hub] Monitor pornit cu PID %d\n", monitor_pid);
    monitor_active = true;
}

void stop_monitor() 
{
    if (!monitor_active) 
    {
        printf("[hub] Monitorul nu este pornit\n");
        return;
    }

    printf("[hub] Trimit semnal de oprire monitorului...\n");
    kill(monitor_pid, SIGTERM);  //semnal generic de oprire
}

int main() 
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGCHLD, &sa, NULL);

    char command[100];

    while (1) {
        printf("hub> ");
        fflush(stdout);
        if (fgets(command, sizeof(command), stdin) == NULL) break;

        //scot newline
        command[strcspn(command, "\n")] = 0;

        if (strcmp(command, "start_monitor") == 0) 
        {
            start_monitor();
        } 
        else if (strcmp(command, "stop_monitor") == 0)
        {
            stop_monitor();
        }
        else if (strcmp(command, "exit") == 0) 
        {
            if (monitor_active) 
            {
                printf("[hub] Eroare: Monitorul inca ruleaza. Opreste-l cu 'stop_monitor'\n");
            } 
            else 
            {
                printf("[hub] Inchidere hub.\n");
                break;
            }
        } 
        else 
        {
            if (!monitor_active) 
            {
                printf("[hub] Eroare: Monitorul nu este activ. Foloseste 'start_monitor'.\n");
            }
            else 
            {
                printf("[hub] Comanda recunoscuta, dar neimplementata inca.\n");
            }
        }
    }

    return 0;
}
