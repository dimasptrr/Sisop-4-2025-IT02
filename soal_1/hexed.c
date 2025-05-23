#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#define MAX_HEX_SIZE 2000000

const char *real_dir = "/home/kali/modul4/soal_1/anomali";
const char *out_dir = "/home/kali/modul4/soal_1/image";
#define LOG_PATH "/home/kali/modul4/soal_1/conversion.log"

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
