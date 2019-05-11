#include <ao/ao.h>
#include <mpg123.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/file.h>
#include <unistd.h>
#include <sys/types.h>
#define BITS 8
#define TRUE 1
#define FALSE 0

typedef struct t_node {
    struct t_node *next;
    char *value;
} Node;

Node *head;
pthread_t tid[2];
char *source_path = "/home/bayulaxana/FP_SISOP19_B10/mounted";
mpg123_handle *mh, *mh_bak;
unsigned char *buffer;
size_t buffer_size;
size_t done;
int err;
int driver;
ao_device *dev;
ao_sample_format format;
int channels, encoding;
int play_status;
long rate;
char *now_playing;


int compare_node(Node *n1, Node *n2);
Node *add_node(Node *list, Node *node, char *str);
void display_list(Node *head);
int search(Node *head, char *str);
void free_list(Node *head);
char *find_next(Node *head, char *str);
char *find_prev(Node *head, char *str);
void *take_input(void *arg);
void list_dir(const char *path);
void *play_song(void *arg);

int main(int argc, char const *argv[])
{
    /* Initialization of Linked List and Listing Directory */
    head = NULL;
    list_dir(source_path);
    now_playing = malloc(sizeof(now_playing)*1024);
    
    ao_initialize();
    driver = ao_default_driver_id();
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    mh_bak = mpg123_new(NULL, &err);
    buffer_size = mpg123_outblock(mh);
    buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));

    play_status = FALSE;
    pthread_create(&(tid[0]), NULL, &take_input, NULL);
    pthread_join(tid[0], NULL);

    return 0;
}

int compare_node(Node *n1, Node *n2) {
    return strcmp(n1->value, n2->value);
}

Node *add_node(Node *list, Node *node, char *str) {
    node->value = malloc(strlen(str) + 1);
    strcpy(node->value, str);
    Node *prev, *next;
    if (!list) list = node;
    else {
        prev = NULL;
        next = list;
        while (next && compare_node(node,next)>0) {
            prev = next;
            next = next->next;
        }
        if (!next) prev->next = node;
        else {
            if (prev) {
                node->next = prev->next;
                prev->next = node;
            } else {
                node->next = list;
                list = node;
            }
        }
    }
    return list;
}

void display_list(Node *head) {
    int cnt = 1;
    while(head) {
        printf("  |- %d %s\n",cnt++, head->value);
        head = head->next;
    }
}

char *find_next(Node *head, char *str) {
    Node *tmp = head;
    while(tmp) {
        int a = strcmp(tmp->value, str);
        if (a == 0) {
            if (tmp->next == NULL) return head->value;
            return tmp->next->value;
        }
        tmp = tmp->next;
    }
}

char *find_prev(Node *head, char *str) {
    Node *tmp = head;
    Node *prev = NULL;
    while(tmp) {
        int a = strcmp(tmp->value, str);
        if (a == 0) {
            if (prev == NULL) {
                while(tmp) {
                    if (tmp->next == NULL) return tmp->value;
                    tmp = tmp->next;
                }
            }
            return prev->value;
        }
        prev = tmp;
        tmp = tmp->next;
    }
}

int search(Node *head, char *str) {
    int a;
    while (head) {
        a = strcmp(head->value, str);
        if (a == 0) return 1;
        head = head->next;
    }
    return 0;
}

void free_list(Node *head) {
	Node *prev = head;
	Node *cur = head;
	while (cur) {
		prev = cur;
		cur = prev->next;
		free(prev);
	}
}

void list_dir(const char *path) {
    Node *newNode;
    struct dirent *de;
    DIR *dir;
    if (!(dir = opendir(path))) return;

    while((de = readdir(dir)) != NULL) {
        if (!strcmp(de->d_name,".") || !strcmp(de->d_name,"..")) continue;
        newNode = (Node*)malloc(sizeof(Node));
        head = add_node(head, newNode, de->d_name);
    }
    closedir(dir);
}

void *take_input(void *arg) 
{
    char input[100], name[1000];
    while(1) {
        scanf("%s",input);
        if (strcmp("play", input) == 0) {
            scanf("%s",name);
            if (search(head, name)) {
                pthread_cancel(tid[1]);
                play_status = TRUE;
                sprintf(now_playing, "%s", name);
                pthread_create(&(tid[1]), NULL, &play_song, name);
            } 
            else {
                puts("  |- File not found");
            }
        }
        else if (strcmp("pause", input) == 0) {
            play_status = (play_status==TRUE? FALSE:TRUE);
        }
        else if (strcmp("stop", input) == 0) {
            pthread_cancel(tid[1]);
            play_status = FALSE;
        }
        else if (strcmp("list",input) == 0) {
            display_list(head);
        }
        else if (strcmp("next",input) == 0) {
            pthread_cancel(tid[1]);
            sprintf(now_playing, "%s", find_next(head, now_playing));
            pthread_create(&(tid[1]), NULL, &play_song, now_playing);
        }
        else if (strcmp("prev",input) == 0) {
            pthread_cancel(tid[1]);
            sprintf(now_playing, "%s", find_prev(head, now_playing));
            pthread_create(&(tid[1]), NULL, &play_song, now_playing);
        } 
        else if (strcmp("exit", input) == 0) {
            
            if (play_status == FALSE) break;
            
            /* clean up */
            free(buffer);
            ao_close(dev);
            mpg123_close(mh);
            mpg123_delete(mh);
            mpg123_exit();
            ao_shutdown();
            break;
        }
    }
}

void *play_song(void *arg) {
    int flag = 0;
    char *str = ((char*)arg);
    char fpath[1024];
    sprintf(fpath, "%s/%s", source_path,str);

    while(1) {
        if (flag) {
            str = find_next(head, str);
            sprintf(fpath, "%s/%s", source_path,str);
        }  
        
        /* open the file and get the decoding format */
        mpg123_open(mh, fpath);
        mpg123_getformat(mh, &rate, &channels, &encoding);

        /* set the output format and open the output device */
        format.bits = mpg123_encsize(encoding) * BITS;
        format.rate = rate;
        format.channels = channels;
        format.byte_format = AO_FMT_NATIVE;
        format.matrix = 0;
        dev = ao_open_live(driver, &format, NULL);

        /* decode and play */
        int errm = mpg123_read(mh, buffer, buffer_size, &done);
        while (1){
            if(play_status){ // paused
                ao_play(dev, buffer, done);
                errm = mpg123_read(mh, buffer, buffer_size, &done);

                if(errm != MPG123_OK) break;
            }
            sleep(0.01);
        }
        flag = 1;
    }
}