/*
 * fat_parser.c — FAT16 Filesystem Image Parser
 *
 * Build: gcc -Wall -g fat_parser.c -o fat_parser
 * Usage: ./fat_parser <fat16_image>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#pragma pack(push, 1)

/* BIOS Parameter Block (BPB) — at offset 0 of boot sector */
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
    /* FAT16 extended */
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_sig;
    uint32_t volume_serial;
    char     volume_label[11];
    char     fs_type[8];
} fat16_bpb_t;

/* Directory Entry — 32 bytes */
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

/* Attribute flags */
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20
#define ATTR_LONG_NAME  0x0F

static void print_bpb(const fat16_bpb_t *bpb) {
    printf("=== FAT16 Boot Sector / BPB ===\n\n");
    printf("  OEM Name:            %.8s\n", bpb->oem_name);
    printf("  Bytes/Sector:        %u\n", bpb->bytes_per_sector);
    printf("  Sectors/Cluster:     %u\n", bpb->sectors_per_cluster);
    printf("  Reserved Sectors:    %u\n", bpb->reserved_sectors);
    printf("  Number of FATs:      %u\n", bpb->num_fats);
    printf("  Root Entry Count:    %u\n", bpb->root_entry_count);
    printf("  Total Sectors (16):  %u\n", bpb->total_sectors_16);
    printf("  Media Type:          0x%02X\n", bpb->media_type);
    printf("  FAT Size (sectors):  %u\n", bpb->fat_size_16);
    printf("  Volume Label:        %.11s\n", bpb->volume_label);
    printf("  FS Type:             %.8s\n", bpb->fs_type);
    printf("  Volume Serial:       %08X\n", bpb->volume_serial);

    uint32_t total = bpb->total_sectors_16 ? bpb->total_sectors_16 : bpb->total_sectors_32;
    printf("  Total Size:          %u KB\n",
           total * bpb->bytes_per_sector / 1024);
}

static void print_attr(uint8_t attr) {
    printf("%c%c%c%c%c%c",
           (attr & ATTR_READ_ONLY) ? 'R' : '-',
           (attr & ATTR_HIDDEN)    ? 'H' : '-',
           (attr & ATTR_SYSTEM)    ? 'S' : '-',
           (attr & ATTR_VOLUME_ID) ? 'V' : '-',
           (attr & ATTR_DIRECTORY) ? 'D' : '-',
           (attr & ATTR_ARCHIVE)   ? 'A' : '-');
}

static void print_directory(FILE *fp, const fat16_bpb_t *bpb,
                            uint32_t dir_offset, int max_entries,
                            const char *prefix) {
    fseek(fp, dir_offset, SEEK_SET);

    printf("\n  %-14s %-6s %-8s %s\n", "Filename", "Attr", "Size", "Cluster");
    printf("  %-14s %-6s %-8s %s\n", "--------", "----", "----", "-------");

    for (int i = 0; i < max_entries; i++) {
        fat_dir_entry_t entry;
        if (fread(&entry, sizeof(entry), 1, fp) != 1) break;

        /* End of directory */
        if ((uint8_t)entry.name[0] == 0x00) break;
        /* Deleted entry */
        if ((uint8_t)entry.name[0] == 0xE5) continue;
        /* Long filename entry */
        if (entry.attr == ATTR_LONG_NAME) continue;

        /* Format 8.3 name */
        char fname[13];
        int pos = 0;
        for (int j = 0; j < 8 && entry.name[j] != ' '; j++)
            fname[pos++] = entry.name[j];
        if (entry.ext[0] != ' ') {
            fname[pos++] = '.';
            for (int j = 0; j < 3 && entry.ext[j] != ' '; j++)
                fname[pos++] = entry.ext[j];
        }
        fname[pos] = '\0';

        printf("  %s%-14s ", prefix, fname);
        print_attr(entry.attr);
        printf(" %-8u %u\n", entry.file_size, entry.first_cluster);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <fat16_image>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) { perror("fopen"); return 1; }

    /* Read BPB */
    fat16_bpb_t bpb;
    if (fread(&bpb, sizeof(bpb), 1, fp) != 1) {
        fprintf(stderr, "Failed to read BPB\n");
        fclose(fp);
        return 1;
    }

    print_bpb(&bpb);

    /* Calculate layout */
    uint32_t fat_start = bpb.reserved_sectors * bpb.bytes_per_sector;
    uint32_t root_dir_start = fat_start + bpb.num_fats * bpb.fat_size_16 *
                              bpb.bytes_per_sector;
    uint32_t root_dir_size = bpb.root_entry_count * 32;
    uint32_t data_start = root_dir_start + root_dir_size;

    printf("\n=== Filesystem Layout ===\n");
    printf("  FAT starts at:       byte %u\n", fat_start);
    printf("  Root dir starts at:  byte %u\n", root_dir_start);
    printf("  Data region at:      byte %u\n", data_start);

    /* Print FAT entries (first 32) */
    printf("\n=== FAT Table (first 32 entries) ===\n  ");
    fseek(fp, fat_start, SEEK_SET);
    for (int i = 0; i < 32; i++) {
        uint16_t entry;
        if (fread(&entry, 2, 1, fp) != 1) break;
        printf("%04X ", entry);
        if ((i + 1) % 16 == 0) printf("\n  ");
    }
    printf("\n");

    /* Print root directory */
    printf("\n=== Root Directory ===\n");
    print_directory(fp, &bpb, root_dir_start, bpb.root_entry_count, "");

    /* Extract a file (first regular file found) */
    fseek(fp, root_dir_start, SEEK_SET);
    for (int i = 0; i < bpb.root_entry_count; i++) {
        fat_dir_entry_t entry;
        if (fread(&entry, sizeof(entry), 1, fp) != 1) break;
        if ((uint8_t)entry.name[0] == 0x00) break;
        if ((uint8_t)entry.name[0] == 0xE5) continue;
        if (entry.attr & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) continue;
        if (entry.file_size == 0) continue;

        /* Found a file — show its content */
        char fname[13];
        int pos = 0;
        for (int j = 0; j < 8 && entry.name[j] != ' '; j++)
            fname[pos++] = entry.name[j];
        if (entry.ext[0] != ' ') {
            fname[pos++] = '.';
            for (int j = 0; j < 3 && entry.ext[j] != ' '; j++)
                fname[pos++] = entry.ext[j];
        }
        fname[pos] = '\0';

        printf("\n=== File Content: %s (%u bytes) ===\n", fname, entry.file_size);

        uint32_t file_offset = data_start +
            (entry.first_cluster - 2) * bpb.sectors_per_cluster * bpb.bytes_per_sector;
        fseek(fp, file_offset, SEEK_SET);

        size_t to_read = entry.file_size < 512 ? entry.file_size : 512;
        char *content = malloc(to_read + 1);
        fread(content, 1, to_read, fp);
        content[to_read] = '\0';
        printf("%s\n", content);
        free(content);
        break;
    }

    fclose(fp);
    printf("\n=== Parser Complete ===\n");
    return 0;
}
