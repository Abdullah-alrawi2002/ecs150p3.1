/* fs.c */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "disk.h"
#include "fs.h"

#define FS_SIGNATURE     "ECS150FS"
#define FS_SIG_LEN       8

/* Superblock structure (packed to exactly BLOCK_SIZE bytes) */
struct __attribute__((packed)) superblock {
    char     signature[FS_SIG_LEN];    /* "ECS150FS" */
    uint16_t total_blocks;             /* total blocks of virtual disk */
    uint16_t root_dir_index;           /* block index of root directory */
    uint16_t data_block_start;         /* index of first data block */
    uint16_t data_block_count;         /* number of data blocks */
    uint8_t  fat_block_count;          /* number of FAT blocks */
    uint8_t  unused[BLOCK_SIZE - FS_SIG_LEN - 2 - 2 - 2 - 2 - 1];
};

/* Directory entry structure (packed) */
struct __attribute__((packed)) dir_entry {
    char     filename[FS_FILENAME_LEN];
    uint32_t size;                     /* file size in bytes */
    uint16_t first_block;              /* first data block index */
    uint8_t  unused[10];
};

static struct superblock sb;
static uint16_t *fat = NULL;
static struct dir_entry *root_dir = NULL;
static int mounted = 0;

int fs_mount(const char *diskname) {
    if (mounted) {
        return -1;
    }
    if (block_disk_open(diskname) != 0) {
        return -1;
    }
    /* Read the superblock */
    if (block_read(0, &sb) != 0) {
        block_disk_close();
        return -1;
    }
    /* Validate signature */
    if (memcmp(sb.signature, FS_SIGNATURE, FS_SIG_LEN) != 0) {
        block_disk_close();
        return -1;
    }
    /* Validate total block count */
    int disk_count = block_disk_count();
    if (disk_count < 0 || (uint16_t)disk_count != sb.total_blocks) {
        block_disk_close();
        return -1;
    }
    /* Load FAT */
    size_t fat_bytes = sb.fat_block_count * BLOCK_SIZE;
    fat = malloc(fat_bytes);
    if (!fat) {
        block_disk_close();
        return -1;
    }
    for (uint8_t i = 0; i < sb.fat_block_count; i++) {
        if (block_read(1 + i, ((uint8_t*)fat) + i * BLOCK_SIZE) != 0) {
            free(fat);
            block_disk_close();
            return -1;
        }
    }
    /* Load root directory */
    root_dir = malloc(BLOCK_SIZE);
    if (!root_dir) {
        free(fat);
        block_disk_close();
        return -1;
    }
    if (block_read(sb.root_dir_index, root_dir) != 0) {
        free(fat);
        free(root_dir);
        block_disk_close();
        return -1;
    }
    mounted = 1;
    return 0;
}

int fs_umount(void) {
    if (!mounted) {
        return -1;
    }
    free(fat);
    fat = NULL;
    free(root_dir);
    root_dir = NULL;
    if (block_disk_close() != 0) {
        return -1;
    }
    mounted = 0;
    return 0;
}

int fs_info(void) {
    if (!mounted) {
        return -1;
    }
    printf("FS Info:\n");
    printf("total_blk_count=%u\n", sb.total_blocks);
    printf("fat_blk_count=%u\n", sb.fat_block_count);
    printf("rdir_blk=%u\n", sb.root_dir_index);
    printf("data_blk=%u\n", sb.data_block_start);
    printf("data_blk_count=%u\n", sb.data_block_count);
    /* Count free FAT entries */
    uint32_t free_fat = 0;
    for (uint32_t i = 0; i < sb.data_block_count; i++) {
        if (fat[i] == 0) free_fat++;
    }
    printf("fat_free_ratio=%u/%u\n", free_fat, sb.data_block_count);
    /* Count free root directory entries */
    uint32_t free_rdir = 0;
    for (uint32_t i = 0; i < FS_FILE_MAX_COUNT; i++) {
        if (root_dir[i].filename[0] == '\0') free_rdir++;
    }
    printf("rdir_free_ratio=%u/%u\n", free_rdir, FS_FILE_MAX_COUNT);
    return 0;
}
