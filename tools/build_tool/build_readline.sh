#!/bin/bash

rootpath=$(cd $(dirname $0) && cd ../..; pwd)
debugpath=${rootpath}/debug
thirdpartypath=${rootpath}/thirdparty

# libreadline
cd ${thirdpartypath}/readline || { echo "Failed to cd to ${thirdpartypath}/readline"; exit 1; }

# 创建 build 目录（如果不存在）
mkdir -p build
cd build || { echo "Failed to cd to build directory"; exit 1; }

# 创建安装目录（如果不存在）
mkdir -p ${debugpath}/readline

# 配置 readline 库
../configure --prefix=${debugpath}/readline --enable-static --disable-shared || { echo "Configure failed"; exit 1; }

# 清理之前的构建文件
make clean

# 编译 readline 库的所有内容
make everything || { echo "Make failed"; exit 1; }

# 安装 readline 库
make install || { echo "Make install failed"; exit 1; }

echo "Readline library has been successfully built and installed to ${debugpath}/readline"

cd ${debugpath}/libs

ln -s ${debugpath}/readline/*.a
