# Panduan Ringkas `maimai_fs.c` (Area a–g)

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







