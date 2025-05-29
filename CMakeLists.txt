cmake_minimum_required(VERSION 3.30)

project(MikuMikuWorld C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE ALL_FILES
        MikuMikuWorld/*.h
        MikuMikuWorld/*.c
        MikuMikuWorld/*.hpp
        MikuMikuWorld/*.cpp
        MikuMikuWorld/*.hh
        MikuMikuWorld/*.mm
)

list(REMOVE_ITEM ALL_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/MikuMikuWorld/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/MikuMikuWorld/resource.h
)

add_executable(MikuMikuWorld
        MikuMikuWorld/main.mm
        ${ALL_FILES}
        Depends/stb_vorbis/stb_vorbis.c
)

target_include_directories(MikuMikuWorld PRIVATE
        Depends/DirectXMath-master/Inc
        Depends/json
        Depends/stb_image
        Depends/stb_vorbis
        Depends/miniaudio
        Depends/GLFW/include
        Depends/ANGLE/include
        Depends/zlib/include
)

target_compile_definitions(MikuMikuWorld PRIVATE
        "PAL_STDCPP_COMPAT"
        "IMGUI_IMPL_OPENGL_ES3"
        "IMGUI_USE_WCHAR32"
        "_XM_NO_INTRINSICS_"
        "MA_NO_RUNTIME_LINKING"
)

target_link_libraries(MikuMikuWorld
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
        "-framework AppKit"
        "-framework CoreGraphics"
        "-framework CoreServices"
        "-framework Foundation"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "${CMAKE_SOURCE_DIR}/Depends/GLFW/lib/libglfw3.a"
        "${CMAKE_SOURCE_DIR}/Depends/zlib/lib/libz.a"
        "${CMAKE_SOURCE_DIR}/Depends/ANGLE/lib/libEGL.dylib"
        "${CMAKE_SOURCE_DIR}/Depends/ANGLE/lib/libGLESv2.dylib"
)

set_target_properties(MikuMikuWorld PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "MikuMikuWorld"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist"
)

add_custom_command(TARGET MikuMikuWorld POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/Depends/ANGLE/lib/libEGL.dylib" "${CMAKE_BINARY_DIR}/MikuMikuWorld.app/Contents/MacOS/libEGL.dylib"
        COMMENT "Copying libEGL.dylib"
)

add_custom_command(TARGET MikuMikuWorld POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/Depends/ANGLE/lib/libGLESv2.dylib" "${CMAKE_BINARY_DIR}/MikuMikuWorld.app/Contents/MacOS/libGLESv2.dylib"
        COMMENT "Copying libGLESv2.dylib"
)

add_custom_command(TARGET MikuMikuWorld POST_BUILD
        COMMAND install_name_tool -add_rpath @loader_path "$<TARGET_FILE:MikuMikuWorld>"
        COMMENT "Run install_name_tool"
)

add_custom_command(TARGET MikuMikuWorld POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/MikuMikuWorld/res" "${CMAKE_BINARY_DIR}/MikuMikuWorld.app/Contents/Resources/res"
        COMMENT "Copying res"
)
