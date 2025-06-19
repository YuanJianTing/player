cmake_minimum_required(VERSION 3.10)
project(eplayer)

# 设置交叉编译工具链（通过外部传入或在此指定）
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${PROJECT_SOURCE_DIR}/aarch64-toolchain.cmake")
endif()

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 全局配置：混合静态/动态链接
set(BUILD_SHARED_LIBS OFF) # 默认优先静态库
set(CMAKE_FIND_LIBRARY_SUFFIXES .a .so) # 优先查找 .a 文件

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

# 通过 FetchContent 获取 MQTT 库（本地编译时直接拉取源码）
include(FetchContent)
set(ENV{http_proxy} "http://192.168.4.111:1087")
set(ENV{https_proxy} "http://192.168.4.111:1087")

# -------------------------------------------------------------------
# 1. 静态链接的小型库：jsoncpp 和 qrencode
# -------------------------------------------------------------------
# jsoncpp
FetchContent_Declare(
    jsoncpp
    GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
    GIT_TAG 1.9.5 # 指定稳定版本
    CMAKE_ARGS -DJSONCPP_WITH_TESTS=OFF -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF -DBUILD_SHARED_LIBS=OFF
)
FetchContent_MakeAvailable(jsoncpp)

# http
FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.21.0 # 指定稳定版本
    CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
)

# 添加https支持
add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)
FetchContent_MakeAvailable(cpp-httplib)

# qrencode
FetchContent_Declare(
    qrencode
    GIT_REPOSITORY https://github.com/fukuchi/libqrencode.git
    GIT_TAG v4.1.1 # 指定稳定版本
    CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
)
FetchContent_MakeAvailable(qrencode)

# mqtt
FetchContent_Declare(
    paho.mqtt.c
    GIT_REPOSITORY https://github.com/eclipse-paho/paho.mqtt.c.git
    GIT_TAG v1.3.10
)

FetchContent_Declare(
    paho.mqtt.cpp
    GIT_REPOSITORY https://github.com/eclipse-paho/paho.mqtt.cpp.git
    GIT_TAG v1.2.0
)
set(PAHO_BUILD_STATIC ON CACHE BOOL "Build static MQTT libraries")
FetchContent_MakeAvailable(paho.mqtt.c paho.mqtt.cpp)

# 字体支持
# find_package(Freetype REQUIRED)

# 包含头文件
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/_deps/jsoncpp-src/include # 手动指定json头文件

    ${CMAKE_BINARY_DIR}/_deps/cpp-httplib-src # 手动指定http头文件
    ${OPENSSL_INCLUDE_DIR}
    ${JPEG_INCLUDE_DIR}
    ${PNG_INCLUDE_DIR}
    ${WEBP_INCLUDE_DIRS}
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

# 链接库（混合静态/动态）
target_link_libraries(eplayer

    # 静态链接的库（通过 FetchContent）
    jsoncpp_static # jsoncpp 的静态库目标名
    qrencode # qrencode 的静态库目标名
    paho-mqttpp3-static # MQTT 静态库

    # cpp-httplib

    # 动态链接的复杂库
    ${OPENSSL_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${PNG_LIBRARIES}
    ${WEBP_LIBRARIES}

    # 系统库
    pthread
    stdc++fs

    # Freetype::Freetype
)

# 设置 RPATH（可选，确保运行时找到库）
set_target_properties(eplayer PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib" # 本地运行时相对路径
    BUILD_WITH_INSTALL_RPATH TRUE
)

# 将 MQTT 动态库拷贝到输出目录
install(TARGETS eplayer
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

# 收集需要打包的动态库
file(GLOB_RECURSE DYNAMIC_LIBS
    "${CURL_LIBRARIES}"
    "${OPENSSL_LIBRARIES}"

    "${JPEG_LIBRARIES}"
    "${PNG_LIBRARIES}"
    "${WEBP_LIBRARIES}"
)

# 去重并过滤掉静态库
list(REMOVE_DUPLICATES DYNAMIC_LIBS)
list(FILTER DYNAMIC_LIBS EXCLUDE REGEX ".+\\.a$")

# 安装可执行文件和动态库
install(TARGETS eplayer DESTINATION bin)
install(FILES ${DYNAMIC_LIBS} DESTINATION lib)
