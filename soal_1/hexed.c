#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#define MAX_HEX_SIZE 2000000

int hexCharToInt(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    return -1;
}

int isValidHexChar(char c) {
    return isxdigit(c);
}

int hexStringToBytes(const char *hex, unsigned char *bytes, int hexLen) {
    if (hexLen % 2 != 0) return -1;
    for (int i = 0; i < hexLen; i += 2) {
        int high = hexCharToInt(hex[i]);
        int low = hexCharToInt(hex[i+1]);
        if (high == -1 || low == -1) return -1;
        bytes[i/2] = (high << 4) | low;
    }
    return hexLen / 2;
}

void getTimestamp(char *date, size_t dateSize, char *timeStr, size_t timeSize) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(date, dateSize, "%Y-%m-%d", t);
    strftime(timeStr, timeSize, "%H:%M:%S", t);
}

int main() {
    DIR *folder;
    struct dirent *entry;

    const char *inputDir = "anomali";
    const char *outputDir = "image";
    mkdir(outputDir, 0755);

    FILE *log = fopen("conversion.log", "a");
    if (!log) {
        perror("Gagal membuka conversion.log");
        return 1;
    }

    folder = opendir(inputDir);
    if (!folder) {
        perror("Gagal membuka folder input");
        return 1;
    }

    while ((entry = readdir(folder)) != NULL) {
        if (entry->d_type != DT_REG || !strstr(entry->d_name, ".txt")) continue;

        char inputPath[256];
        snprintf(inputPath, sizeof(inputPath), "%s/%s", inputDir, entry->d_name);

        FILE *in = fopen(inputPath, "r");
        if (!in) {
            perror("Gagal membuka file");
            continue;
        }

        char *hex = malloc(MAX_HEX_SIZE);
        if (!hex) {
            fprintf(stderr, "Gagal alokasi hex\n");
            fclose(in);
            continue;
        }

        int hexLen = 0;
        int ch;
        while ((ch = fgetc(in)) != EOF && hexLen < MAX_HEX_SIZE - 1) {
            if (isValidHexChar(ch)) {
                hex[hexLen++] = ch;
            }
        }
        fclose(in);

        if (hexLen % 2 != 0) {
        // Abaikan karakter terakhir agar bisa dikonversi
         hexLen--;
        }


        unsigned char *bytes = malloc(hexLen / 2);
        if (!bytes) {
            fprintf(stderr, "Gagal alokasi bytes\n");
            free(hex);
            continue;
        }

        int byteLen = hexStringToBytes(hex, bytes, hexLen);
        if (byteLen < 0) {
            fprintf(stderr, "Hex tidak valid di file: %s\n", entry->d_name);
            free(hex);
            free(bytes);
            continue;
        }

        // Nama output
        char baseName[128];
        strncpy(baseName, entry->d_name, sizeof(baseName));
        char *dot = strrchr(baseName, '.');
        if (dot) *dot = '\0';

        char date[16], timeStr[16];
        getTimestamp(date, sizeof(date), timeStr, sizeof(timeStr));

        char outputFileName[256];
        snprintf(outputFileName, sizeof(outputFileName), "%s/%s_image_%s_%s.png", outputDir, baseName, date, timeStr);

        FILE *out = fopen(outputFileName, "wb");
        if (!out) {
            perror("Gagal membuat file output");
            free(hex);
            free(bytes);
            continue;
        }

        fwrite(bytes, 1, byteLen, out);
        fclose(out);

        fprintf(log, "[%s][%s]: Successfully converted hexadecimal text %s to %s.\n",
                date, timeStr, entry->d_name, outputFileName);

        printf("✅ %s -> %s\n", entry->d_name, outputFileName);

        free(hex);
        free(bytes);
    }

    closedir(folder);
    fclose(log);
    return 0;
}
