#!/bin/bash

rootpath=$(cd $(dirname $0) && cd ../..; pwd)
debugpath=${rootpath}/debug
thirdpartypath=${rootpath}/thirdparty

# libjemalloc
cd ${thirdpartypath}/jemalloc || { echo "Failed to cd to ${thirdpartypath}/jemalloc"; exit 1; }

# 创建configure脚本
./autogen.sh

# 创建安装目录（如果不存在）
mkdir -p ${debugpath}/jemalloc/lib

# 创建build目录（如果不存在）
mkdir -p build
cd build

# 配置
../configure --prefix=${debugpath}/jemalloc

# 清理之前的构建文件
make clean

# 编译并安装
make -j8 || { echo "Make failed"; exit 1; }
make install || { echo "Make install failed"; exit 1; }

# 创建符号链接
cd ${debugpath}/libs
ln -sf ${debugpath}/jemalloc/lib/libjemalloc.so

add_color "jemalloc library has been successfully built and installed to ${debugpath}/jemalloc" green