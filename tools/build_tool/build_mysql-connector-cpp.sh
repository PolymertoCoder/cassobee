#!/bin/bash

# 设置路径
rootpath=$(cd $(dirname $0) && cd ../..; pwd)
debugpath=${rootpath}/debug
thirdpartypath=${rootpath}/thirdparty

# libmysqlconnectorcpp
cd ${thirdpartypath}/mysql-connector-cpp || { echo "Failed to cd to ${thirdpartypath}/mysql-connector-cpp"; exit 1; }

# 创建安装目录（如果不存在）
mkdir -p ${debugpath}/mysql-connector-cpp/lib

# 创建build目录
mkdir -p build
cd build

# 清理之前的构建文件
# rm -rf *

# 配置编译选项并执行 cmake
cmake .. -DCMAKE_INSTALL_PREFIX=${debugpath}/mysql-connector-cpp \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_LIBDIR=lib \
         -DWITH_JDBC=ON || { echo "CMake failed"; exit 1; }

# 编译并安装
make -j8 || { echo "Make failed"; exit 1; }
make install || { echo "Make install failed"; exit 1; }

# 创建符号链接
cd ${debugpath}/libs
ln -sf ${debugpath}/mysql-connector-cpp/lib64/libmysqlcppconn.so

add_color "mysql-connector-cpp library has been successfully built and installed to ${debugpath}/mysql-connector-cpp" green
