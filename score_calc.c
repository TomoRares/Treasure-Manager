#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct GPS
{
    float x, y;
}GPS;

typedef struct treasure
{
    int ID;
    char username[25];
    char clue[25];
    int val;
    GPS gps;
}treasure;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char path[100];
    snprintf(path, sizeof(path), "%s/treasures.dat", argv[1]);

    FILE *f = fopen(path, "rb");
    if (!f)
    {
        perror("Eroare la deschiderea fisierului");
        return 1;
    }

    treasure t;
    int user_count = 0;
    struct
    {
        char username[25];
        int score;
    }users[100];

    while (fread(&t, sizeof(t), 1, f) == 1)
    {
        int found=0;
        for (int i = 0; i < user_count; ++i)
        {
            if (strcmp(users[i].username, t.username) == 0)
            {
                users[i].score+=t.val;
                found=1;
                break;
            }
        }
        if (!found && user_count<100)
        {
            strcpy(users[user_count].username, t.username);
            users[user_count].score=t.val;
            user_count++;
        }
    }

    fclose(f);

    printf("Scoruri pentru hunt %s:\n", argv[1]);
    
    for (int i = 0; i < user_count; ++i)
        printf("%s: %d\n", users[i].username, users[i].score);

    return 0;
}
