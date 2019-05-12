# FP_SISOP19_B10

## Fuse dan Thread

Buatlah sebuah music player dengan bahasa C yang memiliki fitur play nama_lagu, pause, next, prev, list lagu. Selain music player juga terdapat FUSE untuk mengumpulkan semua jenis file yang berekstensi .mp3 kedalam FUSE yang tersebar pada direktori /home/user. Ketika FUSE dijalankan, direktori hasil FUSE hanya berisi file .mp3 tanpa ada direktori lain di dalamnya. Asal file tersebut bisa tersebar dari berbagai folder dan subfolder. program mp3 mengarah ke FUSE untuk memutar musik.

-----

## Listing File .mp3 menggunakan FUSE

- Untuk melakukan listing file sekaligus dengan folder dan sub-folder, kita dapat melakukan listing secara rekursif. Pada dasarnya, rekursif ini dilakukan dengan cara :

    ```
    1.  Buka direktori.
    2.  Ketika melakukan listing direktori, lakukan :
    3.  Jika folder, maka rekursif untuk melisting folder tsb.
    4. Jika bukan folder, maka lakukan filler file tsb
    ```
- Pada FUSE, untuk melakukan listing direktori, menggunakan operasi **.xmp_readdir**. Agar bisa melakukan rekursif, maka perlu dibuatkan fungsi lain yang mirip dengan xmp_readdir. Fungsi tersebut adalah :

    ```c
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
    ```

- Fungsi yang mirip dengan xmp_readdir ditunjukkan pada fungsi **```list_dir()```**.

- Untuk mengecek apakah sedang membaca direktori apa bukan, dilakukan dengan fungsi.

    ```c
    if (de->d_type == DT_DIR) {
        char new_f[1024];
        int a = strcmp(de->d_name, ".");
        int b = strcmp(de->d_name, "..");
        if (!a || !b) continue;
        if (de->d_name[0] == '.') continue;
        snprintf(new_f, sizeof(new_f), "%s/%s", path, de->d_name);
        res = list_dir(new_f, buf, filler, offset, fi);
    }
    ```

- Jika bukan direktori, maka dilakukan filler terhadap file tersebut. Namun, pada kasus ini, yang perlu dilist adalah file .mp3 saja.

    ```c
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
    ```

- Untuk mengecek apakah file tersebut adalah file .mp3, kita dapat menggunakan **regex**. Bentuk regexnya adalah **".\*\\.mp3$"**, yang artinya mengecek apakah ekstensi file tersebut mp3 atau bukan.

    ```c
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
    ```

### Menggunakan, struktur data untuk menyimpan nama file dan path

Karena kita bekerja dengan cara rekursif, maka file-file yang berada pada folder **mount point** hanyalah file-filenya, tidak ada folder/sub-folder. Maka dari itu, kita perlu untuk mengetahui **original path** dari nama-nama file yang sudah ada.

Mengapa hal ini perlu? Karena attribut-attribut dari file diperlukan pada saat proses **xmp_getattr** dan **xmp_read**. Untuk melakukan proses tersebut, memerlukan path asli dari file yang sedang dibaca/read.

Maka dari itu, butuh Struktur Data yang dapat menyimpan **nama file** dan **original path**. Struktur Data yang akan digunakan disini adalah Binary Search.

-- GAMBAR --

Struktur Nodenya menyimpan :

```c
typedef struct t_node {
    char *value;  
    char *path;
    struct t_node *p_left;
    struct t_node *p_right;
    int cnt;
}node;
```

- ```char *path``` dan ```char *value``` menyimpan original path dan nama file.

- variabel ```int cnt``` digunakan untuk menandai jumlah duplikasi dari nama file tersebut.

### Fungsi-fungsi dasar Binary Tree

- **Fungsi Insert**

    ```c
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
    ```

    Fungsi insert() ini digunakan untuk memasukkan nama file dan original path dari sebuah file ketika melakukan listing file.

- **Fungsi Search**

    ```c
    char *search(char* key, node* leaf, Compare cmp)
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
    ```
    Fungsi search() digunakan untuk mencari original path dari sebuah file (menerima parameter berupa nama file).

- **Fungsi Search_Path**

    ```c
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
    ```

    Fungsi search_path() digunakan untuk mencari path ketika terdapat file yang namanya duplikat. Parameternya menerima path dari file yang duplikat.

## Menghubungkan program MP3 Player dengan FUSE