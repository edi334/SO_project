#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h> // Testing & mapping characters : isalnum(int c)
#include <errno.h> // Declaration of global variable errno
#include <fcntl.h> // File descriptor
#include <unistd.h> // Miscellanous functions
#include <sys/stat.h>
#include <dirent.h>
#include <sys/wait.h>

#define PROCESS_NUMBER 3

char name[100], option[10];
DIR *dir;
char path[100], compiled_path[100];
struct stat fis_stat;
struct dirent *in;
int pid, status;

char *get_extension (char *filename) {
    char *dot = strchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot+1;
}

int is_c_file(char *filename) {
    return (strcmp(get_extension(filename), "c") == 0);
}

int has_option(char opt) {
    return (strchr(option, opt) != NULL);
}

char *remove_extension(char *filename) {
    char *ret_str;
    char *last_ext;
    if ((ret_str = malloc (strlen (filename) + 1)) == NULL) return NULL;
    strcpy (ret_str, filename);
    last_ext = strrchr (ret_str, '.');
    if (last_ext != NULL)
        *last_ext = '\0';
    return ret_str;
}

void file_info() {
    if (has_option('n')) {
        printf("Numele fisierului este: %s\n", in->d_name);
    }

    if (has_option('u')) {
        printf("ID-ul utilizatorului este: %d\n", fis_stat.st_uid);
    }

    if (has_option('a')) {
        printf("Utilizator:\n");
        printf("Read - %s\n", fis_stat.st_mode & S_IRUSR ? "DA" : "NU");
        printf("Write - %s\n", fis_stat.st_mode & S_IWUSR ? "DA" : "NU");
        printf("Exec - %s\n", fis_stat.st_mode & S_IXUSR ? "DA" : "NU");
        printf("Grup:\n");
        printf("Read - %s\n", fis_stat.st_mode & S_IRGRP ? "DA" : "NU");
        printf("Write - %s\n", fis_stat.st_mode & S_IWGRP ? "DA": "NU");
        printf("Exec - %s\n", fis_stat.st_mode & S_IXGRP ? "DA": "NU");
        printf("Altii:\n");
        printf("Read - %s\n", fis_stat.st_mode & S_IROTH ? "DA" : "NU");
        printf("Write - %s\n", fis_stat.st_mode & S_IWOTH ? "DA": "NU");
        printf("Exec - %s\n", fis_stat.st_mode & S_IXOTH ? "DA" : "NU");
    }

    if (has_option('d')) {
        printf("Dimensiunea fisierului in octeti: %llx\n", fis_stat.st_size);
    }

    if (has_option('c')) {
        printf("Numarul de legaturi al fisierului: %d\n", fis_stat.st_nlink);
    }

    printf("\n");
}

void make_symlink(char *filepath) {
    char linkpath[100];
    strcpy(linkpath, remove_extension(filepath));

    if (symlink(filepath, linkpath) != 0) {
        printf("EROARE LA CREAREA LEGATURII SIMBOLICE!");
        unlink(filepath);
        exit(1);
    }
}

void citire() {
    while ((in = readdir(dir)) != NULL) {
        sprintf(path, "%s/%s", name, in->d_name);

        if (stat(path, &fis_stat) == -1) {
            perror("EROARE LA STAT!");
            exit(1);
        }

        if (has_option('g')) {
            if (S_ISREG(fis_stat.st_mode) && is_c_file(in->d_name)) {
                strcpy(compiled_path, remove_extension(path));
                strcat(compiled_path, "_compiled");
                char **exec_args = malloc(4 * sizeof(char *));
                exec_args[0] = "gcc";
                exec_args[1] = "-o";
                exec_args[2] = compiled_path;
                exec_args[3] = path;
                exec_args[4] = NULL;
                if ((pid = fork()) < 0) {
                    perror("EROARE LA DESCHIDEREA PROCESULUI!");
                    exit(1);
                }
                if (pid == 0) {
                    if (execvp(exec_args[0], exec_args) == -1) {
                        perror("execvp() FAILED!");
                        exit(1);
                    }
                    exit(2);
                }

                if ((pid = fork()) < 0) {
                    perror("EROARE LA DESCHIDEREA PROCESULUI!");
                    exit(1);
                }
                if (pid == 0) {
                    file_info();
                    exit(2);
                }

                if (fis_stat.st_size < 100000) {
                    if ((pid = fork()) < 0) {
                        perror("EROARE LA DESCHIDEREA PROCESULUI!");
                        exit(1);
                    }
                    if (pid == 0) {
                        make_symlink(path);
                        exit(2);
                    }
                }

                for (int i = 0; i < PROCESS_NUMBER; ++i) {
                    pid = wait(&status);
                    printf("Procesul fiu cu PID %d s-a terminat cu codul %d\n", pid, WEXITSTATUS(status));
                }
                printf("\n");
            }
        } else {
            if (S_ISREG(fis_stat.st_mode) && is_c_file(in->d_name)) {
                file_info();

                if (fis_stat.st_size < 100000) {
                    make_symlink(path);
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        puts("EROARE LA NUMARUL DE ARGUMENTE. NUMARUL NECESAR DE ARGUMENTE ESTE 3!");
        exit(1);
    }

    strcpy(name, argv[1]);
    strcpy(option, argv[2]);

    if ((dir = opendir(name)) < 0) {
        perror("EROARE LA DESCHIDEREA DIRECTORULUI!");
        exit(1);
    }

    citire();
    return 0;
}
