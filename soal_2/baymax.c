#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

#define FRAGMENTS_DIR "/home/kali/modul4/soal_2/relics"
#define LOG_FILE "/home/kali/modul4/soal_2/activity.log"
#define MAX_FRAGMENT_SIZE 1024

// ----------------- Logging -----------------
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

// ----------------- Helpers -----------------
// Dapatkan nama file tanpa '/' depan
static const char *base_name(const char *path) {
    if (path[0] == '/') return path + 1;
    return path;
}

// Buat path fragment: relics/filename.xxx
static void build_fragment_path(const char *base, int idx, char *buf, size_t bufsize) {
    snprintf(buf, bufsize, "%s/%s.%03d", FRAGMENTS_DIR, base, idx);
}

// Hitung jumlah fragment berdasarkan keberadaan file di relics
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

// Hapus semua fragment file
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


// Hitung ukuran total gabungan pecahan file
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

// FUSE callback untuk atribut file/folder
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

// FUSE callback untuk baca isi direktori
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

// *BACA (opsi b)*
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

    // Log baca
    write_log("READ", fname);

    return bytes_read;
}

// *CREATE file baru (awal, kosong)*
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


// *TULIS (opsi c) - pecah file ke fragment 1KB*
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

// Log write dengan nama file dan fragment yang dibuat
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

// *TRUNCATE - hapus semua fragment jika ukuran 0*
static int baymax_truncate(const char *path, off_t size) {
    const char *fname = base_name(path);
    if (size == 0) {
        delete_fragments(fname);
    }
    return 0;
}

// *UNLINK (hapus file dan semua fragmen) (opsi d)*
static int baymax_unlink(const char *path) {
    const char *fname = base_name(path);
    delete_fragments(fname);
    return 0;
}

// *RENAME - tidak didukung*
static int baymax_rename(const char *from, const char *to) {
    // Tidak support rename karena sistem pecah fragmen
    return -ENOSYS;
}

// *FLUSH - tidak perlu apa2*
static int baymax_flush(const char *path, struct fuse_file_info *fi) {
    (void)path; (void)fi;
    return 0;
}

// *INIT - pastikan folder relics ada dan log siap*
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

// Struktur operasi FUSE
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

// MAIN
int main(int argc, char *argv[]) {
    // Clear log di awal run (optional)
    FILE *log = fopen(LOG_FILE, "w");
    if (log) fclose(log);

    return fuse_main(argc, argv, &baymax_oper, NULL);
}
