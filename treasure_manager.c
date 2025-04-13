#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>

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
    GPS gps;

}treasure;

void add_treasure(char *hunt_id);
void list_treasures(char *hunt_id);
void view_treasure(char *hunt_id, int treasure_id);
void remove_treasure(char *hunt_id, int treasure_id);
void remove_hunt(char *hunt_id);

void add_treasure(char *hunt_id)
{
    char dir_path[100], file_path[120], log_path[120], symlink_path[120];

    snprintf(dir_path, sizeof(dir_path), "%s", hunt_id);
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", dir_path);
    snprintf(log_path, sizeof(log_path), "%s/logged_hunt", dir_path);
    snprintf(symlink_path, sizeof(symlink_path), "logged_hunt-%s", hunt_id);

    //creeaza directorul daca nu exista
    mkdir(dir_path, 0777);

    //deschide fisierul pentru adaugare
    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd < 0)
    {
        perror("Eroare la deschiderea fisierului treasure");
        exit(-1);
    }

    treasure t;
    printf("ID: "); scanf("%d", &t.ID);
    printf("Username: "); scanf("%s", t.username);
    printf("Latitude: "); scanf("%f", &t.gps.x);
    printf("Longitude: "); scanf("%f", &t.gps.y);
    printf("Clue: "); scanf(" %[^\n]", t.clue);
    printf("Value: "); scanf("%d", &t.val);

    //scrie structura in fisier
    if (write(fd, &t, sizeof(t)) != sizeof(t))
    {
        perror("Eroare la scriere");
        close(fd);
        exit(-1);
    }
    close(fd);

    //scrie in fisierul de log
    int log_fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (log_fd < 0)
    {
        perror("Eroare log");
        exit(-1);
    }

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0';  //scot newline

    char log_entry[300];
    snprintf(log_entry, sizeof(log_entry), "[%s] Adaugare comoara ID=%d, User=%s\n", 
             timestamp, t.ID, t.username);

    write(log_fd, log_entry, strlen(log_entry));
    close(log_fd);

    //creeaza symlink catre logged_hunt
    if (symlink(log_path, symlink_path) < 0)
    {
        if (errno != EEXIST)
        {
            perror("Eroare creare symlink");
        }
    }

    printf("Comoara adaugata!\n");
}

void list_treasures(char *hunt_id)
{
    char file_path[100];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    //verifica info despre fisier
    struct stat st;
    if (stat(file_path, &st) < 0)
    {
        perror("Eroare stat()");
        exit(-1);
    }

    printf("Hunt: %s\n", hunt_id);
    printf("File size: %ld bytes\n", st.st_size);
    printf("Last modified: %s", ctime(&st.st_mtime)); //deja are newline

    //deschide fisierul pentru citire
    int fd = open(file_path, O_RDONLY);
    if (fd < 0)
    {
        perror("Eroare deschidere fisier");
        exit(-1);
    }

    treasure t;
    printf("\n**** Lista comori ****\n");
    while (read(fd, &t, sizeof(t)) == sizeof(t))
    {
        printf("ID: %d\n", t.ID);
        printf("Username: %s\n", t.username);
        printf("GPS: (%.6f, %.6f)\n", t.gps.x, t.gps.y);
        printf("Clue: %s\n", t.clue);
        printf("Value: %d\n\n", t.val);
    }

    close(fd);
}

void view_treasure(char *hunt_id, int treasure_id)
{
    char file_path[100];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd < 0)
    {
        perror("Eroare deschidere fisier");
        exit(-1);
    }

    treasure t;
    int found = 0;

    while (read(fd, &t, sizeof(t)) == sizeof(t))
    {
        if (t.ID == treasure_id)
        {
            printf("- Detalii treasure:\n\n");
            printf("ID: %d\n", t.ID);
            printf("Username: %s\n", t.username);
            printf("GPS: (%.6f, %.6f)\n", t.gps.x, t.gps.y);
            printf("Clue: %s\n", t.clue);
            printf("Value: %d\n", t.val);
            found = 1;
            break;
        }
    }

    close(fd);

    if (!found)
        printf("Comoara %d nu a fost gasita in hunt-ul '%s'\n", treasure_id, hunt_id);
}

void remove_treasure(char *hunt_id, int treasure_id)
{
    char file_path[100], temp_path[100], log_path[100];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);
    snprintf(temp_path, sizeof(temp_path), "%s/temp.dat", hunt_id);
    snprintf(log_path, sizeof(log_path), "%s/logged_hunt", hunt_id);

    int fd_in = open(file_path, O_RDONLY);
    if (fd_in < 0)
    {
        perror("Eroare deschidere fisier original");
        exit(-1);
    }

    int fd_out = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_out < 0)
    {
        perror("Eroare creare fisier temporar");
        close(fd_in);
        exit(-1);
    }

    treasure t;
    int found = 0;

    while (read(fd_in, &t, sizeof(t)) == sizeof(t))
    {
        if (t.ID == treasure_id)
        {
            found = 1;
            continue; // nu scriem comoara stearsa
        }
        write(fd_out, &t, sizeof(t));
    }

    close(fd_in);
    close(fd_out);

    if (!found)
    {
        printf("Comoara %d nu a fost gasita in hunt-ul '%s'\n", treasure_id, hunt_id);
        unlink(temp_path); // stergem fisierul temporar
        exit(-1);
    }

    // inlocuim fisierul original cu cel temporar
    if (remove(file_path) != 0 || rename(temp_path, file_path) != 0)
    {
        perror("Eroare inlocuire fisier");
        exit(-1);
    }

    // logare
    int log_fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (log_fd >= 0)
    {
        time_t now = time(NULL);
        char *timestamp = ctime(&now);
        timestamp[strlen(timestamp) - 1] = '\0'; // scoate newline

        char log_entry[256];
        snprintf(log_entry, sizeof(log_entry), "[%s] stergere comoara ID=%d\n", timestamp, treasure_id);
        write(log_fd, log_entry, strlen(log_entry));
        close(log_fd);
    }

    printf("Comoara %d a fost stearsa cu succes!\n", treasure_id);
}

void remove_hunt(char *hunt_id) 
{
    char path[100], symlink_path[100];
    snprintf(path, sizeof(path), "%s", hunt_id);
    snprintf(symlink_path, sizeof(symlink_path), "logged_hunt-%s", hunt_id);

    DIR *dir = opendir(path);
    if (!dir)
    {
        perror("Eroare deschidere director");
        exit(-1);
    }

    struct dirent *entry;
    char file_path[500];

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(file_path, sizeof(file_path), "%s/%s", path, entry->d_name);
        if (unlink(file_path) != 0) {
            perror("Eroare la stergerea fisierului");
        }
    }

    closedir(dir);

    if (rmdir(path) != 0)
    {
        perror("Eroare la stergerea directorului");
        exit(-1);
    }

    if (unlink(symlink_path) != 0)
    {
        perror("Eroare la stergerea symlink-ului");
        //dar continuam, poate nu exista
    }

    printf("Hunt-ul '%s' a fost sters complet.\n", hunt_id);
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
    else if(strcmp(argv[1], "--remove_treasure")==0) 
    {
        if(argc<4) 
        {
            fprintf(stderr, "ID lipsa\n");
            exit(-1);
        }
        remove_treasure(argv[2], atoi(argv[3]));
    }
    else if(strcmp(argv[1], "--remove_hunt")==0) 
    {
        remove_hunt(argv[2]);
    }
    else
    {
        fprintf(stderr, "Comanda invalida\n");
        exit(-1);
    }

    return 0;
}