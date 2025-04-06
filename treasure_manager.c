#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct GPS
{
    float x;
    float y;
}GPS;

typedef struct treasure
{
    int ID;
    char username[25];
    char clue[25];
    int val;
}treasure;

void add_treasure(char *hunt_id);
void list_treasures(char *hunt_id);
void view_treasure(char *hunt_id, int treasure_id);

void add_treasure(char *hunt_id)
{
    printf("test\n");
}
void list_treasures(char *hunt_id)
{
    printf("test\n");
}
void view_treasure(char *hunt_id, int treasure_id)
{
    printf("test\n");
}


int main(int argc, char *argv[])
{
    if(argc<3)
    {
        fprintf(stderr, "Folositi formatul:\n"
                        "--add <hunt_id>\n"
                        "--list <hunt_id>\n"
                        "--view <hunt_id><treasure_id>\n");
        exit(-1);
    }

    if(strcmp(argv[1], "--add")==0)
    {
        add_treasure(argv[2]);
    }
    else if(strcmp(argv[1], "--list")==0)
    {
        list_treasures(argv[2]);
    }
    else if(strcmp(argv[1], "--view")==0)
    {
        if(argc<4)
        {
            fprintf(stderr,"ID lipsa\n");
            exit(-1);
        }
        view_treasure(argv[2],atoi(argv[3]));
    }
    else
    {
        fprintf(stderr, "Comanda invalida\n");
        exit(-1);
    }

    return 0;
}