# CMakeList.txt: player 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#

# 将源代码添加到此项目的可执行文件。
add_executable (player "player.cpp" "player.h" "GStreamerPlayer.cpp" "GStreamerPlayer.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET player PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。

# 查找GStreamer库
if(WIN32)
    # Windows平台配置 已配置环境变量
    #set(GSTREAMER_ROOT "C:/Program Files/gstreamer/1.0/msvc_x86_64" CACHE PATH "GStreamer安装路径")
    #set(GST_PLUGIN_PATH "C:/Program Files/gstreamer/1.0/msvc_x86_64/lib/gstreamer-1.0/" CACHE PATH "GStreamer安装路径")
    
    list(APPEND CMAKE_PREFIX_PATH ${GSTREAMER_ROOT})
    
    find_path(GSTREAMER_INCLUDE_DIR gst/gst.h
        PATHS ${GSTREAMER_ROOT}/include/gstreamer-1.0
    )
    
    find_library(GSTREAMER_LIBRARY
        NAMES gstreamer-1.0
        PATHS ${GSTREAMER_ROOT}/lib
    )
    
    find_library(GLIB_LIBRARY
        NAMES glib-2.0
        PATHS ${GSTREAMER_ROOT}/lib
    )
    
    find_library(GOBJECT_LIBRARY
        NAMES gobject-2.0
        PATHS ${GSTREAMER_ROOT}/lib
    )
    
    include_directories(
        ${GSTREAMER_INCLUDE_DIR}
        ${GSTREAMER_ROOT}/include/glib-2.0
        ${GSTREAMER_ROOT}/lib/glib-2.0/include
    )

    # Windows平台DLL复制
   # add_custom_command(TARGET player POST_BUILD
   #     COMMAND ${CMAKE_COMMAND} -E copy_if_different
   #        "${GSTREAMER_ROOT}/bin/gstreamer-1.0-0.dll"
   #         $<TARGET_FILE_DIR:player>
   #     COMMAND ${CMAKE_COMMAND} -E copy_if_different
   #         "${GSTREAMER_ROOT}/bin/glib-2.0-0.dll"
   #         $<TARGET_FILE_DIR:player>
   #     COMMAND ${CMAKE_COMMAND} -E copy_if_different
   #         "${GSTREAMER_ROOT}/bin/gobject-2.0-0.dll"
   #         $<TARGET_FILE_DIR:player>
   # )

else()
    # Linux平台配置
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
    
    include_directories(${GSTREAMER_INCLUDE_DIRS})
    link_directories(${GSTREAMER_LIBRARY_DIRS})
endif()

# 链接库
if(WIN32)
    target_link_libraries(player
        ${GSTREAMER_LIBRARY}
        ${GLIB_LIBRARY}
        ${GOBJECT_LIBRARY}
    )
    
    # Windows平台复制DLL
   # add_custom_command(TARGET player POST_BUILD
   #     COMMAND ${CMAKE_COMMAND} -E copy
   #         "${GSTREAMER_ROOT}/bin/gstreamer-1.0-0.dll"
   #         $<TARGET_FILE_DIR:player>
   # )
else()
    target_link_libraries(player
        ${GSTREAMER_LIBRARIES}
    )
endif()

# 安装目标
install(TARGETS player
    RUNTIME DESTINATION bin
)

# 安装头文件
install(FILES
    player.h
    GStreamerPlayer.h
    DESTINATION include
)