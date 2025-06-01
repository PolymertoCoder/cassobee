#!/bin/bash

rootpath=$(cd $(dirname $0) && cd ../..; pwd)
debugpath=${rootpath}/debug
thirdpartypath=${rootpath}/thirdparty

# libcryptopp
cd ${thirdpartypath}/cryptopp || { echo "Failed to cd to ${thirdpartypath}/cryptopp"; exit 1; }

# 创建安装目录（如果不存在）
mkdir -p ${debugpath}/cryptopp/lib

# 清理之前的构建文件
make clean

# 配置编译选项并执行 make
make static -j8 CXXFLAGS="-O2 -DNDEBUG -fPIC" || { echo "Make failed"; exit 1; }

# 移动静态库
mv libcryptopp.a ${debugpath}/cryptopp/lib

# 创建符号链接
cd ${debugpath}/libs
ln -sf ${debugpath}/cryptopp/lib/libcryptopp.a

add_color "cryptopp library has been successfully built and installed to ${debugpath}/cryptopp" green
