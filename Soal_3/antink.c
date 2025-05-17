#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

static const char *real_root = "/original";
static const char *log_path = "/logs/access.log";

static void log_access(const char *operation, const char *path) {
    FILE *log = fopen(log_path, "a");
    if (log) {
        fprintf(log, "%s: %s\n", operation, path);
        fclose(log);
    }
}

static int antink_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", real_root, path);
    int res = lstat(fullpath, stbuf);
    return (res == -1) ? -errno : 0;
}

static int antink_open(const char *path, struct fuse_file_info *fi) {
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", real_root, path);
    int fd = open(fullpath, fi->flags);
    if (fd == -1) return -errno;
    close(fd);
    log_access("OPEN", path);
    return 0;
}

static int antink_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", real_root, path);
    int fd = open(fullpath, O_RDONLY);
    if (fd == -1) return -errno;
    lseek(fd, offset, SEEK_SET);
    int res = read(fd, buf, size);
    close(fd);
    if (res >= 0) log_access("READ", path);
    return res;
}

static int antink_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags) {
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", real_root, path);
    DIR *dp = opendir(fullpath);
    if (!dp) return -errno;
    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        filler(buf, de->d_name, NULL, 0, 0);
    }
    closedir(dp);
    return 0;
}

static struct fuse_operations antink_oper = {
    .getattr = antink_getattr,
    .open = antink_open,
    .read = antink_read,
    .readdir = antink_readdir,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &antink_oper, NULL);
}
