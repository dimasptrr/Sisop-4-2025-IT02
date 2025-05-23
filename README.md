# Sisop-4-2025-IT02
# Soal Pertama
## The Shorekeeper
```c
const char *real_dir = "/home/kali/modul4/soal_1/anomali";
const char *out_dir = "/home/kali/modul4/soal_1/image";
#define LOG_PATH "/home/kali/modul4/soal_1/conversion.log"
```
- `real_dir`: menunjuk ke folder sumber (anomali) yang berisi file asli.
- `out_dir`: menunjuk ke folder tujuan (image) tempat hasil konversi disimpan.
- `LOG_PATH`: path ke file log (conversion.log) untuk mencatat proses konversi.
```c
int convert_hex_file_to_png(const char *input_base, char *outpng, size_t outsize);
int find_latest_png_for_txt(const char *input_base, char *outpng, size_t outsize);

int is_hex_char(char c) {
    return isxdigit(c);
}

int hex_char_to_int(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    return -1;
}
int hex_to_bytes(const char *hex, unsigned char *bytes, int hex_len) {
    if (hex_len % 2 != 0) hex_len--;
    for (int i = 0; i < hex_len; i += 2) {
        int hi = hex_char_to_int(hex[i]);
        int lo = hex_char_to_int(hex[i + 1]);
        if (hi == -1 || lo == -1) return -1;
        bytes[i / 2] = (hi << 4) | lo;
    }
    return hex_len / 2;
}
```
- `convert_hex_file_to_png & find_latest_png_for_txt`: deklarasi fungsi, kemungkinan digunakan untuk 
 mengonversi file teks berisi hex ke PNG dan mencari PNG terbaru dari file teks.
- `is_hex_char`: mengecek apakah karakter merupakan karakter heksadesimal (0-9, a-f, A-F).
- `hex_char_to_int`: mengubah karakter heksadesimal menjadi nilai integer (contoh: 'A' → 10).
- `hex_to_bytes`: mengubah string heksadesimal menjadi array byte. Dua karakter hex dikonversi menjadi satu byte.
```c
void timestamp(char *date, size_t dsize, char *timeStr, size_t tsize) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(date, dsize, "%Y-%m-%d", t);
    strftime(timeStr, tsize, "%H:%M:%S", t);
}

void make_output_filename(const char *input_name, char *outname, size_t size) {
    char date[16], time[16];
    timestamp(date, sizeof(date), time, sizeof(time));
    snprintf(outname, size, "%s/%s_image_%s_%s.png", out_dir, input_name, date, time);
}
```
- `timestamp`: mengambil waktu saat ini dan memformatnya menjadi tanggal (YYYY-MM-DD) dan waktu (HH:MM:SS).
- `make_output_filename`: membuat nama file output PNG berdasarkan nama file input, ditambah timestamp, 
 lalu disimpan ke dalam folder out_dir.
```c
int convert_hex_file_to_png(const char *input_base, char *outpng, size_t outsize) {
    if (find_latest_png_for_txt(input_base, outpng, outsize) == 0) {
        char date[16], timeStr[16];
        timestamp(date, sizeof(date), timeStr, sizeof(timeStr));

        FILE *log = fopen(LOG_PATH, "a");
        if (log) {
            fprintf(log, "[%s][%s]: File %s already converted as %s\n",
                    date, timeStr, input_base, outpng);
            fclose(log);
        }
        return 0;
    }

    char txtfilename[256];
    snprintf(txtfilename, sizeof(txtfilename), "%s/%s.txt", real_dir, input_base);

    FILE *fp = fopen(txtfilename, "r");
    if (!fp) return -ENOENT;

    char *hex = malloc(MAX_HEX_SIZE);
    int len = 0, ch;
    while ((ch = fgetc(fp)) != EOF && len < MAX_HEX_SIZE - 1) {
        if (is_hex_char(ch)) hex[len++] = ch;
    }
    fclose(fp);

    if (len % 2 != 0) len--;

    unsigned char *bytes = malloc(len / 2);
    int bytelen = hex_to_bytes(hex, bytes, len);
    if (bytelen <= 0) {
        free(hex);
        free(bytes);
        return -EIO;
    }

    make_output_filename(input_base, outpng, outsize);
    FILE *out = fopen(outpng, "wb");
    if (!out) {
        free(hex);
        free(bytes);
        return -EIO;
    }
    fwrite(bytes, 1, bytelen, out);
    fclose(out);

    char date[16], timeStr[16];
    timestamp(date, sizeof(date), timeStr, sizeof(timeStr));
    FILE *log = fopen(LOG_PATH, "a");
    if (log) {
        fprintf(log, "[%s][%s]: Successfully converted hexadecimal text %s.txt to %s\n",
                date, timeStr, input_base, outpng);
        fclose(log);
    }

    free(hex);
    free(bytes);
    return 0;
}
```
- Fungsi `convert_hex_file_to_png`:
    - Mengecek apakah file sudah pernah dikonversi.
    - Jika belum, membaca file teks hex, mengubahnya ke byte.
    - Menyimpan hasilnya sebagai file PNG dengan nama berisi timestamp.
    - Mencatat proses ke file log.
    - Menghindari konversi ulang dan menangani error.
```c
int find_latest_png_for_txt(const char *input_base, char *outpng, size_t outsize) {
    DIR *dp = opendir(out_dir);
    if (!dp) return -ENOENT;

    char prefix[256];
    snprintf(prefix, sizeof(prefix), "%s_image_", input_base);

    struct dirent *de;
    time_t latest_mtime = 0;
    char latest_file[512] = {0};

    while ((de = readdir(dp)) != NULL) {
        if (strncmp(de->d_name, prefix, strlen(prefix)) == 0 && strstr(de->d_name, ".png")) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", out_dir, de->d_name);
            struct stat st;
            if (stat(filepath, &st) == 0 && st.st_mtime > latest_mtime) {
                latest_mtime = st.st_mtime;
                strncpy(latest_file, filepath, sizeof(latest_file));
            }
        }
    }
    closedir(dp);

    if (latest_mtime > 0) {
        strncpy(outpng, latest_file, outsize);
        outpng[outsize - 1] = 0;
        return 0;
    }
    return -ENOENT;
}
```
- Fungsi `find_latest_png_for_txt`:
    - Membuka direktori output (out_dir).
    - Mencari file PNG yang namanya diawali dengan <input_base>_image_.
    - Menemukan file PNG terbaru berdasarkan waktu modifikasi.
    - Jika ditemukan, mengembalikan path file terbaru lewat outpng dan return 0.
    - Jika tidak ada, return error -ENOENT.
```c
static int do_getattr(const char *path, struct stat *st) {
    memset(st, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0 || strcmp(path, "/image") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
        return 0;
    }
    if (strcmp(path, "/conversion.log") == 0) {
        return stat(LOG_PATH, st);
    }
    if (strncmp(path, "/image/", 7) == 0) {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", out_dir, path + 7);
        return stat(filepath, st);
    }
    if (strstr(path, ".txt")) {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", real_dir, path + 1);
        return stat(filepath, st);
    }
    return -ENOENT;
}
```
- Fungsi `do_getattr`:
    - Mengisi informasi atribut file atau direktori berdasarkan path virtual.
    - Root `(/)` dan `/image` dianggap direktori.
    - `/conversion.log` mengarah ke file log asli.
    - File di `/image/` di-mapping ke file di direktori output `(out_dir)`.
    - File .txt di-mapping ke file asli di real_dir.
    - Jika path tidak dikenali, kembalikan error -ENOENT.
```c
static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    if (strcmp(path, "/") == 0) {
        DIR *dp = opendir(real_dir);
        if (!dp) return -ENOENT;

        struct dirent *de;
        while ((de = readdir(dp)) != NULL) {
            if (strstr(de->d_name, ".txt")) {
                filler(buf, de->d_name, NULL, 0);
            }
        }
        closedir(dp);

        filler(buf, "conversion.log", NULL, 0);
        filler(buf, "image", NULL, 0);
    } else if (strcmp(path, "/image") == 0) {
        DIR *dp = opendir(out_dir);
        if (!dp) return -ENOENT;

        struct dirent *de;
        while ((de = readdir(dp)) != NULL) {
            if (strstr(de->d_name, ".png")) {
                filler(buf, de->d_name, NULL, 0);
            }
        }
        closedir(dp);
    }
    return 0;
}
```
- Fungsi `do_readdir`:
    - Mengisi daftar isi direktori virtual untuk path yang diminta.
    - Untuk root /: menampilkan semua file .txt di real_dir, plus conversion.log dan direktori image.
    - Untuk /image: menampilkan semua file .png di out_dir.
    - Menambahkan entri . dan .. di setiap direktori.
```c
static int do_open(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path, "/conversion.log") == 0) return 0;

    if (strncmp(path, "/image/", 7) == 0) {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", out_dir, path + 7);
        FILE *fp = fopen(filepath, "rb");
        if (!fp) return -ENOENT;
        fclose(fp);
        return 0;
    }
    if (strstr(path, ".txt")) {
        char name[256];
        strncpy(name, path + 1, strlen(path) - 5);
        name[strlen(path) - 5] = '\0';

        char outpng[512];
        if (find_latest_png_for_txt(name, outpng, sizeof(outpng)) != 0) {
            if (convert_hex_file_to_png(name, outpng, sizeof(outpng)) != 0)
                return -ENOENT;
        }
        return 0;
    }
    return -ENOENT;
}
```
- Fungsi `do_open`:
    - Membuka file virtual yang diminta.
    - Untuk /conversion.log dan file PNG di /image/, cek keberadaan file asli, lalu buka dan tutup untuk verifikasi.
    - Untuk file .txt, cari PNG hasil konversi terbaru; jika belum ada, lakukan konversi dulu.
    - Jika file tidak ada atau gagal, kembalikan error -ENOENT.
```c
static int do_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    const char *filepath;
    char realpath[512];

    if (strcmp(path, "/conversion.log") == 0) {
        filepath = LOG_PATH;
    } else if (strncmp(path, "/image/", 7) == 0) {
        snprintf(realpath, sizeof(realpath), "%s/%s", out_dir, path + 7);
        filepath = realpath;
    } else {
        return -ENOENT;
    }

    FILE *fp = fopen(filepath, "rb");
    if (!fp) return -ENOENT;

    fseek(fp, 0, SEEK_END);
    off_t filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (offset < filesize) {
        if (offset + size > filesize) size = filesize - offset;
        fseek(fp, offset, SEEK_SET);
        fread(buf, 1, size, fp);
    } else {
        size = 0;
    }

    fclose(fp);
    return size;
}

static struct fuse_operations ops = {
    .getattr = do_getattr,
    .readdir = do_readdir,
    .open    = do_open,
    .read    = do_read,
};
```
Penjelasan fungsi `do_read` dan `struktur ops`:
- `do_read`:
 - Membaca isi file virtual dari path yang diberikan (/conversion.log atau file PNG di /image/).
   Membuka file asli, membaca data mulai dari offset sebanyak size byte, lalu mengembalikan jumlah byte yang dibaca. Jika offset melebihi ukuran file, baca 0 byte.
- `ops`:
 - Struktur yang menghubungkan operasi FUSE dengan fungsi-fungsi implementasi seperti getattr, readdir, open, dan read di filesystem virtual ini.
```c
int main(int argc, char *argv[]) {
    struct stat st = {0};
    if (stat(out_dir, &st) == -1) mkdir(out_dir, 0755);
    FILE *log = fopen(LOG_PATH, "a"); if (log) fclose(log);

    const char *mount_dir = "/home/kali/modul4/soal_1/mnt";
    if (argc > 1) mount_dir = argv[argc - 1];
    if (stat(mount_dir, &st) == -1) mkdir(mount_dir, 0755);

    char *fuse_argv[argc + 1];
    for (int i = 0; i < argc - 1; i++) fuse_argv[i] = argv[i];
    fuse_argv[argc - 1] = (char *)mount_dir;
    fuse_argv[argc] = NULL;

    return fuse_main(argc, fuse_argv, &ops, NULL);
}
```
- Penjelasan fungsi `main`:
    - Memastikan direktori output (`out_dir`) dan direktori mount (`mount_dir`) ada, buat jika belum.
    - Membuka dan menutup file log untuk memastikan file log siap dipakai.
    - Menentukan direktori mount dari argumen program (default /home/kali/modul4/soal_1/mnt).
    - Menyiapkan argumen untuk FUSE dan menjalankan fuse_main untuk mount filesystem virtual dengan operasi ops.
  
# Soal Kedua
## Baymax
```c
void write_log(const char *action, const char *details) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm tstruct = *localtime(&now);
    char timebuf[20];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tstruct);

    fprintf(log, "[%s] %s: %s\n", timebuf, action, details);
    fclose(log);
}

static const char *base_name(const char *path) {
    if (path[0] == '/') return path + 1;
    return path;
}

static void build_fragment_path(const char *base, int idx, char *buf, size_t bufsize) {
    snprintf(buf, bufsize, "%s/%s.%03d", FRAGMENTS_DIR, base, idx);
}
```
- Berikut penjelasan `write_log`, `base_name`, dan `build_fragment_path`:
    - `write_log`: mencatat aksi (`action`) dan detailnya (`details`) ke file log dengan timestamp.
    - `base_name`: mengambil nama file tanpa leading slash / dari path.
    - `build_fragment_path`: membuat path lengkap untuk fragmen file dengan format `FRAGMENTS_DIR/base.xxx` di mana xxx adalah nomor fragmen (3 digit).
```c
static int count_fragments(const char *base) {
    int count = 0;
    char path[256];
    while (1) {
        build_fragment_path(base, count, path, sizeof(path));
        if (access(path, F_OK) != 0) break;
        count++;
    }
    return count;
}


static void delete_fragments(const char *base) {
    int count = count_fragments(base);
    if (count == 0) return;

    char details[512] = {0};
    snprintf(details, sizeof(details), "%s.000 - %s.%03d", base, base, count-1);

    for (int i=0; i<count; i++) {
        char fragpath[256];
        build_fragment_path(base, i, fragpath, sizeof(fragpath));
        unlink(fragpath);
    }

    write_log("DELETE", details);
}
```
- Penjelasan kode `count_fragments` dan `delete_fragments`:
    - `count_fragments`: menghitung berapa banyak fragmen file yang ada dengan mengecek keberadaan file fragmen berurutan (0, 1, 2, ...) sampai tidak ditemukan lagi.
    - `delete_fragments`: menghapus semua fragmen dari file berdasarkan jumlah fragmen yang dihitung, lalu mencatat aksi penghapusan ke log dengan rentang fragmen yang dihapus.
```c
static off_t get_file_size(const char *base) {
    off_t total = 0;
    char path[256];
    int idx = 0;
    struct stat st;

    while (1) {
        build_fragment_path(base, idx, path, sizeof(path));
        if (stat(path, &st) != 0) break; // Pecahan tidak ada lagi
        total += st.st_size;
        idx++;
    }
    return total;
}


static int baymax_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    const char *fname = base_name(path);
    off_t size = get_file_size(fname);

    if (size > 0) {
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = size;
        return 0;
    }

    return -ENOENT;
}
```
- Penjelasan kode `get_file_size` dan `baymax_getattr` :
 - `get_file_size`: menjumlahkan ukuran semua fragmen file berdasarkan base nama, mengembalikan total ukuran gabungan.
 - `baymax_getattr`: mengisi atribut file/direktori untuk path virtual.
    - Jika root /, set sebagai direktori.
    - Jika file ditemukan (fragmen ada), set sebagai file biasa dengan ukuran total fragmen.
    - Jika tidak ditemukan, kembalikan error -ENOENT.
```c
static int baymax_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi) {
    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    // Tampilkan satu file utama hasil gabungan pecahan
    filler(buf, "Baymax.jpeg", NULL, 0);

    return 0;
}

static int baymax_open(const char *path, struct fuse_file_info *fi) {
    const char *fname = base_name(path);
    if (get_file_size(fname) == 0) return -ENOENT;

    // izinkan baca/tulis sesuai mode
    if ((fi->flags & O_ACCMODE) != O_RDONLY &&
        (fi->flags & O_ACCMODE) != O_WRONLY &&
        (fi->flags & O_ACCMODE) != O_RDWR)
        return -EACCES;

    return 0;
}
```
Penjelasan singkat fungsi-fungsi ini:
- `baymax_readdir`:
 - Untuk direktori root /, menampilkan entri . dan .. serta satu file virtual bernama `"Baymax.jpeg"` (hasil gabungan fragmen).
- `baymax_open`:
- Memeriksa apakah file virtual yang diminta ada (dengan cek ukuran total fragmen).
  Mengizinkan akses baca/tulis hanya jika mode akses valid (read, write, atau read-write).
  Jika file tidak ada, kembalikan error -ENOENT.
```c
static int baymax_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    const char *fname = base_name(path);
    off_t file_size = get_file_size(fname);
    if (offset >= file_size) return 0;

    if (offset + size > file_size) size = file_size - offset;

    size_t bytes_read = 0;
    off_t current_offset = 0;

    for (int i=0;; i++) {
        char fragpath[256];
        build_fragment_path(fname, i, fragpath, sizeof(fragpath));

        FILE *fp = fopen(fragpath, "rb");
        if (!fp) break;

        fseek(fp, 0, SEEK_END);
        size_t frag_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (offset < current_offset + frag_size) {
            size_t skip = offset > current_offset ? offset - current_offset : 0;
            fseek(fp, skip, SEEK_SET);

            size_t to_read = frag_size - skip;
            if (to_read > size - bytes_read) to_read = size - bytes_read;

            fread(buf + bytes_read, 1, to_read, fp);
            bytes_read += to_read;
            offset += to_read;

            fclose(fp);
            if (bytes_read >= size) break;
        } else {
            fclose(fp);
        }

        current_offset += frag_size;
    }

    write_log("READ", fname);

    return bytes_read;
}
```
- Penjelasan singkat fungsi `baymax_read`:
    - Membaca data file virtual "Baymax.jpeg" yang merupakan gabungan beberapa fragmen file.
    - Menghitung ukuran total dan membaca hanya bagian yang diminta (offset dan size).
    - Membuka fragmen satu per satu, lompat sesuai offset, baca sebagian data, dan gabungkan hasilnya ke buffer buf.
    - Menulis log aksi "READ" dengan nama file.
    - Mengembalikan jumlah byte yang berhasil dibaca.
```c
static int baymax_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) mode;
    const char *fname = base_name(path);

    // Buat fragment pertama file kosong supaya file ada di virtual FS
    char fragpath[256];
    build_fragment_path(fname, 0, fragpath, sizeof(fragpath));

    FILE *fp = fopen(fragpath, "wb");
    if (fp) fclose(fp);

    write_log("CREATE", fname);
    return 0;
}
```
- Penjelasan singkat fungsi `baymax_create`:
    - Membuat file baru secara virtual dengan membuat fragmen pertama (index 0) berupa file kosong di direktori fragmen.
    - Ini menandakan file tersebut ada di sistem file virtual.
    - Mencatat aksi pembuatan file ke log dengan write_log.
    - Mengembalikan 0 jika sukses.
```c
static int baymax_write(const char *path, const char *buf, size_t size,
                        off_t offset, struct fuse_file_info *fi) {
    const char *fname = base_name(path);

    // Jika offset 0, hapus fragment lama (overwrite file)
    if (offset == 0) delete_fragments(fname);

    size_t total_written = 0;
    int frag_start = offset / MAX_FRAGMENT_SIZE;
    int frag_offset = offset % MAX_FRAGMENT_SIZE;

    while (total_written < size) {
        char fragpath[256];
        build_fragment_path(fname, frag_start, fragpath, sizeof(fragpath));

        FILE *fp = fopen(fragpath, "r+b");
        if (!fp) fp = fopen(fragpath, "w+b");
        if (!fp) return -EIO;

        // Pastikan fragmen minimal ukuran frag_offset + to_write
        fseek(fp, 0, SEEK_END);
        long frag_size = ftell(fp);
        if (frag_size < MAX_FRAGMENT_SIZE) {
            // Extend fragmen dengan 0 jika perlu
            for (long i = frag_size; i < MAX_FRAGMENT_SIZE; i++) {
                fputc(0, fp);
            }
        }

        fseek(fp, frag_offset, SEEK_SET);

        size_t to_write = MAX_FRAGMENT_SIZE - frag_offset;
        if (to_write > size - total_written) to_write = size - total_written;

        fwrite(buf + total_written, 1, to_write, fp);
        fclose(fp);

        total_written += to_write;
        frag_start++;
        frag_offset = 0;
    }

int frag_count = count_fragments(fname);
char details[512];
if (frag_count > 0) {
    char frags[480] = "";
    for (int i = 0; i < frag_count; i++) {
        char fragname[32];
        snprintf(fragname, sizeof(fragname), "%s.%03d", fname, i);
        strcat(frags, fragname);
        if (i != frag_count - 1)
            strcat(frags, ", ");
    }
    snprintf(details, sizeof(details), "%s -> %s", fname, frags);
} else {
    snprintf(details, sizeof(details), "%s", fname);
}
write_log("WRITE", details);
return size;
}
```
- Berikut penjelasan singkat fungsi `baymax_write`:
    - Jika menulis dari offset 0, hapus dulu semua fragmen lama (overwrite file).
    - Hitung fragmen mulai (`frag_start`) dan offset di dalam fragmen (`frag_offset`) berdasarkan offset global.
    - Loop menulis data ke fragmen-fragmen satu per satu, buat fragmen baru jika belum ada.
    - Setiap fragmen dipastikan memiliki ukuran maksimal `MAX_FRAGMENT_SIZE` dengan mengisi 0 jika perlu.
    - Setelah selesai menulis semua data, buat string log yang menampilkan fragmen yang sudah ada.
    - Catat aksi tulis (`WRITE`) ke log dengan daftar fragmen yang terlibat.
    - Kembalikan jumlah byte yang ditulis (`size`).
```c
static int baymax_truncate(const char *path, off_t size) {
    const char *fname = base_name(path);
    if (size == 0) {
        delete_fragments(fname);
    }
    return 0;
}

static int baymax_unlink(const char *path) {
    const char *fname = base_name(path);
    delete_fragments(fname);
    return 0;
}

static int baymax_rename(const char *from, const char *to) {
    // Tidak support rename karena sistem pecah fragmen
    return -ENOSYS;
}

static int baymax_flush(const char *path, struct fuse_file_info *fi) {
    (void)path; (void)fi;
    return 0;
}

static void *baymax_init(struct fuse_conn_info *conn) {
    (void)conn;
    struct stat st = {0};
    if (stat(FRAGMENTS_DIR, &st) == -1) {
        mkdir(FRAGMENTS_DIR, 0755);
    }
    FILE *log = fopen(LOG_FILE, "a");
    if (log) fclose(log);
    return NULL;
}
```
- `baymax_truncate`:
 - Jika ukuran dipotong ke 0, hapus semua fragmen file terkait.
- `baymax_unlink`:
 - Hapus semua fragmen file yang dihapus dari sistem file virtual.
- `baymax_rename`:
 - Rename tidak didukung karena file terpecah dalam fragmen yang sulit dikelola.
- `baymax_flush`:
 - Fungsi kosong, hanya placeholder untuk flush data (tidak melakukan apa-apa).
- `baymax_init`:
 - Inisialisasi filesystem, membuat direktori pecahan (FRAGMENTS_DIR) jika belum ada, dan membuka/tutup file log untuk memastikan file log siap.
```c
static struct fuse_operations baymax_oper = {
    .getattr = baymax_getattr,
    .readdir = baymax_readdir,
    .open = baymax_open,
    .read = baymax_read,
    .create = baymax_create,
    .write = baymax_write,
    .truncate = baymax_truncate,
    .unlink = baymax_unlink,
    .rename = baymax_rename,
    .flush = baymax_flush,
    .init = baymax_init,
};
```
- .getattr → baymax_getattr: Mengatur atribut file/direktori (ukuran, mode, dll).
- .readdir → baymax_readdir: Membaca dan menampilkan isi direktori.
- .open → baymax_open: Membuka file untuk operasi baca/tulis.
- .read → baymax_read: Membaca data dari file yang sudah dibuka
- .create → baymax_create: Membuat file baru di filesystem virtual.
- .write → baymax_write: Menulis data ke file (fragmented).
- .truncate → baymax_truncate: Memotong atau mengosongkan file.
- .unlink → baymax_unlink: Menghapus file beserta fragmennya.
- .rename → baymax_rename: Rename file (tidak didukung, return error).
- .flush → baymax_flush: Flush data (biasanya sync, tapi kosong di sini).
- .init → baymax_init: Inisialisasi filesystem saat mount dimulai.
```c
int main(int argc, char *argv[]) {
    // Clear log di awal run (optional)
    FILE *log = fopen(LOG_FILE, "w");
    if (log) fclose(log);

    return fuse_main(argc, argv, &baymax_oper, NULL);
}
```
- `main` function ini berfungsi untuk:
    - Mengosongkan (`clear`) isi file log (`LOG_FILE`) saat program mulai dijalankan supaya log sebelumnya hilang.
    - Memanggil `fuse_main` untuk menjalankan filesystem FUSE dengan operasi yang sudah didefinisikan di baymax_oper.
    - Menerima argumen command line `argc` dan `argv` untuk pengaturan mount point dan opsi lainnya.

# Soal Ketiga
## fuse_antink
```c
    static void log_access(const char *operation, const char *path)

    FILE *log = fopen(log_path, "a");  // buka file log (append mode)
fprintf(log, "%s: %s\n", operation, path);  // tulis log
fclose(log);

```
Fungsi ini mencatat operasi file (OPEN, READ, dll.) ke file log.
Penting: Di sinilah logger melihat aktivitas file.

```c
    static int antink_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)

    snprintf(fullpath, sizeof(fullpath), "%s%s", real_root, path); // gabungkan path
int res = lstat(fullpath, stbuf);  // ambil info file asli

```
Digunakan oleh FUSE untuk mengambil metadata file (ukuran, waktu modifikasi, dll.)
Ini harus ada supaya sistem file FUSE bisa "melihat" struktur file asli.

```c
    static int antink_open(const char *path, struct fuse_file_info *fi)

    int fd = open(fullpath, fi->flags);  // buka file asli
log_access("OPEN", path);            // log aktivitas

```
Fungsi ini menangani aksi ketika user membuka file (misalnya cat).
Ini juga log aktivitas ke file log!

```c
    static int antink_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)

int fd = open(fullpath, O_RDONLY);  // buka file asli
lseek(fd, offset, SEEK_SET);        // loncat ke offset tertentu
int res = read(fd, buf, size);      // baca data
log_access("READ", path);           // log aktivitas

```
Fungsi ini menangani membaca isi file.
Di sinilah kita bisa deteksi baca file mencurigakan (misalnya nama file mengandung "rahasia").

```c
    static int antink_readdir(...)

    DIR *dp = opendir(fullpath);     // buka direktori asli
while ((de = readdir(dp)) != NULL) {
    filler(buf, de->d_name, NULL, 0, 0);  // masukkan tiap nama file ke output
}

```
Fungsi ini digunakan oleh FUSE untuk membaca isi direktori (misal saat ls).
Wajib untuk bisa melihat file di dalam mount point.

```c
int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &antink_oper, NULL);
}
```
Fungsi utama — memulai FUSE dan mendaftarkan operasi yang kita definisikan (getattr, read, dll).

## logger
```c
    FILE *file = fopen(LOG_FILE, "r");
fseek(file, 0, SEEK_END);  // lompat ke akhir file

while (1) {
    if (fgets(line, sizeof(line), file)) {
        printf("[AntiNK-LOG] %s", line);  // tampilkan log baru
    } else {
        usleep(500000);  // tunggu 0.5 detik jika belum ada baris baru
    }
}

```
Ini baca terus menerus isi file log, cocok untuk ditaruh di container logging real-time.

```c
    if (strstr(path, "rahasia") || strstr(path, "nafis") || strstr(path, "kimcun")) {
    log_access("ALERT", path);
}

```
Saat ini sistem ini mendeteksi aktivitas file. Untuk meningkatkan deteksi "kenakalan" Nafis & Kimcun, kamu bisa
Deteksi jumlah akses berulang bisa ditambahkan di masa depan (misal pakai cache atau count).

# Soal Keempat
## Persiapan Umum
```bash
cd ~/Sisop-4-2025-IT02/soal_4
gcc -o maimai_fs maimai_fs.c -Wall $(pkg-config fuse --cflags --libs) -lz
fusermount -u /tmp/fuse_maimai 2>/dev/null || true
rm -rf /tmp/fuse_maimai; mkdir /tmp/fuse_maimai
./maimai_fs -f /tmp/fuse_maimai &
```

A. Starter Chiho
Backend: chiho/starter/*.mai

Tanpa transform, hanya sembunyikan .mai.

```c
// write
backend_path(bp, "/starter/foo.txt");  // → .../starter/foo.txt.mai
pwrite(fd, buf, size, off);
// read
pread(fd, buf, size, off);
```

Tes

```bash
echo "Halo" > /tmp/fuse_maimai/starter/foo.txt
cat /tmp/fuse_maimai/starter/foo.txt    # → Halo
ls chiho/starter                       # foo.txt.mai
```

B. Metropolis Chiho
- Backend: chiho/metro/*.ccc
- Transform: shift byte (+i saat write, –i saat read).

```c
// write
shift_buf(tmp, size, 1);
pwrite(fd, tmp, size, off);
// read
pread(fd, buf, size, off);
shift_buf(buf, r, 0);
```

Tes

```bash
echo "XYZ" > /tmp/fuse_maimai/metro/bar.txt
cat /tmp/fuse_maimai/metro/bar.txt     # → XYZ
ls chiho/metro                         # bar.txt.ccc
```

C. Dragon Chiho
- Backend: chiho/dragon/*
- Transform: ROT13 on write/read.

```c
// write
rot13_buf(tmp, size);
pwrite(fd, tmp, size, off);
// read
pread(fd, buf, size, off);
rot13_buf(buf, r);
```

Tes

```bash
echo "HELLO" > /tmp/fuse_maimai/dragon/greet.txt
cat /tmp/fuse_maimai/dragon/greet.txt # → HELLO
cat chiho/dragon/greet.txt             # URYYB
```

D. Black Rose Chiho
- Backend: chiho/blackrose/*
- Passthrough (binary 1:1).

```c
// write
pwrite(fd, buf, size, off);
// read
pread(fd, buf, size, off);
```

Tes

```bash
cp ~/rand.bin /tmp/fuse_maimai/blackrose/rand.bin
cmp ~/rand.bin chiho/blackrose/rand.bin  # identical
```

E. Heaven Chiho
- Backend: chiho/heaven/*
- AES-256-CBC via openssl enc.

```c
// write
pwrite(temp, buf, size, off);
system("openssl enc -aes-256-cbc -K ... -iv ... -in temp -out backend");
// read
system("openssl enc -d -aes-256-cbc -K ... -iv ... -in backend -out temp");
pread(fd, buf, size, off);
```

Tes

```bash
echo "Secret" > /tmp/fuse_maimai/heaven/data.dat
cat /tmp/fuse_maimai/heaven/data.dat   # → Secret
hexdump -C chiho/heaven/data.dat       # ciphertext
```

F. Skystreet Chiho
- Backend: chiho/skystreet/*.gz
- gzip compress/decompress via zlib.

```c
// write
gzFile gz = gzopen(path, "wb"); gzwrite(gz, buf, size); gzclose(gz);
// read
gzFile gz = gzopen(path, "rb"); gzseek(gz, off, SEEK_SET);
r = gzread(gz, buf, size); gzclose(gz);
```

Tes

```bash
echo "Kair0s" > /tmp/fuse_maimai/skystreet/test.txt
cat /tmp/fuse_maimai/skystreet/test.txt  # → Kair0s
ls chiho/skystreet                       # test.txt.gz
```

G. 7sRef Chiho
- Proxy: /7sref/area_file → /area/file.
- Delegasi: parse area & file, lalu panggil handler area.

```c
if (strncmp(path, "/7sref/", 7)==0) {
  parse_7sref(path, area, file);
  snprintf(proxy, ... , "/%s/%s", area, file);
  return maimai_read(proxy, buf, size, off, fi);
}
```

Tes

```bash
echo "Ref" > /tmp/fuse_maimai/7sref/starter_intro.txt
cat /tmp/fuse_maimai/7sref/starter_intro.txt  # → Ref
cat /tmp/fuse_maimai/starter/intro.txt        # → Ref
ls /tmp/fuse_maimai/7sref                     # starter_intro.txt
```

Unmount ketika selesai:
```bash
fusermount -u /tmp/fuse_maimai
```







