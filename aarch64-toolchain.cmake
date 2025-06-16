set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 指定交叉编译器路径
set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)

# 优先搜索 ARM64 专用路径
list(APPEND CMAKE_LIBRARY_PATH "/usr/lib/aarch64-linux-gnu")
list(APPEND CMAKE_INCLUDE_PATH "/usr/include/aarch64-linux-gnu")

# 设置 sysroot（目标系统的根文件系统）
set(CMAKE_SYSROOT /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})

# 指定库搜索路径
set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH /usr/local/aarch64/lib)
set(CMAKE_FIND_ROOT_PATH /usr/lib/aarch64-linux-gnu)


# 只在目标系统查找库
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

