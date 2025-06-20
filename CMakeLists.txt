cmake_minimum_required(VERSION 3.10)
project(eplayer)

# 设置交叉编译工具链（通过外部传入或在此指定）
# if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
# set(CMAKE_TOOLCHAIN_FILE "${PROJECT_SOURCE_DIR}/aarch64-toolchain.cmake")
# endif()

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置源码根目录变量
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# 启用 SSL 支持（MQTT 需要）
set(PAHO_WITH_SSL ON CACHE BOOL "Enable SSL support")

# 查找必要的库
find_package(PkgConfig REQUIRED)

# 动态链接的复杂库配置
find_package(OpenSSL REQUIRED) # 动态链接 OpenSSL
pkg_check_modules(WEBP REQUIRED libwebp libwebpdemux libwebpmux)
pkg_check_modules(JPEG REQUIRED libjpeg)
pkg_check_modules(PNG REQUIRED libpng)

pkg_search_module(QRENCODE REQUIRED libqrencode)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# 通过 FetchContent 获取 MQTT 库（本地编译时直接拉取源码）
include(FetchContent)

# set(ENV{http_proxy} "http://192.168.4.151:1087")
# set(ENV{https_proxy} "http://192.168.4.151:1087")

# http
FetchContent_Declare(
    cpp-httplib

    # GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    # GIT_TAG v0.21.0 # 指定稳定版本
    # CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
    SOURCE_DIR ../package/cpp-httplib-src
)

# 添加https支持
add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)
FetchContent_MakeAvailable(cpp-httplib)

# mqtt
FetchContent_Declare(
    paho.mqtt.c

    # GIT_REPOSITORY https://github.com/eclipse-paho/paho.mqtt.c.git
    # GIT_TAG v1.3.10
    SOURCE_DIR ../package/paho.mqtt.c-src
)

FetchContent_Declare(
    paho.mqtt.cpp

    # GIT_REPOSITORY https://github.com/eclipse-paho/paho.mqtt.cpp.git
    # GIT_TAG v1.2.0
    SOURCE_DIR ../package/paho.mqtt.cpp-src
)
FetchContent_MakeAvailable(paho.mqtt.c paho.mqtt.cpp)

# 字体绘制、图像处理、音频处理、实用工具​、数学与算法​
FetchContent_Declare(
    std

    # GIT_REPOSITORY https://github.com/nothings/stb.git
    # GIT_TAG master
    SOURCE_DIR ../package/std-src
)
FetchContent_MakeAvailable(std)

# 包含头文件
include_directories(
    ${PROJECT_SOURCE_DIR}/include

    ${CMAKE_BINARY_DIR}/_deps/std-src

    # ${CMAKE_BINARY_DIR}/_deps/jsoncpp-src/include # 手动指定json头文件
    ${CMAKE_BINARY_DIR}/_deps/cpp-httplib-src # 手动指定json头文件

    ${OPENSSL_INCLUDE_DIR}
    ${JPEG_INCLUDE_DIR}
    ${PNG_INCLUDE_DIR}
    ${WEBP_INCLUDE_DIRS}

    ${JSONCPP_INCLUDE_DIRS}
    ${QRENCODE_INCLUDE_DIRS}
    ${PAHO_MQTT_C_INCLUDE_DIRS}
    ${PAHO_MQTT_CPP_INCLUDE_DIRS}
    ${stb_SOURCE_DIR}
)

# 将源代码添加到此项目的可执行文件。
add_executable(eplayer
    src/main.cpp
    src/downloader.cpp
    src/http_client.cpp
    src/mqtt_client.cpp
    src/control.cpp
    src/daemon_thread.cpp
    src/task_repository.cpp
    src/display.cpp
    src/ImageDecoder.cpp
    src/Framebuffer.cpp
    src/Tools.cpp
    src/QrCodeGenerator.cpp
    src/TextRenderer.cpp
    src/stb_init.cpp
)

# 链接库（混合静态/动态）
target_link_libraries(eplayer

    # 动态链接的复杂库
    # ${CURL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${PNG_LIBRARIES}
    ${WEBP_LIBRARIES}

    # 系统库
    pthread
    stdc++fs

    paho-mqttpp3 # MQTT 库

    ${JSONCPP_LIBRARIES}
    ${QRENCODE_LIBRARIES}

    # ${PAHO_MQTT_C_LIBRARIES}
    # ${PAHO_MQTT_CPP_LIBRARIES}
)
