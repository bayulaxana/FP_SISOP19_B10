#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <regex.h>
#include <sys/file.h>
#define TRUE 1
#define FALSE 0

static const char *dirpath = "/home/bayulaxana";

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
int listed_dir;
node *root = NULL;

char *handle_duplicate(char *str, int n) 
{
    int l_occur;
    char *pch = strrchr(str, '.');
    if (pch == NULL) return "";
    l_occur = pch-str+1;
    char *new = malloc(sizeof(char)*1024);
    snprintf(new, l_occur, "%s", str);
    sprintf(new, "%s_(%d).mp3", new,n);
    return new;
}

void insert(char* key, char *path, node** leaf, Compare cmp)
{
    int res;
    if (*leaf == NULL) {
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
            char *tmp;
            tmp = handle_duplicate(key, (*leaf)->cnt);
            insert(tmp, path, &(*leaf)->p_right, cmp);
        }
    }
}

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

void search_path(char* path_to, char *buff, node* leaf, Compare cmp, int flag)  // no need for **
{
    int res;
	if (flag) return;
	if (leaf == NULL) return;
    if( leaf != NULL ) {
		res = strcmp(path_to, leaf->path);
		if (res==0) {
			flag = 1;
			sprintf(buff, "%s", leaf->value);
		}
		search_path(path_to, buff, leaf->p_left, cmp, flag);
		search_path(path_to, buff, leaf->p_right, cmp, flag);
    }
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

void in_order(node *root)
{
    if( root != NULL )
    {
        in_order(root->p_left);
        //printf("%s: %s\n",root->value,root->path);
        in_order(root->p_right);
    }
}

int check(const char *str) 
{
    reg = regexec(&regularEx, str, 0, NULL, 0);
    if (!reg) return 1;
    else return 0;
}

static void *pre_init(struct fuse_conn_info *conn)
{
	listed_dir = FALSE;
	reg = regcomp(&regularEx, ".*\\.mp3$", 0);
    if (reg) {
        fprintf(stderr, "Could not compile regex\n");
        return NULL;
    }
	insert("/","/home/bayulaxana", &root, CmpStr);
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
    int res;
	char tmp[1024];
	sprintf(tmp, "%s", path);

	char *fpath = search(tmp, root, CmpStr);
	res = lstat(fpath, stbuf);

	if (res == -1)
		return -errno;

	return 0;
}

static int list_dir(const char *path, void *buf, fuse_fill_dir_t filler, 
				off_t offset, struct fuse_file_info *fi)
{
	if (strcmp("/home/bayulaxana/FP_SISOP19_B10/mounted", path) == 0) return 0;
	if (strcmp("/home/bayulaxana/Downloads/firefox", path) == 0) return 0;
	
	int res = 0;
	DIR *dir;
	struct dirent *de;

	(void)offset;
	(void)fi;

	dir = opendir(path);
	if (dir == NULL)
		return -errno;

	while ((de = readdir(dir)) != NULL) {
		if (de->d_type == DT_DIR) {
			char new_f[1024];
			int a = strcmp(de->d_name, ".");
			int b = strcmp(de->d_name, "..");
			if (!a || !b) continue;
			if (de->d_name[0] == '.') continue;
			snprintf(new_f, sizeof(new_f), "%s/%s", path, de->d_name);
			res = list_dir(new_f, buf, filler, offset, fi);
		}
		else {
			char fname[1024], npath[1024], d_name[1024];
			sprintf(fname, "/%s", de->d_name);
			sprintf(npath, "%s/%s", path,de->d_name);
			struct stat st;
			memset(&st, 0, sizeof(st));
			st.st_ino = de->d_ino;
			st.st_mode = de->d_type << 12;

			if (check(fname)) {
				insert(fname, npath, &root, CmpStr);
				char temp[1024];
				search_path(npath, temp, root, CmpStr, 0);
				strncpy(d_name, temp+1, strlen(temp));
				res = (filler(buf, d_name, &st, 0));
				if(res!=0) break;
			}
		}
	}
	closedir(dir);
	return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
    char fpath[1000];
	if(strcmp(path,"/") == 0)
	{
		path=dirpath;
		sprintf(fpath,"%s",path);
	}
	else sprintf(fpath, "%s%s",dirpath,path);
	int res;
	
	res = list_dir(fpath, buf, filler, offset, fi);
	listed_dir = TRUE;
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
    
{
	int res = 0;
    int fd = 0 ;
	
	char tmp[1024];
	sprintf(tmp, "%s", path);

	char *fpath = search(tmp, root, CmpStr);

	(void) fi;
	fd = open(fpath, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
    
{
	int res;
	char tmp[1024];
	sprintf(tmp, "%s", path);
	char *fpath = search(tmp, root, CmpStr);
	res = statvfs(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static struct fuse_operations xmp_oper = {
	.init		= pre_init,
	.getattr	= xmp_getattr,
	.readdir	= xmp_readdir,
	.read		= xmp_read,
    .statfs 	= xmp_statfs,
};

int main(int argc, char *argv[])
{
	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}

