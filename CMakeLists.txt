cmake_minimum_required(VERSION 3.10)
project(eplayer)

# 设置交叉编译工具链（通过外部传入或在此指定）
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${PROJECT_SOURCE_DIR}/aarch64-toolchain.cmake")
endif()

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# 设置源码根目录变量
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(PAHO_WITH_SSL ON CACHE BOOL "Enable SSL support")


include(FetchContent)

# 设置交叉编译的系统根目录
set(CMAKE_SYSROOT /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})


# 查找必要的库
find_package(PkgConfig REQUIRED)

# 设置 pkg-config 路径以查找 ARM64 库
set(ENV{PKG_CONFIG_PATH} "/usr/lib/aarch64-linux-gnu/pkgconfig")
set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib/aarch64-linux-gnu/pkgconfig")
set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})


set(QRENCODE_INCLUDE_DIRS "/usr/include/aarch64-linux-gnu")
set(QRENCODE_LIBRARIES "/usr/lib/aarch64-linux-gnu/libqrencode.so")

pkg_search_module(QRENCODE REQUIRED libqrencode)
#pkg_check_modules(JSONCPP REQUIRED jsoncpp)
# json
set(JSONCPP_INCLUDE_DIRS "/usr/include/jsoncpp")


#curl
set(CURL_LIBRARY "/usr/lib/aarch64-linux-gnu/libcurl.so")
set(CURL_INCLUDE_DIR "/usr/include/aarch64-linux-gnu")
find_package(CURL REQUIRED)

pkg_check_modules(WEBP REQUIRED libwebp libwebpdemux libwebpmux)


pkg_check_modules(JPEG REQUIRED libjpeg)
pkg_check_modules(PNG REQUIRED libpng)

#mqtt
FetchContent_Declare(
    paho.mqtt.cpp
    GIT_REPOSITORY https://github.com/eclipse-paho/paho.mqtt.cpp.git
    GIT_TAG v1.5.3  # 指定你需要的版本
)
FetchContent_MakeAvailable(paho.mqtt.cpp)

# 字体支持
#find_package(Freetype REQUIRED)

# 设置 OpenSSL 的 ARM64 路径
set(OPENSSL_ROOT_DIR "/usr/lib/aarch64-linux-gnu")
set(OPENSSL_INCLUDE_DIR "${CMAKE_SYSROOT}/usr/include/aarch64-linux-gnu/openssl")
find_package(OpenSSL REQUIRED)



# 包含头文件
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${JSONCPP_INCLUDE_DIRS}
    ${JPEG_INCLUDE_DIR}
    ${PNG_INCLUDE_DIR}
    ${WEBP_INCLUDE_DIRS}
    ${QRENCODE_INCLUDE_DIRS}
    ${PAHO_MQTT_C_INCLUDE_DIRS}
    ${PAHO_MQTT_CPP_INCLUDE_DIRS}
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
)

# 链接必要的库
target_link_libraries(eplayer
    ${CURL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    pthread
    stdc++fs
    paho-mqttpp3
    #${PAHO_MQTT_C_LIBRARIES}
    #${PAHO_MQTT_CPP_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${PNG_LIBRARIES}
    ${WEBP_LIBRARIES}
    ${QRENCODE_LIBRARIES}
    #Freetype::Freetype
)


# 设置可执行文件的 RPATH 以确保在目标设备上能找到库
set_target_properties(eplayer PROPERTIES
    INSTALL_RPATH "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu"
    BUILD_WITH_INSTALL_RPATH TRUE
)