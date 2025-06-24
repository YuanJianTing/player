cmake_minimum_required(VERSION 3.15)
project(eplayer)

# 设置 vcpkg 工具链
#set(CMAKE_TOOLCHAIN_FILE "/home/ubuntu/work/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
# 设置 vcpkg 工具链（如果使用）
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

# 手动指定 vcpkg 安装目录
set(VCPKG_INSTALLED_DIR ${CMAKE_SOURCE_DIR}/vcpkg_installed)
list(APPEND CMAKE_PREFIX_PATH ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET})


# 设置静态链接
set(VCPKG_TARGET_TRIPLET "arm64-linux" CACHE STRING "")
set(VCPKG_LIBRARY_LINKAGE "static" CACHE STRING "")

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 或者明确指定不使用SSL版本
set(PAHO_WITH_SSL OFF CACHE BOOL "Disable SSL support in Paho MQTT")

# 查找必要的库
find_package(OpenSSL REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(WebP CONFIG REQUIRED 
    PATHS "/home/ubuntu/work/vcpkg/packages/libwebp_arm64-linux/share/WebP"
    NO_DEFAULT_PATH)
find_package(jsoncpp REQUIRED)

find_path(QRENCODE_INCLUDE_DIR NAMES qrencode.h)
find_library(QRENCODE_LIBRARY_RELEASE qrencode)
#find_library(QRENCODE_LIBRARY_DEBUG qrencoded)
#set(QRENCODE_LIBRARIES optimized ${QRENCODE_LIBRARY_RELEASE} debug ${QRENCODE_LIBRARY_DEBUG})
message(STATUS "LibQREncode found: ${QRENCODE_LIBRARY_RELEASE}")



find_package(httplib CONFIG REQUIRED)
find_package(eclipse-paho-mqtt-c CONFIG REQUIRED)


#set(ENV{http_proxy} "http://192.168.4.151:1087")
#set(ENV{https_proxy} "http://192.168.4.151:1087")
# stb 单文件库（vcpkg 中没有，仍使用 FetchContent）
#include(FetchContent)

# 添加https支持
add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)

message( ${CMAKE_BINARY_DIR} )
# 包含头文件
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
    ${JPEG_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${WEBP_INCLUDE_DIRS}
    ${QRENCODE_INCLUDE_DIR}
)


# 添加可执行文件
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

# 链接库（全部静态链接）
target_link_libraries(eplayer
    # vcpkg 提供的静态库
    jsoncpp
    httplib::httplib
    # target_link_libraries(main PRIVATE eclipse-paho-mqtt-c::paho-mqtt3a-static eclipse-paho-mqtt-c::paho-mqtt3c-static eclipse-paho-mqtt-c::paho-mqtt3as-static eclipse-paho-mqtt-c::paho-mqtt3cs-static)
    eclipse-paho-mqtt-c::paho-mqtt3cs-static
    # 系统库
    OpenSSL::SSL
    OpenSSL::Crypto
    ${JPEG_LIBRARIES}
    ${PNG_LIBRARIES}
    ${WEBP_LIBRARIES}
    ${QRENCODE_LIBRARY_RELEASE}
    pthread
    stdc++fs
    dl
)

# 设置 RPATH
set_target_properties(eplayer PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# 安装目标
install(TARGETS eplayer DESTINATION bin)