#include <stdlib.h>
#include <string.h>
#include "fs.h"

/*
 * 0 for superblock,
 * 1 for inode bitmap,
 * 2 for block bitmap,
 * 3 for inode,
 * 4 for first dir
 * */

struct fs_blkdev *g_bdev = NULL;
struct inode *g_root = NULL;

#define BITS_PER_WORD 32
#define BLOCK_BITMAP_COUNT (FS_BLOCK_COUNT / BITS_PER_WORD)
#define INODE_BITMAP_COUNT (FS_BLOCK_SIZE / (sizeof(struct dinode)))
#define DINODE_COUNT (FS_BLOCK_SIZE / (sizeof(struct dinode)))
static uint32_t BlockBitmap[BLOCK_BITMAP_COUNT];
static uint32_t InodeBitmap[(INODE_BITMAP_COUNT - 1) / 32 + 1];
static struct dinode DInodeArray[DINODE_COUNT];

void bitmap_init(uint32_t *bitmap, uint32_t count)
{
    for(uint32_t i = 0; i < count; i++) {
        bitmap[i] = ~0;
    }
}

void bitmap_set_free(uint32_t *bitmap, uint32_t blk)
{
    uint32_t idx = blk / BITS_PER_WORD;
    uint32_t bit = blk % BITS_PER_WORD;
    bitmap[idx] |= (1U << bit);
}

void bitmap_set_used(uint32_t *bitmap, uint32_t blk)
{
    uint32_t idx = blk / BITS_PER_WORD;
    uint32_t bit = blk % BITS_PER_WORD;
    bitmap[idx] &= ~(1U << bit);
}

int bitmap_find_free(const uint32_t *bitmap, uint32_t bit_word_count)
{
    for (uint32_t i = 0; i < bit_word_count; i++) {
        uint32_t word = bitmap[i];
        if (word != 0) {
            for (uint32_t b = 0; b < BITS_PER_WORD; b++) {
                if (word & (1U << b)) {
                    return (int)(i * BITS_PER_WORD + b);
                }
            }
        }
    }
    return -1;
}

int block_alloc(void)
{
    int blk = bitmap_find_free(BlockBitmap, FS_BLOCK_COUNT / BITS_PER_WORD);
    if (blk < 0) return -1;
    bitmap_set_used(BlockBitmap, blk);
    return blk;
}


int block_free(uint32_t blk)
{
    bitmap_set_free(BlockBitmap, blk);
    return 0;
}

int block_write(uint32_t blk, uint32_t off, const void *buf, uint32_t size)
{
    return g_bdev->write(g_bdev->ctx, blk, off, buf, size);
}


int inode_alloc(struct inode **out)
{
    int i = bitmap_find_free(InodeBitmap, INODE_BITMAP_COUNT);
    if (i < 0) return -1;
    bitmap_set_used(InodeBitmap, i);

    struct inode *ino = malloc(sizeof(struct inode));
    if (!ino) return -1;
    ino->ino = i;
    ino->refcnt = 1;
    *out = ino;
    return 0;
}

int inode_free(struct inode *inode)
{
    free(inode);
    return 0;
}


int fs_mount(struct superblock *sb, struct fs_blkdev *bdev)
{
    g_bdev = bdev;
    if (g_bdev->read(g_bdev->ctx, 0, 0, sb, sizeof(struct superblock)) != 0)
        return -1;
    if (sb->magic != 0x12345678)
        return -2;
    if (g_bdev->read(g_bdev->ctx, 1, 0, InodeBitmap, sizeof(InodeBitmap)) != 0)
        return -1;
    if (g_bdev->read(g_bdev->ctx, 2, 0, BlockBitmap, sizeof(BlockBitmap)) != 0)
        return -1;
    if (g_bdev->read(g_bdev->ctx, 3, 0, DInodeArray, sizeof(DInodeArray)) != 0)
        return -1;
    g_root = malloc(sizeof(struct inode));
    g_root->ino = 0;
    g_root->refcnt = 1;
    g_root->din = DInodeArray[0];
    return 0;
}

int fs_unmount(struct superblock *sb)
{
    if (!sb || !g_bdev) return -1;
    if (g_root) {
        inode_free(g_root);
        g_root = NULL;
    }
    if (g_bdev->sync) g_bdev->sync(g_bdev->ctx);
    g_bdev = NULL;
    return 0;
}

int fs_format(struct superblock *sb)
{
    uint32_t i;
    bitmap_init(BlockBitmap, BLOCK_BITMAP_COUNT);
    bitmap_init(InodeBitmap, INODE_BITMAP_COUNT);
    sb->magic = 0x12345678;
    sb->block_size = g_bdev->block_size;
    sb->total_blocks = g_bdev->block_count;
    sb->inode_start = 1;
    sb->inode_count = sb->block_size / sizeof(struct inode);
    sb->data_start = sb->inode_start + 1;

    if (g_bdev->erase) {
        for (i = 0; i < 8; i++) {
            g_bdev->erase(g_bdev->ctx, i);
        }
    }
    for (i = 0; i < 4; i++) {
        bitmap_set_used(BlockBitmap, i);
    }

    inode_alloc(&g_root);
    g_root->din.size = sb->block_size;
    g_root->din.direct[0] = block_alloc();
    DInodeArray[g_root->ino] = g_root->din;

    if (g_bdev->write(g_bdev->ctx, 0, 0, sb, sizeof(struct superblock))) {
        return -1;
    }
    if (g_bdev->write(g_bdev->ctx, 1, 0, InodeBitmap, sizeof(InodeBitmap))) {
        return -1;
    }
    if (g_bdev->write(g_bdev->ctx, 2, 0, BlockBitmap, sizeof(BlockBitmap))) {
        return -1;
    }
    if (g_bdev->write(g_bdev->ctx, 3, 0, DInodeArray, sizeof(DInodeArray))) {
        return -1;
    }

    return 0;
}

int fs_sync(void)
{
    if (g_bdev->write(g_bdev->ctx, 1, 0, InodeBitmap, sizeof(InodeBitmap)) != 0)
        return -1;
    if (g_bdev->write(g_bdev->ctx, 2, 0, BlockBitmap, sizeof(BlockBitmap)) != 0)
        return -1;
    if (g_bdev->write(g_bdev->ctx, 3, 0, DInodeArray, sizeof(DInodeArray)) != 0)
        return -1;

    if (g_bdev->sync)
        g_bdev->sync(g_bdev->ctx);

    return 0;
}

#define DIR_COUNT_MAX (FS_BLOCK_SIZE / sizeof(struct dirent))
static struct dirent ents[DIR_COUNT_MAX];
int dir_is_exist(uint32_t blk, char *token, struct dirent *out)
{
    memset(ents, 0, sizeof(ents));
    if (g_bdev->read(g_bdev->ctx, blk, 0, ents, sizeof(ents)) != 0)
        return -1;

    for (uint32_t i = 0; i < DIR_COUNT_MAX; i++) {
        if (ents[i].name[0] == 0xFF)
            continue;

        if (strcmp(ents[i].name, token) == 0) {
            if (out)
                memcpy(out, &ents[i], sizeof(struct dirent));
            return 1;
        }
    }

    return 0;
}


int dir_creat(uint32_t blk, char *token, uint32_t size)
{
    if (g_bdev->write(g_bdev->ctx, blk, 0, token, size) != 0)
        return -1;
    return 0;
}

static struct dirent fuck[DIR_COUNT_MAX];
int dir_add_entry(struct inode *dir, const char *name, uint32_t ino, uint8_t type)
{
    uint32_t blk = dir->din.direct[0];
    int count = DIR_COUNT_MAX;

    if (g_bdev->read(g_bdev->ctx, blk, 0, ents, sizeof(ents)) != 0)
        return -1;

    for (int i = 0; i < count; i++) {
        if (strcmp(ents[i].name, name) == 0)
            return -1;

        if (ents[i].name[0] == 0xFF) {
            strncpy(ents[i].name, name, NAME_MAX);
            ents[i].name[NAME_MAX - 1] = '\0';

            ents[i].ino = ino;
            ents[i].type = type;
            if (g_bdev->erase(g_bdev->ctx, blk) != 0)
                return -1;
            if (g_bdev->write(g_bdev->ctx, blk, 0, ents, sizeof(ents)) != 0)
                return -1;

            return 0;
        }
    }

    return -1;
}



int dir_lookup(struct inode *dir, const char *name, struct dirent *out)
{
    uint32_t blk = dir->din.direct[0];
    int count = FS_BLOCK_SIZE / sizeof(struct dirent);

    if (g_bdev->read(g_bdev->ctx, blk, 0, ents, FS_BLOCK_SIZE) != 0)
        return 0;

    for (int i = 0; i < count; i++) {
        if (ents[i].name[0] == '\0')
            continue;

        if (strcmp(ents[i].name, name) == 0) {
            if (out)
                memcpy(out, &ents[i], sizeof(struct dirent));
            return 1;
        }
    }

    return 0;
}


#define PATH_LEN_MAX  64
static char path_copy[PATH_LEN_MAX];
int fs_mkdir(const char *path, struct inode **ino)
{
    if (path == NULL || path[0] == '/') {
        *ino = g_root;
        return 0;
    }

    memset(path_copy, 0, PATH_LEN_MAX);
    strncpy(path_copy, path, PATH_LEN_MAX);

    char *saveptr;
    char *token = strtok_r(path_copy, "/", &saveptr);

    struct inode *cur = malloc(sizeof(struct inode));
    *cur = *g_root;
    struct inode *parent = NULL;
    struct dirent dir;

    while (token) {
        parent = cur;
        *ino = parent;
        uint32_t data_blk = parent->din.direct[0];

        if (dir_is_exist(data_blk, token, &dir)) {
            cur->din = DInodeArray[dir.ino];
            cur->ino = dir.ino;
            token = strtok_r(NULL, "/", &saveptr);
            *ino = cur;
            continue;
        }

        break;
    }

    if (!token) return 0;

    struct inode *new_inode;
    while (token) {
        if (inode_alloc(&new_inode)) {
            return -1;
        }

        uint32_t blk = block_alloc();
        new_inode->din.direct[0] = blk;
        DInodeArray[new_inode->ino] = new_inode->din;

        if (dir_add_entry(parent, token, new_inode->ino, FILE_TYPE_DIR) < 0)
            return -1;

        parent = new_inode;

        token = strtok_r(NULL, "/", &saveptr);
    }
    *ino = new_inode;
    return 0;
}


int fs_rmdir(struct superblock *sb, const char *path)
{
    (void*)sb;
    (void*)path;
    return 0;
}

static int fs_lookup_path(const char *path, struct inode **out)
{
    if (strcmp(path, "/") == 0) {
        *out = g_root;
        return 0;
    }

    char tmp[PATH_LEN_MAX];
    memset(tmp, 0, sizeof(tmp));
    strncpy(tmp, path, PATH_LEN_MAX);
    tmp[PATH_LEN_MAX - 1] = '\0';

    char *save_ptr;
    char *token = strtok_r(tmp, "/", &save_ptr);

    struct inode *cur = malloc(sizeof(struct inode));
    if (!cur) return -1;
    *cur = *g_root;

    struct dirent de;

    while (token) {
        if (!dir_lookup(cur, token, &de)) {
            free(cur);
            return -1;
        }

        cur->din = DInodeArray[de.ino];
        cur->ino = de.ino;

        token = strtok_r(NULL, "/", &save_ptr);
    }

    *out = cur;
    return 0;
}

int fs_readdir(const char *path, struct dirent *buf, int max, int *nread)
{
    *nread = 0;

    struct inode *dir_ino;
    if (fs_lookup_path(path, &dir_ino) < 0)
        return -1;

    uint32_t blk = dir_ino->din.direct[0];
    if (g_bdev->read(g_bdev->ctx, blk, 0, ents, sizeof(ents)) != 0) {
        free(dir_ino);
        return -1;
    }

    int count = 0;
    for (uint32_t i = 0; ((i < DIR_COUNT_MAX) && (count < max)); i++) {
        if ((uint8_t)ents[i].name[0] == 0xFF)
            continue;

        buf[count++] = ents[i];
    }

    *nread = count;
    free(dir_ino);
    return 0;
}

static void split_path(const char *path, char *parent, char *name)
{
    int len = (int)strlen(path);
    int i = len - 1;

    while (i >= 0 && path[i] != '/')
        i--;

    strncpy(name, path + i + 1, PATH_LEN_MAX);
    name[PATH_LEN_MAX - 1] = '\0';

    if (i <= 0) {
        strcpy(parent, "/");
    } else {
        strncpy(parent, path, i);
        parent[i] = '\0';
    }
}

char parent_path[PATH_LEN_MAX];
char filename[PATH_LEN_MAX];
int fs_open(const char *path, int flags, struct inode **out)
{
    memset(parent_path, 0, PATH_LEN_MAX);
    memset(filename, 0, PATH_LEN_MAX);
    split_path(path, parent_path, filename);

    struct inode *parent;
    if (fs_mkdir(parent_path, &parent) < 0)
        return -1;

    uint32_t data_blk = parent->din.direct[0];
    struct dirent dir;

    if (dir_is_exist(data_blk, filename, &dir)) {
        if ((flags & O_EXCL) && (flags & O_CREAT)) {
            return -1;
        }

        struct inode *node = malloc(sizeof(struct inode));
        if (!node) return -1;
        node->din = DInodeArray[dir.ino];
        node->ino = dir.ino;
        node->refcnt++;
        *out = node;
        return 0;
    }

    if (!(flags & O_CREAT)) {
        return -1;
    }

    struct inode *newfile;
    inode_alloc(&newfile);

    newfile->din.mode = FILE_TYPE_REG;
    newfile->din.size = 0;
    newfile->din.direct[0] = block_alloc();

    DInodeArray[newfile->ino] = newfile->din;

    if (dir_add_entry(parent, filename, newfile->ino, FILE_TYPE_REG) < 0)
        return -1;

    newfile->refcnt++;
    *out = newfile;

    return 0;
}

int fs_read(struct inode *inode, uint32_t off, void *buf, uint32_t len)
{
    uint32_t file_size = inode->din.size;

    if (off >= file_size)
        return 0;

    if (off + len > file_size)
        len = file_size - off;

    uint8_t *dst = (uint8_t *)buf;
    uint32_t total = 0;

    while (total < len) {
        uint32_t pos = off + total;
        uint32_t blk_index = pos / FS_BLOCK_SIZE;
        uint32_t blk_off   = pos % FS_BLOCK_SIZE;

        if (blk_index >= NDIRECT)
            break;

        uint32_t blkno = inode->din.direct[blk_index];

        if (blkno == 0xFFFFFFFF)
            break;

        uint32_t chunk = FS_BLOCK_SIZE - blk_off;
        if (chunk > len - total)
            chunk = len - total;

        if (g_bdev->read(g_bdev->ctx, blkno, blk_off,
                         dst + total, chunk) != 0)
            return -1;

        total += chunk;
    }

    return (int)total;
}

static uint8_t block_cache[FS_BLOCK_SIZE];
int fs_write(struct inode *inode, uint32_t off, const void *buf, uint32_t len)
{
    const uint8_t *src = (const uint8_t *)buf;
    uint32_t total = 0;

    while (total < len) {
        uint32_t pos = off + total;
        uint32_t blk_index = pos / FS_BLOCK_SIZE;
        uint32_t blk_off   = pos % FS_BLOCK_SIZE;

        if (blk_index >= NDIRECT)
            break;

        uint32_t blkno = inode->din.direct[blk_index];

        if (blkno == 0xFFFFFFFF)
            break;

        uint32_t chunk = FS_BLOCK_SIZE - blk_off;
        if (chunk > len - total)
            chunk = len - total;

        memcpy(block_cache + blk_off, src + total, chunk);

        if (g_bdev->erase(g_bdev->ctx, blkno) != 0)
            return -1;

        if (g_bdev->write(g_bdev->ctx, blkno, 0, block_cache, FS_BLOCK_SIZE) != 0)
            return -1;

        total += chunk;
    }
    inode->din.size = total;
    DInodeArray[inode->ino] = inode->din;

    return (int)total;
}


int fs_truncate(struct inode *inode, uint32_t newsize)
{
    (void*)inode;
    return 0;
}

int fs_close(struct inode *inode)
{
    if (!inode)
        return -1;

    if (--inode->refcnt == 0)
        free(inode);
    for (int i = 1; i < 4; i++) {
        g_bdev->erase(g_bdev->ctx, i);
    }
    if (g_bdev->write(g_bdev->ctx, 1, 0, InodeBitmap, sizeof(InodeBitmap))) {
        return -1;
    }
    if (g_bdev->write(g_bdev->ctx, 2, 0, BlockBitmap, sizeof(BlockBitmap))) {
        return -1;
    }
    if (g_bdev->write(g_bdev->ctx, 3, 0, DInodeArray, sizeof(DInodeArray))) {
        return -1;
    }

    return 0;
}

