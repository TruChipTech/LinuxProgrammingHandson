/*
 * fuse_fat.c — FUSE FAT16 Filesystem Driver (read-only)
 *
 * Build: gcc -Wall -g fuse_fat.c -o fuse_fat $(pkg-config --cflags --libs fuse3)
 * Usage: ./fuse_fat <fat16_image> <mountpoint>
 * Unmount: fusermount3 -u <mountpoint>
 */

#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t  jmp[3];
    char     oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_sig;
    uint32_t volume_serial;
    char     volume_label[11];
    char     fs_type[8];
} fat16_bpb_t;

typedef struct {
    char     name[8];
    char     ext[3];
    uint8_t  attr;
    uint8_t  nt_reserved;
    uint8_t  create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t first_cluster_hi;
    uint16_t modify_time;
    uint16_t modify_date;
    uint16_t first_cluster;
    uint32_t file_size;
} fat_dir_entry_t;
#pragma pack(pop)

#define ATTR_DIRECTORY  0x10
#define ATTR_VOLUME_ID  0x08
#define ATTR_LONG_NAME  0x0F

/* Global state */
static FILE *disk_fp = NULL;
static fat16_bpb_t bpb;
static uint32_t fat_start;
static uint32_t root_dir_start;
static uint32_t data_start;
static uint32_t cluster_size;

/* Get next cluster from FAT */
static uint16_t fat_next_cluster(uint16_t cluster) {
    fseek(disk_fp, fat_start + cluster * 2, SEEK_SET);
    uint16_t next;
    if (fread(&next, 2, 1, disk_fp) != 1) return 0xFFFF;
    return next;
}

/* Convert cluster number to byte offset */
static uint32_t cluster_to_offset(uint16_t cluster) {
    return data_start + (cluster - 2) * cluster_size;
}

/* Format 8.3 filename */
static void format_name(const fat_dir_entry_t *e, char *out) {
    int pos = 0;
    for (int i = 0; i < 8 && e->name[i] != ' '; i++)
        out[pos++] = e->name[i];
    if (e->ext[0] != ' ') {
        out[pos++] = '.';
        for (int i = 0; i < 3 && e->ext[i] != ' '; i++)
            out[pos++] = e->ext[i];
    }
    out[pos] = '\0';
}

/* Find a directory entry by path */
static int find_entry(const char *path, fat_dir_entry_t *result) {
    if (strcmp(path, "/") == 0) {
        memset(result, 0, sizeof(*result));
        result->attr = ATTR_DIRECTORY;
        return 0;
    }

    /* Parse path components */
    char pathcopy[512];
    strncpy(pathcopy, path + 1, sizeof(pathcopy) - 1);  /* Skip leading / */
    pathcopy[sizeof(pathcopy) - 1] = '\0';

    /* Start from root directory */
    uint32_t dir_offset = root_dir_start;
    int max_entries = bpb.root_entry_count;
    int is_root = 1;

    char *component = strtok(pathcopy, "/");
    while (component) {
        char *next_component = strtok(NULL, "/");

        /* Convert component to uppercase for comparison */
        char upper[13];
        int i;
        for (i = 0; component[i] && i < 12; i++)
            upper[i] = (component[i] >= 'a' && component[i] <= 'z')
                       ? component[i] - 32 : component[i];
        upper[i] = '\0';

        /* Search directory */
        fseek(disk_fp, dir_offset, SEEK_SET);
        int found = 0;
        for (int j = 0; j < max_entries; j++) {
            fat_dir_entry_t entry;
            if (fread(&entry, sizeof(entry), 1, disk_fp) != 1) break;
            if ((uint8_t)entry.name[0] == 0x00) break;
            if ((uint8_t)entry.name[0] == 0xE5) continue;
            if (entry.attr == ATTR_LONG_NAME) continue;
            if (entry.attr & ATTR_VOLUME_ID) continue;

            char fname[13];
            format_name(&entry, fname);

            if (strcasecmp(fname, component) == 0) {
                if (next_component == NULL) {
                    *result = entry;
                    return 0;
                }
                if (!(entry.attr & ATTR_DIRECTORY)) return -ENOTDIR;

                dir_offset = cluster_to_offset(entry.first_cluster);
                max_entries = cluster_size / 32;
                is_root = 0;
                found = 1;
                break;
            }
        }
        if (!found) return -ENOENT;
        component = next_component;
    }

    return -ENOENT;
}

/* FUSE: getattr */
static int fusefat_getattr(const char *path, struct stat *st,
                           struct fuse_file_info *fi) {
    (void)fi;
    memset(st, 0, sizeof(*st));

    fat_dir_entry_t entry;
    int ret = find_entry(path, &entry);
    if (ret < 0) return ret;

    if (entry.attr & ATTR_DIRECTORY) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else {
        st->st_mode = S_IFREG | 0444;
        st->st_nlink = 1;
        st->st_size = entry.file_size;
    }
    st->st_uid = getuid();
    st->st_gid = getgid();
    return 0;
}

/* FUSE: readdir */
static int fusefat_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t offset, struct fuse_file_info *fi,
                           enum fuse_readdir_flags flags) {
    (void)offset; (void)fi; (void)flags;

    fat_dir_entry_t dir_entry;
    int ret = find_entry(path, &dir_entry);
    if (ret < 0) return ret;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    uint32_t dir_offset;
    int max_entries;

    if (strcmp(path, "/") == 0) {
        dir_offset = root_dir_start;
        max_entries = bpb.root_entry_count;
    } else {
        dir_offset = cluster_to_offset(dir_entry.first_cluster);
        max_entries = cluster_size / 32;
    }

    fseek(disk_fp, dir_offset, SEEK_SET);
    for (int i = 0; i < max_entries; i++) {
        fat_dir_entry_t entry;
        if (fread(&entry, sizeof(entry), 1, disk_fp) != 1) break;
        if ((uint8_t)entry.name[0] == 0x00) break;
        if ((uint8_t)entry.name[0] == 0xE5) continue;
        if (entry.attr == ATTR_LONG_NAME) continue;
        if (entry.attr & ATTR_VOLUME_ID) continue;

        char fname[13];
        format_name(&entry, fname);
        filler(buf, fname, NULL, 0, 0);
    }
    return 0;
}

/* FUSE: read */
static int fusefat_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {
    (void)fi;

    fat_dir_entry_t entry;
    int ret = find_entry(path, &entry);
    if (ret < 0) return ret;
    if (entry.attr & ATTR_DIRECTORY) return -EISDIR;

    if (offset >= entry.file_size) return 0;
    if (offset + size > entry.file_size) size = entry.file_size - offset;

    /* Follow cluster chain */
    uint16_t cluster = entry.first_cluster;
    size_t cluster_skip = offset / cluster_size;
    size_t intra_offset = offset % cluster_size;

    /* Skip to the right cluster */
    for (size_t i = 0; i < cluster_skip && cluster < 0xFFF8; i++) {
        cluster = fat_next_cluster(cluster);
    }

    size_t bytes_read = 0;
    while (bytes_read < size && cluster >= 2 && cluster < 0xFFF8) {
        uint32_t coff = cluster_to_offset(cluster) + intra_offset;
        size_t to_read = cluster_size - intra_offset;
        if (to_read > size - bytes_read) to_read = size - bytes_read;

        fseek(disk_fp, coff, SEEK_SET);
        size_t n = fread(buf + bytes_read, 1, to_read, disk_fp);
        bytes_read += n;

        intra_offset = 0;
        cluster = fat_next_cluster(cluster);
    }

    return bytes_read;
}

static int fusefat_open(const char *path, struct fuse_file_info *fi) {
    (void)fi;
    fat_dir_entry_t entry;
    return find_entry(path, &entry);
}

static const struct fuse_operations fusefat_ops = {
    .getattr = fusefat_getattr,
    .readdir = fusefat_readdir,
    .open    = fusefat_open,
    .read    = fusefat_read,
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <fat16_image> <mountpoint> [fuse_options]\n",
                argv[0]);
        return 1;
    }

    /* Open disk image */
    disk_fp = fopen(argv[1], "rb");
    if (!disk_fp) { perror("fopen"); return 1; }

    /* Read BPB */
    if (fread(&bpb, sizeof(bpb), 1, disk_fp) != 1) {
        fprintf(stderr, "Failed to read BPB\n");
        return 1;
    }

    /* Calculate layout */
    fat_start = bpb.reserved_sectors * bpb.bytes_per_sector;
    root_dir_start = fat_start + bpb.num_fats * bpb.fat_size_16 *
                     bpb.bytes_per_sector;
    uint32_t root_dir_size = bpb.root_entry_count * 32;
    data_start = root_dir_start + root_dir_size;
    cluster_size = bpb.sectors_per_cluster * bpb.bytes_per_sector;

    fprintf(stderr, "FAT16: %.11s, %u bytes/cluster, mounting...\n",
            bpb.volume_label, cluster_size);

    /* Shift argv to remove the image path for FUSE */
    argv[1] = argv[2];
    argc--;
    for (int i = 2; i < argc; i++) argv[i] = argv[i + 1];

    int ret = fuse_main(argc, argv, &fusefat_ops, NULL);
    fclose(disk_fp);
    return ret;
}
