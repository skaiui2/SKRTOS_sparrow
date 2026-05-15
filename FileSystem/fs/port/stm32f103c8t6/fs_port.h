#ifndef FS_PORT_H
#define FS_PORT_H

#include "stm32f1xx_hal.h"
#include "fs.h"

#define FS_FLASH_BASE     0x0800C000U


void fs_port_init(void);
int fs_port_mount(struct superblock *sb);
void fs_port_deinit(struct superblock *sb);

#endif
