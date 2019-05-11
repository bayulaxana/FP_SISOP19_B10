#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <unistd.h>

regex_t regularEx;
int reg;
int check(const char *str);
static void list_dir(const char *dir_name, int indent);

int main(int argc, char const *argv[])
{
    reg = regcomp(&regularEx, ".*\\.mp3$", 0);
    if (reg) {
        fprintf(stderr, "Could not compile regex\n");
        return -1;
    }
    
    char *path = "/home/bayulaxana/Documents";
    list_dir(path, 0);
    return 0;
}

int check(const char *str) {
    reg = regexec(&regularEx, str, 0, NULL, 0);
    if (!reg) return 1;
    else return 0;
}

void list_dir(const char *dir_name, int indent) {
    struct dirent *de;
    DIR *dir;
    if (!(dir = opendir(dir_name))) return;

    while((de = readdir(dir)) != NULL) {
        if (de->d_type == DT_DIR) {
            char path[1024];
            int a = strcmp(de->d_name,".");
            int b = strcmp(de->d_name,"..");
            if (!a || !b) continue;
            snprintf(path, sizeof(path), "%s/%s", dir_name, de->d_name);
            printf("%s\n",path);
            //printf("%*s[%s]\n",indent,"",de->d_name);
            list_dir(path, indent+2);
        }
        else {
            //printf("%*s- %s\n", indent, "", de->d_name);
            int flag = check(de->d_name);
            if (flag) printf("%s: %s\n",de->d_name, dir_name);
        }
    }
    closedir(dir);
}
