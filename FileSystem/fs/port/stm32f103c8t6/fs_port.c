#include "fs_port.h"
#include "fs.h"
#include <memory.h>

static int fs_bd_read(void *ctx, uint32_t block,
                      uint32_t off, void *buffer, uint32_t size) {
    uint32_t addr = FS_FLASH_BASE + block * FS_BLOCK_SIZE + off;
    memcpy(buffer, (const void*)addr, size);
    return FS_ERR_OK;
}

static int fs_bd_write(void *ctx, uint32_t block,
                       uint32_t off, const void *buffer, uint32_t size) {
    if ((off % FS_PROG_SIZE) || (size % FS_PROG_SIZE)) return FS_ERR_IO;
    HAL_FLASH_Unlock();
    const uint16_t *p = (const uint16_t*)buffer;
    uint32_t addr = FS_FLASH_BASE + block * FS_BLOCK_SIZE + off;
    for (uint32_t i = 0; i < size/2; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i*2, p[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return FS_ERR_IO;
        }
        if (*(volatile uint16_t*)(addr + i*2) != p[i]) {
            HAL_FLASH_Lock();
            return FS_ERR_IO;
        }
    }
    HAL_FLASH_Lock();
    return FS_ERR_OK;
}

static int fs_bd_erase(void *ctx, uint32_t block) {
    uint32_t page_addr = FS_FLASH_BASE + block * FS_BLOCK_SIZE;
    FLASH_EraseInitTypeDef ei = {0};
    ei.TypeErase   = FLASH_TYPEERASE_PAGES;
    ei.PageAddress = page_addr;
    ei.NbPages     = 1;
    uint32_t err = 0;
    HAL_FLASH_Unlock();
    HAL_StatusTypeDef st = HAL_FLASHEx_Erase(&ei, &err);
    HAL_FLASH_Lock();
    return (st == HAL_OK) ? FS_ERR_OK : FS_ERR_IO;
}

static int fs_bd_sync(void *ctx) {
    return FS_ERR_OK;
}

struct fs_blkdev fs_dev;
extern struct fs_blkdev *g_bdev;
void fs_port_init(void) {
    g_bdev = &fs_dev;
    memset(&fs_dev, 0, sizeof(fs_dev));
    fs_dev.read  = fs_bd_read;
    fs_dev.write = fs_bd_write;
    fs_dev.erase = fs_bd_erase;
    fs_dev.sync  = fs_bd_sync;

    fs_dev.read_size   = FS_READ_SIZE;
    fs_dev.prog_size   = FS_PROG_SIZE;
    fs_dev.block_size  = FS_BLOCK_SIZE;
    fs_dev.block_count = FS_BLOCK_COUNT;
}

int fs_port_mount(struct superblock *sb)
{
    int err = fs_mount(sb, &fs_dev);
    if (err) {
        err = fs_format(sb);
        if (err) return err;
        err = fs_mount(sb, &fs_dev);
    }
    return err;
}

void fs_port_deinit(struct superblock *sb)
{
    fs_unmount(sb);
}
