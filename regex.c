#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
regex_t regularEx;
int reg;
int check(const char *str);

int main(int argc, char const *argv[])
{
    reg = regcomp(&regularEx, ".*\\.mp3$", 0);
    if (reg) {
        fprintf(stderr, "Could not compile regex\n");
        return -1;
    }
    
    char *input = malloc(sizeof(input)*100);
    while(1) {
        scanf("%s",input);
        int f = check(input);
        puts(f==1?"YES":"NO");
    }
    return 0;
}

int check(const char *str) {
    reg = regexec(&regularEx, str, 0, NULL, 0);
    if (!reg) return 1;
    else return 0;
}