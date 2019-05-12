# FP_SISOP19_B10

## Fuse dan Thread

Buatlah sebuah music player dengan bahasa C yang memiliki fitur play nama_lagu, pause, next, prev, list lagu. Selain music player juga terdapat FUSE untuk mengumpulkan semua jenis file yang berekstensi .mp3 kedalam FUSE yang tersebar pada direktori /home/user. Ketika FUSE dijalankan, direktori hasil FUSE hanya berisi file .mp3 tanpa ada direktori lain di dalamnya. Asal file tersebut bisa tersebar dari berbagai folder dan subfolder. program mp3 mengarah ke FUSE untuk memutar musik.

-----

## Listing File .mp3 menggunakan FUSE

### Listing Direktori secara rekursif

----

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

---

Karena kita bekerja dengan cara rekursif, maka file-file yang berada pada folder **mount point** hanyalah file-filenya, tidak ada folder/sub-folder. Maka dari itu, kita perlu untuk mengetahui **original path** dari nama-nama file yang sudah ada.

Mengapa hal ini perlu? Karena attribut-attribut dari file diperlukan pada saat proses **xmp_getattr** dan **xmp_read**. Untuk melakukan proses tersebut, memerlukan path asli dari file yang sedang dibaca/read.

Maka dari itu, butuh Struktur Data yang dapat menyimpan **nama file** dan **original path**. Struktur Data yang akan digunakan disini adalah Binary Search.

![alt text](/img/TREE.png)

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

- Pada bagian **pre_init()**, pertama yang dilakukan adalah meng-insert path root kedalam tree agar dapat root dapat dibaca (mengarah ke /home/user).

    ```c
    static void *pre_init(struct fuse_conn_info *conn)
    {
        /* --- */
        /* --- */
        insert("/","/home/bayulaxana", &root, CmpStr);
    }
    ```

- Sehingga pada fungsi **xmp_getattr**, kita membaca path asli dari nama file (didapatkan dari Binary Tree).

    ```c
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
    ```

### Mengimplementasikan operasi read agar file .mp3 dapat diputar

Agar file .mp3 yang telah dilist tadi dapat dibuka dan diputar, kita perlu mengimplementasikan fungsi **xmp_read**. Sama seperti fungsi **xmp_getattr**, pada fungsi ini kita memerlukan path asli dari nama filenya (didapatkan dari tree).

- Fungsi **xmp_read()**

    ```c
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
    ```

    > Untuk mencari original path dari nama file, dapat dilakukan dengan fungsi search()

## Integrasi program MP3 Player dengan FUSE

Setelah listing file .mp3 berhasil dilakukan, maka selanjutnya adalah membuat program MP3 Player yang dapat memainkan lagu dari direktori Fuse yang dibuat tadi.

Untuk membuat program MP3 Player, disini menggunakan library **libmpg123** dan **libao**.

Fitur-fitur yang diimplementasikan pada program MP3 Player ini adalah :

1. Fitur Play untuk memutar lagu (dengan memilih id lagu).
2. Fitur Pause/Resume untuk melakukan jeda lagu.
3. Fitur List untuk menampilkan daftar lagu yang tersedia.
4. Fitur Next untuk memutar lagu selanjutnya.
5. Fitur Prev untuk memutar lagu sebelumnya.
6. Fitur Stop untuk menghentikan pemutaran lagu.

### Implementasi Fitur

-----

Program MP3 ini mendapatkan list lagu dari folder mount Fuse yang telah dibuat sebelumnya. Untuk mendapatkan listing dari folder Fuse tersebut, maka diperlukan struktur data untuk menyimpan nama filenya.

Struktur data yang digunakan disini adalah **Linked List**. Linked List digunakan karena sifatnya sekuensial (berurutan), sehingga lebih mudah untuk mengatur Next/Prev.

Struktur Linked List :

```c
typedef struct t_node {
    struct t_node *next;
    char *value;
} Node;
```

Fungsi fungsi dasar yang diperlukan seperti linked list pada umumnya, yakni **Insert** (secara alphabetically) dan **Search**.

```c
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

int search(Node *head, char *str) {
    int a;
    while (head) {
        a = strcmp(head->value, str);
        if (a == 0) return 1;
        head = head->next;
    }
    return 0;
}

char *search_by_id(Node *head, int id) {
    if (id == 0 || id < 0) return " ";
    int cnt = 1;
    while(head) {
        if (id == cnt) return head->value;
        cnt++;
        head = head->next;
    }
    return " ";
}
```

Untuk melakukan listing, kita membaca direktori Fuse lalu setiap file yang didapat (nama file), di Insert ke dalam Linked List.

Fungsi untuk membaca direktori Fuse :

```c
char *source_path = "/home/bayulaxana/FP_SISOP19_B10/mounted";

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
```
> Dapat dilihat bahwa setiap nama file dimasukkan ke dalam Linked List.

### Thread yang digunakan

Terdapat dua thread yang digunakan dalam program ini. Thread pertama (ditunjukkan pada variabel **```tid[0]```**) digunakan untuk menjalankan input. Sedangkan thread kedua digunakan ditunjukkan pada variabel **```tid[1]```**, digunakan untuk melakukan Play lagu berdasarkan nama.

### 1. Fitur Play

- Implementasi fitur play disini menggunakan thread yang menerima parameter berupa Nama File yang hendak diputar. Pemutaran lagu menggunakan fungsi bawaan yang disediakan oleh library **libao** dan **libmpg123**. Berikut adalah implementasi fitur Play.

    ```c
    void *play_song(void *arg) {
        int flag = 0;
        char *str = ((char*)arg);
        char fpath[1024];
        sprintf(fpath, "%s/%s", source_path,str);

        while(1) {
            if (flag) {
                str = find_next(head, str);
                sprintf(fpath, "%s/%s", source_path,str);
                sprintf(now_playing, "%s", str);
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
    ```

- Pada fungsi **play_song()** ini diterapkan mekanisme **auto_play**, yakni setelah satu lagu selesai diputar, maka akan otomatis memutar lagu selanjutnya dalam list. Ini dilakukan dengan menggunakan looping **while(1)**. Mekanismenya sederhana, yakni :
    1. Ketika satu lagu selesai diputar, ambil nama lagu selanjutnya.
    2. Putar lagu berdasarkan nama tersebut.
    3. Lakukan step 1.

### 2. Fitur Pause/Resume

- Fitur Pause/Resume diimplementasikan dengan memanipulasi status Playing. Terdapat variabel **int play_status** yang digunakan untuk menandai apakah program sedang memainkan lagu atau tidak.

- Ketika lagu di-"pause", maka variabel **play_status** berubah menjadi FALSE, dan ketika lagu di-"resume", variabel **play_status** menjadi TRUE.

- Potongan kode yang mengimplementasikan fitur pause adalah :

    ```c
    int errm = mpg123_read(mh, buffer, buffer_size, &done);
    while (1){
        if(play_status){ // paused
            ao_play(dev, buffer, done);
            errm = mpg123_read(mh, buffer, buffer_size, &done);

            if(errm != MPG123_OK) break;
        }
        sleep(0.01);
    }
    ```

    > Jadi pada prinsipnya, ketika sedang memainkan lagu, maka variabel **```int erm```** akan terus me-read buffer dari file .mp3 yang sedang dibaca. Ketika "pause", variabel tersebut akan seolah tertunda untuk membaca buffer, sehingga tidak akan diputar oleh **```ao_play()```**.

### 3. Fitur List

- Untuk menampilkan list dari lagu, maka dapat melakukan traversing pada **linked list** dan mencetak nama filenya selagi melakukan traversing.

    ```c
    void display_list(Node *head) {
        if (head == NULL) {
            printf("\n  No Files Found\n");
            return;
        }
        puts("");
        int cnt = 1;
        while(head) {
            printf("  |- %d %s\n",cnt++, head->value);
            head = head->next;
        }
    }
    ```

### 4. Fitur Stop

- Implementasi fitur Stop ini adalah dengan men-cancel thread **```play_song()```** yang sedang berjalan dan mengubah status **play_status** menjadi FALSE.

    ```c
    // -----
    else if (strcmp("stop", input) == 0) {
        pthread_cancel(tid[1]);
        play_status = FALSE;
    }
    // -----
    ```

### 5. Fitur Prev

- Implementasi fitur Prev dilakukan dengan traversing pada Linked List, kemudian jika menemukan nama yang sesuai dengan lagu yang diputar sekarang, akan memutar lagu dengan nama sebelumnya dalam Linked List.

    ```c
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
    ```

### 6. Fitur Next

- Implementasi fitur Next dilakukan dengan traversing pada Linked List, kemudian jika menemukan nama yang sesuai dengan lagu yang diputar sekarang, akan memutar lagu dengan nama setelahnya dalam Linked List.

    ```c
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
    ```

## Sekian dan Terima Kasih