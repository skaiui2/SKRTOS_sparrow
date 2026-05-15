#!/bin/bash

# 定义源文件和目标文件夹
SOURCE_FOLDER="/home/skaiuijing/DevRTOS/imxproject/armgcc"  
TARGET_FOLDER="/home/skaiuijing/DevRTOS/imxproject/armgcc/down"

cd "$SOURCE_FOLDER"

./clean.sh
./build_ddr_debug.sh

cd ddr_debug
# 覆盖移动文件
cp -f "sdk20-app.bin" "$TARGET_FOLDER"

# 切换到目标文件夹
cd "$TARGET_FOLDER"

# 执行命令
./imxdownload sdk20-app.bin /dev/sdb
