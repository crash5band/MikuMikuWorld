cmake_minimum_required(VERSION 3.24)

project(MikuMikuWorld VERSION 3.4.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_COLOR_DIAGNOSTICS ON)

# Platform-specific configurations
if (WIN32)
        message(FATAL_ERROR "Build using CMake is not supported on Windows. Please use the provided Visual Studio solution instead.")
elseif(APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
        
        set(INCLUDE_FOR_SOURCE_DIR
                MikuMikuWorld/*.mm
        )
        set(REMOVE_FROM_SOURCE_DIR
                ${CMAKE_CURRENT_SOURCE_DIR}/MikuMikuWorld/main.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/MikuMikuWorld/resource.h
        )
        set(PLATFROM_COMPILE_DEFINITIONS
                "PAL_STDCPP_COMPAT"
                "IMGUI_IMPL_OPENGL_ES3"
                "_XM_NO_INTRINSICS_"
                "MA_NO_RUNTIME_LINKING"
        )
        set(PLATFORM_LINK_LIBRARIES
                "-framework IOKit"
                "-framework AppKit"
                "-framework CoreAudio"
                "-framework AudioToolbox"
                "${CMAKE_SOURCE_DIR}/Depends/GLFW/lib/libglfw3.a"
                "${CMAKE_SOURCE_DIR}/Depends/zlib/lib/libz.a"
                "${CMAKE_SOURCE_DIR}/Depends/ANGLE/lib/libEGL.dylib"
                "${CMAKE_SOURCE_DIR}/Depends/ANGLE/lib/libGLESv2.dylib"
        )

        configure_file(
                ${CMAKE_SOURCE_DIR}/Info.plist.in
                ${CMAKE_SOURCE_DIR}/Info.plist
        )
        set_target_properties(MikuMikuWorld PROPERTIES
                MACOSX_BUNDLE TRUE
                MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist"
        )
elseif(LINUX)
        set(PLATFROM_COMPILE_DEFINITIONS
                "PAL_STDCPP_COMPAT"
                "_XM_NO_INTRINSICS_"
                "IMGUI_IMPL_OPENGL_ES3"
                "MMW_APP_VERSION=\"${PROJECT_VERSION}\""
        )
        set(PLATFORM_LINK_LIBRARIES
                # GLFW 3.4 build
                "${CMAKE_SOURCE_DIR}/Depends/GLFW/lib/unix/libglfw3.a"
                "z"
                "EGL"
                "GLESv2"
        )

else()
        message(FATAL_ERROR "Unsupported platform.")
endif()


file(GLOB_RECURSE ALL_FILES
        MikuMikuWorld/*.h
        MikuMikuWorld/*.c
        MikuMikuWorld/*.hpp
        MikuMikuWorld/*.cpp
        ${INCLUDE_FOR_SOURCE_DIR}
)

list(REMOVE_ITEM ALL_FILES ${REMOVE_FROM_SOURCE_DIR})

add_executable(MikuMikuWorld
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

target_compile_definitions(MikuMikuWorld PRIVATE ${PLATFROM_COMPILE_DEFINITIONS})

target_compile_options(MikuMikuWorld PRIVATE "-Wno-format-security")

target_link_libraries(MikuMikuWorld ${PLATFORM_LINK_LIBRARIES})

if (APPLE)
        add_custom_command(TARGET MikuMikuWorld POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/MikuMikuWorld/res/mmw_icon.icns" "${CMAKE_BINARY_DIR}/MikuMikuWorld.app/Contents/Resources/app.icns"
                COMMENT "Copying app.icns"
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
else()
        make_directory("${CMAKE_BINARY_DIR}/MikuMikuWorld")
        set_target_properties(MikuMikuWorld PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/MikuMikuWorld"
        )
        add_custom_command(TARGET MikuMikuWorld POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/MikuMikuWorld/res" "${CMAKE_BINARY_DIR}/MikuMikuWorld/res"
                COMMENT "Copying res"
        )
endif()