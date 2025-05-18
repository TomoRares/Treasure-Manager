#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>

pid_t monitor_pid = -1;
bool monitor_active = false;

int pipefd[2];  //pipe intre hub si monitor
int monitor_read_fd = -1;  //unde vom citi outputul monitorului

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

    if (pipe(pipefd) < 0)
    {
        perror("Eroare la creare pipe");
        exit(-1);
    }

    monitor_pid = fork();

    if (monitor_pid < 0) 
    {
        perror("Eroare la fork()");
        exit(-1);
    }

    if (monitor_pid == 0) 
    {
        //procesul fiu - devine monitor
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execl("./monitor", "monitor", NULL);
        perror("Eroare la exec monitor");
        exit(-1);
    }

    close(pipefd[1]);
    monitor_read_fd = pipefd[0];

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

void read_monitor_output()
{
    char buf[400];
    ssize_t n;

    while ((n = read(monitor_read_fd, buf, sizeof(buf)-1)) > 0)
    {
        buf[n] = '\0';
        printf("%s", buf);
        if (n < sizeof(buf)-1) break;
    }
}

void calculate_score()
{
    DIR *d = opendir(".");
    if(!d)
    {
        perror("Nu pot deschide directorul curent");
        return;
    }

    struct dirent *entry;
    while((entry = readdir(d)) != NULL)
    {
        if(entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {

            char test_path[400];
            snprintf(test_path, sizeof(test_path), "%s/treasures.dat", entry->d_name);

            if (access(test_path, F_OK) == 0)
            {
                int pipefd[2];
                if (pipe(pipefd) < 0)
                {
                    perror("Pipe error");
                    continue;
                }

                pid_t pid = fork();
                if (pid == 0)
                {
                    //copilul
                    close(pipefd[0]);
                    dup2(pipefd[1], STDOUT_FILENO);
                    close(pipefd[1]);
                    execl("./score_calc", "score_calc", entry->d_name, NULL);
                    perror("execl score_calc");
                    exit(1);
                }
                else if(pid > 0)
                {
                    //parintele
                    close(pipefd[1]);
                    char buf[200];
                    ssize_t n;
                    printf("\n[Scoruri pentru %s]\n", entry->d_name);
                    while ((n = read(pipefd[0], buf, sizeof(buf)-1)) > 0) {
                        buf[n] ='\0';
                        printf("%s", buf);
                    }
                    close(pipefd[0]);
                    waitpid(pid, NULL, 0);
                }
            }
        }
    }

    closedir(d);
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
            usleep(400000); // pauza pana scrie monitorul
            read_monitor_output();
        } 
        else if (strncmp(command, "list_treasures", 14) == 0) 
        {
            send_command_to_monitor(command);
            usleep(400000);
            read_monitor_output();
        } 
        else if (strncmp(command, "view_treasure", 13) == 0) 
        {
            send_command_to_monitor(command);
            usleep(400000);
            read_monitor_output();
        }
        else if (strcmp(command, "calculate_score") == 0) 
        {
            calculate_score();
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
