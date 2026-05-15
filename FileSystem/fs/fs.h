#ifndef FS_H
#define FS_H

#include <stdint.h>

#define FS_ERR_OK     0
#define FS_ERR_IO    -5
#define FS_ERR_NOMEM -12
#define FS_ERR_INVAL -22

/* File type macros */
#define S_IFMT   0xF000  /* bitmask for the file type */
#define S_IFREG  0x8000  /* regular file */
#define S_IFDIR  0x4000  /* directory */
#define S_IFCHR  0x2000  /* character device */
#define S_IFBLK  0x6000  /* block device */
#define S_IFIFO  0x1000  /* FIFO */
#define S_IFLNK  0xA000  /* symbolic link */
#define S_IFSOCK 0xC000  /* socket */

/* Error codes */
#define EPERM       1   /* operation not permitted */
#define ENOENT      2   /* no such file or directory */
#define ENOMEM     12   /* out of memory */
#define EEXIST     17   /* file exists */
#define EINVAL     22   /* invalid argument */
#define ENOTDIR    20   /* not a directory */
#define EISDIR     21   /* is a directory */
#define ENOSPC     28   /* no space left on device */
#define ENOTEMPTY  39   /* directory not empty */

#define FILE_TYPE_UNKNOWN   0
#define FILE_TYPE_REG       1
#define FILE_TYPE_DIR       2
#define FILE_TYPE_SYMLINK   3

#define O_RDONLY   0x01
#define O_WRONLY   0x02
#define O_RDWR     0x04
#define O_CREAT    0x08
#define O_EXCL     0x10

/*CONFIG!!!*/
#define FS_BLOCK_COUNT    32U
#define FS_BLOCK_SIZE     1024   /* STM32F1 page size */
#define FS_PROG_SIZE      2U
#define FS_READ_SIZE      2U
#define FS_CACHE_SIZE     64U
#define FS_LOOKAHEAD_SIZE 32U
#define FS_BLOCK_CYCLES   100U


struct superblock {
    uint32_t magic;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t inode_start;
    uint32_t data_start;
    uint32_t inode_count;
};

/*
 * for flash
 * */
#define NDIRECT 8
struct dinode {
    uint16_t mode;
    uint32_t size;
    uint32_t direct[NDIRECT];
    uint32_t indirect;
} __attribute__((aligned(64)));

/*
 * for memory
 */
struct inode {
    uint32_t ino;
    struct dinode din;
    uint16_t refcnt;
};

#define NAME_MAX 32
struct dirent {
    uint8_t type;
    uint32_t ino;
    char name[NAME_MAX];
}__attribute__((aligned(64)));

struct mount_min {
    struct superblock *sb;
    uint32_t dev_id;
    uint32_t block_size;
    uint32_t inode_start;
    uint32_t data_start;
};

struct dqblk_min {
    uint32_t hardlimit_blocks;
    uint32_t used_blocks;
    uint32_t hardlimit_inodes;
    uint32_t used_inodes;
};

struct dquot_min {
    uint32_t id;
    struct dqblk_min dqb;
    uint16_t refcnt;
    uint16_t flags;
};

struct lock_min {
    uint32_t ino;
    uint32_t type;
    uint32_t owner_id;
};

struct fs_blkdev {
    uint32_t block_size;
    uint32_t block_count;
    uint32_t prog_size;
    uint32_t read_size;
    void *ctx;

    int (*read)(void *ctx, uint32_t blk, uint32_t off, void *buf, uint32_t len);
    int (*write)(void *ctx, uint32_t blk, uint32_t off, const void *buf, uint32_t len);
    int (*erase)(void *ctx, uint32_t blk);
    int (*sync)(void *ctx);
};

int fs_mount(struct superblock *sb, struct fs_blkdev *bdev);
int fs_unmount(struct superblock *sb);
int fs_format(struct superblock *sb);

int fs_mkdir(const char *path, struct inode **out);
int fs_readdir(const char *path, struct dirent *buf, int max, int *nread);

int fs_open(const char *path, int flags, struct inode **out);
int fs_read(struct inode *inode, uint32_t off, void *buf, uint32_t len);
int fs_write(struct inode *inode, uint32_t off, const void *buf, uint32_t len);
int fs_close(struct inode *inode);

int fs_truncate(struct inode *inode, uint32_t newsize);
int fs_sync(void);




#endif
