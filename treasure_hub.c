#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdbool.h>

pid_t monitor_pid = -1;
bool monitor_active = false;

void sigchld_handler(int sig) 
{
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid == monitor_pid) 
    {
        printf("[hub] Monitorul s-a oprit.\n");
        monitor_pid = -1;
        monitor_active = false;
    }
}

void start_monitor() 
{
    if (monitor_active) 
    {
        printf("[hub] Monitorul ruleaza deja (PID: %d).\n", monitor_pid);
        return;
    }

    monitor_pid = fork();

    if (monitor_pid < 0) 
    {
        perror("Eroare la fork()");
        exit(1);
    }

    if (monitor_pid == 0) 
    {
        //procesul fiu - devine monitor
        execl("./monitor", "monitor", NULL);
        perror("Eroare la exec monitor");
        exit(1);
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
    kill(monitor_pid, SIGTERM);  // semnal de oprire
}

void send_command_to_monitor(const char *cmd)
{
    if (!monitor_active) 
    {
        printf("[hub] Eroare: Monitorul nu ruleaza.\n");
        return;
    }

    // scrie comanda intr-un fisier
    FILE *f = fopen("hub_cmd.txt", "w");
    if (!f) 
    {
        perror("Eroare deschidere hub_cmd.txt");
        return;
    }

    fprintf(f, "%s\n", cmd);
    fclose(f);

    kill(monitor_pid, SIGUSR1);
}

int main() 
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGCHLD, &sa, NULL);

    char command[100];

    while (1) 
    {
        printf("hub> ");
        fflush(stdout);
        if (fgets(command, sizeof(command), stdin) == NULL) break;

        command[strcspn(command, "\n")] = 0;

        if (strcmp(command, "start_monitor") == 0) 
        {
            start_monitor();
        } 
        else if (strcmp(command, "stop_monitor") == 0) 
        {
            send_command_to_monitor("stop_monitor");
        } 
        else if (strcmp(command, "exit") == 0) 
        {
            if (monitor_active) {
                printf("[hub] Eroare: Monitorul ruleaza inca. Opreste-l cu 'stop_monitor'.\n");
            } else {
                printf("[hub] Inchidere hub.\n");
                break;
            }
        } 
        else if (strncmp(command, "list_hunts", 10) == 0) 
        {
            send_command_to_monitor("list_hunts");
        } 
        else if (strncmp(command, "list_treasures", 14) == 0) 
        {
            send_command_to_monitor(command);
        } 
        else if (strncmp(command, "view_treasure", 13) == 0) 
        {
            send_command_to_monitor(command);
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
