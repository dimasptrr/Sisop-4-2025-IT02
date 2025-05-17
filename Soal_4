  GNU nano 8.3                                                                                                                                                                                                                                                                                                                                                                                                    maimai_fs.c *                                                                                                                                                                                                                                                                                                                                                                                                           
=/*
 * maimai_fs.c – FUSE filesystem untuk maimai universe
 *
 * Implementasi gabungan:
 * [a] Starter/Beginning Area  – Starter Chiho
 * [b] World's End Area       – Metropolis Chiho
 * [c] World Tree Area        – Dragon Chiho
 * [d] Black Rose Area        – Black Rose Chiho
 * [e] Tenkai Area            – Heaven Chiho
 * [f] Youth Area             – Skystreet Chiho
 * [g] Prism Area             – 7sRef Chiho
 *
 * Backend directory:
 *   ~/Sisop-4-2025-IT02/soal_4/chiho/{starter,metro,dragon,blackrose,heaven,skystreet}
 */

#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <zlib.h>

static const char *base_dir      = "/home/evancn/Sisop-4-2025-IT02/soal_4/chiho";
static const char *ext_starter   = ".mai";
static const char *ext_metro     = ".ccc";
static const char *ext_skystreet = ".gz";
static const char *aes_key       = "00112233445566778899AABBCCDDEEFF00112233445566778899AABBCCDDEEFF";
static const char *aes_iv        = "0102030405060708090A0B0C0D0E0F10";

// Helper: build backend path (with extension)
static void backend_path(char out[1024], const char *path) {
    out[0] = '\0';
    if (!strcmp(path, "/")) return;
    const char *p = path+1;
    const char *s = strchr(p,'/');
    char area[32]={0}, *file=NULL;
    if (s) {
        size_t al = s-p; if (al>=sizeof(area)) al=sizeof(area)-1;
        memcpy(area,p,al); area[al]='\0';
        file = (char*)(s+1);
    } else strncpy(area,p,sizeof(area)-1);
    snprintf(out,1024,"%s/%s", base_dir, area);
    if (!file) return;
    size_t L=strlen(out);
    snprintf(out+L,1024-L,"/%s",file);
    const char *ext=NULL;
    if (!strcmp(area,"starter"))   ext=ext_starter;
    else if (!strcmp(area,"metro"))     ext=ext_metro;
    else if (!strcmp(area,"skystreet")) ext=ext_skystreet;
    if (ext) { L=strlen(out); snprintf(out+L,1024-L,"%s",ext); }
}

// Helper for Prism 7sref
// parse "/7sref/area_file.ext..." → area, file
static int parse_7sref(const char *path, char *area, char *file) {
    const char *p = path + strlen("/7sref/");
    char *u = strchr(p,'_');
    if (!u) return -1;
    size_t al = u-p; if (al>=32) return -1;
    memcpy(area,p,al); area[al]='\0';
    strcpy(file, u+1);
    return 0;
}

// Transforms
static void shift_buf(char*b,size_t n,int f){for(size_t i=0;i<n;i++)b[i]=(unsigned char)(b[i]+(f?(i%256):-(i%256)));} 
static void rot13_buf(char*b,size_t n){for(size_t i=0;i<n;i++){unsigned char c=b[i];if(c>='a'&&c<='z')b[i]='a'+(c-'a'+13)%26;else if(c>='A'&&c<='Z')b[i]='A'+(c-'A'+13)%26;}}
static int run_openssl(const char*cmd){return system(cmd)==0?0:-EIO;}

// ========== getattr ==========
static int maimai_getattr(const char *path, struct stat *st) {
    memset(st,0,sizeof(*st));
    if (!strcmp(path,"/")) { st->st_mode=S_IFDIR|0755; return 0; }
    // Prism root
    if (!strncmp(path,"/7sref",6)) {
        if (!strcmp(path,"/7sref")) { st->st_mode=S_IFDIR|0755; return 0; }
        char area[32], file[256];
        if (parse_7sref(path,area,file)==0) {
            char proxy[1024]; snprintf(proxy,sizeof(proxy),"/%s/%s",area,file);
            return maimai_getattr(proxy,st);
        }
    }
    // main areas
    if (!strncmp(path,"/starter",8)||!strncmp(path,"/metro",6)||
        !strncmp(path,"/dragon",7)||!strncmp(path,"/blackrose",10)||
        !strncmp(path,"/heaven",7)||!strncmp(path,"/skystreet",10))
    {
        if (!strchr(path+1,'/')){ st->st_mode=S_IFDIR|0755; return 0; }
    }
    // file
    char bp[1024]; backend_path(bp,path);
    int r=lstat(bp,st);
    return r<0?-errno:0;
}

// ========== readdir ==========
static int maimai_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t off, struct fuse_file_info *fi)
{
    (void)off; (void)fi;
    if (!strcmp(path,"/")) {
        filler(buf,".",NULL,0); filler(buf,"..",NULL,0);
        filler(buf,"starter",NULL,0);
        filler(buf,"metro",NULL,0);
        filler(buf,"dragon",NULL,0);
        filler(buf,"blackrose",NULL,0);
        filler(buf,"heaven",NULL,0);
        filler(buf,"skystreet",NULL,0);
        filler(buf,"7sref",NULL,0);
        return 0;
    }
    if (!strcmp(path,"/7sref")) {
        filler(buf,".",NULL,0); filler(buf,"..",NULL,0);
        const char *areas[]={"starter","metro","dragon","blackrose","heaven","skystreet"};
        for (int i=0;i<6;i++){
            char dirp[1024]; snprintf(dirp,sizeof(dirp),"%s/%s",base_dir,areas[i]);
            DIR *d=opendir(dirp); if (!d) continue;
            struct dirent *e;
            while ((e=readdir(d))) {
                if (e->d_name[0]=='.') continue;
                // strip ext
                char name[256]; strcpy(name,e->d_name);
                const char *ext = !strcmp(areas[i],"starter") ? ext_starter
                                 : !strcmp(areas[i],"metro")   ? ext_metro
                                 : !strcmp(areas[i],"skystreet")? ext_skystreet
                                 : NULL;
                if (ext) {
                    size_t ln=strlen(name), el=strlen(ext);
                    if (ln>el && !strcmp(name+ln-el,ext)) name[ln-el]='\0';
                }
                char entry[300];
                snprintf(entry,sizeof(entry),"%s_%s",areas[i],name);
                filler(buf,entry,NULL,0);
            }
            closedir(d);
        }
        return 0;
    }
    // normal area
    const char *area=NULL;
    if (!strcmp(path,"/starter"))   area="starter";
    else if (!strcmp(path,"/metro"))     area="metro";
    else if (!strcmp(path,"/dragon"))    area="dragon";
    else if (!strcmp(path,"/blackrose")) area="blackrose";
    else if (!strcmp(path,"/heaven"))    area="heaven";
    else if (!strcmp(path,"/skystreet")) area="skystreet";
    if (!area) return -ENOENT;
    char dirp[1024]; snprintf(dirp,sizeof(dirp),"%s/%s",base_dir,area);
    DIR *d=opendir(dirp); if(!d) return -errno;
    filler(buf,".",NULL,0); filler(buf,"..",NULL,0);
    struct dirent *e;
    size_t el=0; const char *ext=NULL;
    if (!strcmp(area,"starter"))   ext=ext_starter;
    else if (!strcmp(area,"metro"))     ext=ext_metro;
    else if (!strcmp(area,"skystreet")) ext=ext_skystreet;
    if (ext) el=strlen(ext);
    while((e=readdir(d))){
        size_t ln=strlen(e->d_name);
        if (!strcmp(area,"dragon")||!strcmp(area,"blackrose")||!strcmp(area,"heaven")) {
            filler(buf,e->d_name,NULL,0);
        } else if (ln>el && !strcmp(e->d_name+ln-el,ext)) {
            char name[256]; memcpy(name,e->d_name,ln-el); name[ln-el]='\0';
            filler(buf,name,NULL,0);
        }
    }
    closedir(d);
    return 0;
}

// ========== create / open ==========
static int maimai_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    // Prism proxy
    if (!strncmp(path,"/7sref/",7)) {
        char area[32], file[256], proxy[1024];
        if (parse_7sref(path,area,file)==0) {
            snprintf(proxy,sizeof(proxy),"/%s/%s",area,file);
            return maimai_create(proxy,mode,fi);
        }
    }
    char bp[1024]; backend_path(bp,path);
    int fd=open(bp,fi->flags,mode);
    if(fd<0)return-errno;
    fi->fh=fd;
    return 0;
}
static int maimai_open(const char *path, struct fuse_file_info *fi) {
    if (!strncmp(path,"/7sref/",7)) {
        char area[32], file[256], proxy[1024];
        if (parse_7sref(path,area,file)==0) {
            snprintf(proxy,sizeof(proxy),"/%s/%s",area,file);
            return maimai_open(proxy,fi);
        }
    }
    char bp[1024]; backend_path(bp,path);
    int fd=open(bp,fi->flags);
    if(fd<0)return-errno;
    fi->fh=fd;
    return 0;
}

// ========== read ==========
static int maimai_read(const char *path, char *buf, size_t size, off_t off,
                       struct fuse_file_info *fi)
{
    (void)fi;
    // Prism proxy
    if (!strncmp(path,"/7sref/",7)) {
        char area[32], file[256], proxy[1024];
        if (parse_7sref(path,area,file)==0) {
            snprintf(proxy,sizeof(proxy),"/%s/%s",area,file);
            return maimai_read(proxy,buf,size,off,fi);
        }
    }
    // Heaven decrypt
    if (!strncmp(path,"/heaven/",8)) {
        char bp[1024],tmpf[64],cmd[2048];
        backend_path(bp,path);
        snprintf(tmpf,sizeof(tmpf),"/tmp/heaven_%d.dec",getpid());
        snprintf(cmd,sizeof(cmd),
          "openssl enc -d -aes-256-cbc -K %s -iv %s -in '%s' -out '%s' 2>/dev/null",
          aes_key,aes_iv,bp,tmpf);
        if(run_openssl(cmd)<0) return -EIO;
        int fd=open(tmpf,O_RDONLY);
        if(fd<0){unlink(tmpf);return-errno;}
        int r=pread(fd,buf,size,off);
        close(fd);unlink(tmpf);
        return r<0?-errno:r;
    }
    // Skystreet gzip
    if (!strncmp(path,"/skystreet/",11)) {
        char bp[1024]; backend_path(bp,path);
        gzFile gz=gzopen(bp,"rb");
        if(!gz)return-errno;
        if(gzseek(gz,off,SEEK_SET)<0){gzclose(gz);return 0;}
        int r=gzread(gz,buf,size);
        gzclose(gz);
        return r<0?-EIO:r;
    }
    // default read
    char bp[1024]; backend_path(bp,path);
    int fd=open(bp,O_RDONLY);
    if(fd<0)return-errno;
    int r=pread(fd,buf,size,off);
    close(fd);
    if(r<0)return-errno;
    if(!strncmp(path,"/metro/",7))    shift_buf(buf,r,0);
    else if(!strncmp(path,"/dragon/",8))rot13_buf(buf,r);
    return r;
}

// ========== write ==========
static int maimai_write(const char *path, const char *buf, size_t size,
                        off_t off, struct fuse_file_info *fi)
{
    // Prism proxy
    if (!strncmp(path,"/7sref/",7)) {
        char area[32], file[256], proxy[1024];
        if (parse_7sref(path,area,file)==0) {
            snprintf(proxy,sizeof(proxy),"/%s/%s",area,file);
            return maimai_write(proxy,buf,size,off,fi);
        }
    }
    // Heaven encrypt
    if (!strncmp(path,"/heaven/",8)) {
        char bp[1024],tmpf[64],cmd[2048];
        backend_path(bp,path);
        snprintf(tmpf,sizeof(tmpf),"/tmp/heaven_%d.enc",getpid());
        int fd=open(tmpf,O_WRONLY|O_CREAT|O_TRUNC,0600);
        if(fd<0)return-errno;
        if(pwrite(fd,buf,size,off)<0){close(fd);unlink(tmpf);return-errno;}
        close(fd);
        snprintf(cmd,sizeof(cmd),
          "openssl enc -aes-256-cbc -K %s -iv %s -in '%s' -out '%s' 2>/dev/null",
          aes_key,aes_iv,tmpf,bp);
        int e=run_openssl(cmd);
        unlink(tmpf);
        return e? -EIO : size;
    }
    // Skystreet gzip
    if (!strncmp(path,"/skystreet/",11)) {
        char bp[1024]; backend_path(bp,path);
        gzFile gz=gzopen(bp,"wb");
        if(!gz) return-errno;
        int w=gzwrite(gz,buf,size);
        gzclose(gz);
        return w<0? -EIO : size;
    }
    // default write
    char bp[1024]; backend_path(bp,path);
    char tmpb[size]; memcpy(tmpb,buf,size);
    if(!strncmp(path,"/metro/",7))    shift_buf(tmpb,size,1);
    else if(!strncmp(path,"/dragon/",8))rot13_buf(tmpb,size);
    int fd=open(bp,O_WRONLY|O_CREAT,0644);
    if(fd<0)return-errno;
    int w=pwrite(fd,tmpb,size,off);
    close(fd);
    return w<0?-errno:w;
}

// ========== truncate ==========
static int maimai_truncate(const char *path, off_t size) {
    // Prism proxy
    if (!strncmp(path,"/7sref/",7)) {
        char area[32], file[256], proxy[1024];
        if (parse_7sref(path,area,file)==0) {
            snprintf(proxy,sizeof(proxy),"/%s/%s",area,file);
            return maimai_truncate(proxy,size);
        }
    }
    char bp[1024]; backend_path(bp,path);
    int r=truncate(bp,size);
    return r<0?-errno:0;
}

// ========== unlink ==========
static int maimai_unlink(const char *path) {
    // Prism proxy
    if (!strncmp(path,"/7sref/",7)) {
        char area[32], file[256], proxy[1024];
        if (parse_7sref(path,area,file)==0) {
            snprintf(proxy,sizeof(proxy),"/%s/%s",area,file);
            return maimai_unlink(proxy);
        }
    }
    char bp[1024]; backend_path(bp,path);
    int r=unlink(bp);
    return r<0?-errno:0;
}

static struct fuse_operations ops = {
    .getattr  = maimai_getattr,
    .readdir  = maimai_readdir,
    .create   = maimai_create,
    .open     = maimai_open,
    .read     = maimai_read,
    .write    = maimai_write,
    .truncate = maimai_truncate,
    .unlink   = maimai_unlink,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &ops, NULL);
}
