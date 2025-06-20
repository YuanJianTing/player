# eplayer

> 基于C++开发的流媒体播放器，支持多种媒体格式和协议

## 开发环境要求

- **操作系统**: Ubuntu 22.04 LTS
- **编译器**: GCC 11+ (建议11.3.0)
- **构建工具**: CMake 3.22+
- **开发工具**: Visual Studio Code

## 项目地址

```bash
git clone https://github.com/YuanJianTing/player.git
cd player
```

## 安装依赖库

```bash
# 更新系统包列表
sudo apt update

# 安装编译工具和基础依赖
sudo apt install -y build-essential cmake pkg-config

# 字体支持
sudo apt install libfreetype6-dev

sudo apt install -y libqrencode4 libqrencode-dev

sudo apt install -y \
    libjsoncpp-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    libpaho-mqttpp-dev \
    libpaho-mqtt-dev \
    libpng-dev \
    libjpeg-dev \
    curl \
    libwebp-dev
```+

## 安装开发工具（VSCode）

1. [下载VSCode安装包](https://code.visualstudio.com/download) 或使用命令安装：
```bash
sudo snap install --classic code
# 安装调试工具
sudo apt-get install gdb

# 如果使用非arm64架构，则安装交叉编译环境
apt install gcc-aarch64-linux-gnu
# 验证安装
aarch64-linux-gnu-gcc --version

# 安装drm_info 工具 记录下可用的 plane-id 和显示模式
apt-get install drm-info

# 如果在x86、x64设备中调试，则需要安装 qemu arm64模拟器
sudo apt install qemu-system-arm

```

2. 安装推荐扩展：
   - CMake Tools (ms-vscode.cmake-tools)
   - C/C++ Extension Pack (ms-vscode.cpptools-extension-pack)

## 编译项目

```bash
# 创建构建目录并进入
mkdir build
cd build

# 生成Makefile
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/home/ubuntu/eplayer

# 编译项目（使用4个线程并行编译）
make -j4

make install
```

## 运行项目

```bash
# 在build目录下
./bin/player --source=[媒体源URL]
```

示例：
```bash
./bin/player --source=https://example.com/stream.m3u8
```

## 环境验证

```bash
# 检查CMake版本
cmake --version

# 检查gstreamer版本
#gst-inspect-1.0 --version

# 检查jsoncpp头文件是否存在
ls /usr/include/jsoncpp/json
```