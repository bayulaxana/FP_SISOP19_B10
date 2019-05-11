#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <regex.h>
#include <sys/file.h>
#include <dirent.h>
#include <limits.h>

typedef struct t_node {
    char *value;  
    char *path;
    struct t_node *p_left;
    struct t_node *p_right;
    int cnt;
}node;

typedef int (*Compare)(const char *, const char *);
regex_t regularEx;
int reg;
node *root = NULL;

void insert(char* key, char *path, node** leaf, Compare cmp)
{
    int res;
    if( *leaf == NULL ) {
        *leaf = (node*) malloc(sizeof( node ));
        (*leaf)->value = malloc(strlen(key)+1 );
        (*leaf)->path = malloc(strlen(path)+1);
        strcpy ((*leaf)->value, key);
        strcpy((*leaf)->path, path);
        (*leaf)->cnt = 0;
        (*leaf)->p_left = NULL;
        (*leaf)->p_right = NULL;
    } else {
        res = cmp (key, (*leaf)->value);
        if( res < 0)
            insert( key, path, &(*leaf)->p_left, cmp);
        else if( res > 0)
            insert( key, path, &(*leaf)->p_right, cmp);
        else {
            (*leaf)->cnt += 1;
            char tmp[strlen(key)+555];
            sprintf(tmp, "%s__%d", key, (*leaf)->cnt);
            insert(tmp, path, &(*leaf)->p_right, cmp);
        }
    }
}
// copy the key
int CmpStr(const char *a, const char *b) {
    return (strcmp (a, b));
}

char *search(char* key, node* leaf, Compare cmp)  // no need for **
{
    int res;
    if( leaf != NULL ) {
        res = cmp(key, leaf->value);
        if( res < 0)
            search( key, leaf->p_left, cmp);
        else if( res > 0)
            search( key, leaf->p_right, cmp);
        else
            return (leaf)->path;   // string type
    }
    else return "";
}

void delete_tree(node** leaf)
{
    if( *leaf != NULL ) {
        delete_tree(&(*leaf)->p_left);
        delete_tree(&(*leaf)->p_right);
        free( (*leaf)->value );         // free the key
        free( (*leaf) );
    }
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
            //printf("%s\n",path);
            //printf("%*s[%s]\n",indent,"",de->d_name);
            list_dir(path, indent+2);
        }
        else {
            //printf("%*s- %s\n", indent, "", de->d_name);
            int flag = check(de->d_name);
            if (flag) {
                char npath[1024], fname[1024];
                sprintf(fname, "%s", de->d_name);
                sprintf(npath, "%s/%s", dir_name,de->d_name);
                insert(fname, npath, &root, CmpStr);
            }
        }
    }
    closedir(dir);
}

void in_order(node *root)
{
    if( root != NULL )
    {
        in_order(root->p_left);
        printf("%s\n",root->value);
        in_order(root->p_right);
    }
}

int main()
{
    char str[1024];
    char x[10];
    while(1) {
        scanf("%s",x);
        sprintf(str, "%s", x);
        printf("%s\n",str);
    }
    return 0;
}