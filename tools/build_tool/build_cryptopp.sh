#!/bin/bash

rootpath=$(cd $(dirname $0) && cd ../..; pwd)
debugpath=${rootpath}/debug
thirdpartypath=${rootpath}/thirdparty

# libcryptopp
cd ${thirdpartypath}/cryptopp || { echo "Failed to cd to ${thirdpartypath}/cryptopp"; exit 1; }

# 创建安装目录（如果不存在）
mkdir -p ${debugpath}/cryptopp

# 清理之前的构建文件
make clean

# 配置编译选项并执行 make
make static -j8 CXXFLAGS="-O2 -DNDEBUG" PREFIX=${debugpath}/cryptopp || { echo "Make failed"; exit 1; }

echo "cryptopp library has been successfully built and installed to ${debugpath}/cryptopp"
