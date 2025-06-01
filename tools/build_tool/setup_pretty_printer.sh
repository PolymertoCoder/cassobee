#!/bin/bash

# 获取GCC版本
GCC_VERSION=$(gcc -dumpversion)

# 设置Pretty Printer配置文件路径
GDB_INIT_FILE="$HOME/.cassobee.gdb"
GDBINIT_FILE="$HOME/.gdbinit"

# 生成Pretty Printer配置文件
cat <<EOL > ${GDB_INIT_FILE}
python
import sys
import os

gcc_version = '${GCC_VERSION}'
libstdcxx_path = '/usr/share/gcc/python'

sys.path.insert(0, libstdcxx_path)
from libstdcxx.v6.printers import register_libstdcxx_printers
register_libstdcxx_printers(gdb.current_objfile())
end
EOL

# 检查.gdbinit文件是否存在
if [ ! -f ${GDBINIT_FILE} ]; then
    touch ${GDBINIT_FILE}
fi

# 检查.gdbinit文件是否已经包含source ~/.cassobee.gdb
if ! grep -q "source ~/.cassobee.gdb" ${GDBINIT_FILE}; then
    echo "source ~/.cassobee.gdb" >> ${GDBINIT_FILE}
    echo "已将 'source ~/.cassobee.gdb' 添加到 ${GDBINIT_FILE}"
else
    echo "${GDBINIT_FILE} 已包含 'source ~/.cassobee.gdb'"
fi

echo "Pretty Printer配置文件已生成：${GDB_INIT_FILE}"
