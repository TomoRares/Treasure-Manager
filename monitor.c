#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

typedef struct GPS 
{
    float x;
    float y;
} GPS;

typedef struct treasure 
{
    int ID;
    char username[25];
    char clue[25];
    GPS gps;
    int val;
} treasure;

volatile sig_atomic_t got_command = 0;

volatile sig_atomic_t stop_requested = 0;

void handle_sigusr1(int sig) 
{
    got_command = 1;
}

void list_hunts()
{
    printf("[monitor] Comanda: list_hunts\n");

    system("ls -d */ 2>/dev/null | while read dir; do "
           "if [ -f \"$dir/treasures.dat\" ]; then "
           "count=$(stat -c%s \"$dir/treasures.dat\" | awk '{print int($1/sizeof_struct)}'); "
           "echo \"$dir - comori: $count\"; fi; done");
}

void list_treasures_cmd(const char *hunt_id)
{
    char path[100];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);

    FILE *f = fopen(path, "rb");
    if (!f)
    {
        printf("[monitor] Nu pot deschide: %s\n", path);
        return;
    }

    printf("[monitor] Comorile din %s:\n", hunt_id);

    treasure t;
    while (fread(&t, sizeof(treasure), 1, f) == 1)
    {
        printf("ID: %d | User: %s | GPS: %.2f, %.2f | Valoare: %d\n",
               t.ID, t.username, t.gps.x, t.gps.y, t.val);
        printf("Clue: %s\n", t.clue);
        printf("-----------------------------\n");
    }

    fclose(f);
}

void view_treasure_cmd(const char *hunt_id, int target_id) 
{
    char path[100];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);

    FILE *f = fopen(path, "rb");
    if (!f) 
    {
        printf("[monitor] Nu pot deschide: %s\n", path);
        return;
    }

    treasure t;
    int found = 0;
    while (fread(&t, sizeof(treasure), 1, f) == 1) 
    {
        if (t.ID == target_id) 
        {
            printf("[monitor] Comoara gasita in %s:\n", hunt_id);
            printf("ID: %d | User: %s | GPS: %.2f, %.2f | Valoare: %d\n",
                   t.ID, t.username, t.gps.x, t.gps.y, t.val);
            printf("Clue: %s\n", t.clue);
            found = 1;
            break;
        }
    }

    if (!found) 
        printf("[monitor] Comoara %d nu a fost gasita in %s.\n", target_id, hunt_id);

    fclose(f);
}

void run_monitor()
{
    struct sigaction sa;
    sa.sa_handler = handle_sigusr1;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) < 0)
    {
        perror("sigaction");
        exit(1);
    }

    printf("[monitor] Pornit. Astept comenzi...\n");

    while (1) 
    {
        if (got_command) 
        {
            got_command = 0;

            FILE *f = fopen("hub_cmd.txt", "r");
            if (!f)
            {
                perror("Eroare deschidere hub_cmd.txt");
                continue;
            }

            char cmd[250];
            if (fgets(cmd, sizeof(cmd), f))
            {
                cmd[strcspn(cmd, "\n")] = 0;

                if (strcmp(cmd, "list_hunts") == 0) {
                    list_hunts();
                } 
                else if (strncmp(cmd, "list_treasures", 14) == 0) 
                {
                    char hunt_id[100];
                    if (sscanf(cmd, "list_treasures %s", hunt_id) == 1)
                    {
                        list_treasures_cmd(hunt_id);
                    } 
                    else 
                    {
                        printf("[monitor] Sintaxa gresita pentru list_treasures\n");
                    }
                } 
                else if (strncmp(cmd, "view_treasure", 13) == 0) 
                {
                    char hunt_id[100];
                    int id;
                    if (sscanf(cmd, "view_treasure %s %d", hunt_id, &id) == 2)
                    {
                        view_treasure_cmd(hunt_id, id);
                    }
                    else 
                    {
                        printf("[monitor] Sintaxa gresita pentru view_treasure\n");
                    }
                }
                else if (strcmp(cmd, "stop_monitor") == 0) 
                {
                    printf("[monitor] Stop primit. Oprire in curs...\n");
                    stop_requested = 1;
                }
                else 
                {
                    printf("[monitor] Comanda necunoscuta: %s\n", cmd);
                }
            }
            fclose(f);
        }

        if (stop_requested) 
        {
            usleep(5000000); //5 secunde intarziere
            printf("[monitor] Monitor inchis.\n");
            exit(0);
        }

        usleep(100000); //0.1 sec pentru a nu face busy wait

    }
}


int main()
{
    run_monitor();
    return 0;
}
