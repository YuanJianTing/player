cmake_minimum_required(VERSION 3.10)
project(eplayer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置源码根目录变量
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# 查找必要的库
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

# find_package(nlohmann_json 3.4.0 REQUIRED)
find_package(PahoMqttCpp REQUIRED)

# 查找 Paho MQTT C 库（C++库的依赖）
find_library(PAHO_MQTT_C_LIB paho-mqtt3as)
find_library(PAHO_MQTT_CPP_LIB paho-mqttpp3)

# 包含头文件
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${GST_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${JSONCPP_INCLUDE_DIRS}
    /usr/include/mqtt # Paho MQTT C++ 头文件路径
)

# 将源代码添加到此项目的可执行文件。
add_executable(eplayer
    src/main.cpp
    src/downloader.cpp
    src/GStreamerPlayer.cpp
    src/http_client.cpp
    src/mqtt_client.cpp
    src/control.cpp
    src/daemon_thread.cpp
    src/task_repository.cpp
    src/GStreamerImage.cpp
    src/display.cpp
)

# link_directories(${GSTREAMER_LIBRARY_DIRS})

# 链接必要的库
target_link_libraries(eplayer
    ${CURL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${GST_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    pthread
    stdc++fs
    ${PAHO_MQTT_CPP_LIB}
    ${PAHO_MQTT_C_LIB}
)
