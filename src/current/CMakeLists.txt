cmake_minimum_required(VERSION 3.16)
project(VideoCapture LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
endif()

# ---------------------------------------------------------------------------
# FFMPEG_ROOT: путь к локальной статической сборке ffmpeg внутри репозитория
# ---------------------------------------------------------------------------
if (NOT DEFINED FFMPEG_ROOT)
    if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/../../deps/Windows/ffmpeg_static" CACHE PATH "Path to FFmpeg root (local)")
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/../../deps/MacOS/ffmpeg" CACHE PATH "Path to FFmpeg root (local)")
    else()
        set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/../../deps/Linux/ffmpeg" CACHE PATH "Path to FFmpeg root (local)")
    endif()
endif()
message(STATUS "FFMPEG_ROOT = ${FFMPEG_ROOT}")

# Optional: manually specify exact library files (full paths) if auto-detection fails.
# Example:
#   -DFFMPEG_LIBS="T:/.../lib/libavcodec.a;T:/.../lib/libavutil.a;T:/.../lib/libswscale.a"
if (NOT DEFINED FFMPEG_LIBS)
    set(FFMPEG_LIBS "" CACHE STRING "Optional semicolon-separated list of full paths to ffmpeg libs (overrides autodetect)")
endif()

# ---------------------------------------------------------------------------
# find other deps
# ---------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED)

# sources
set(SOURCES
    main.cpp
    mainwindow.cpp
    capturethread.cpp
    EncoderWorker.cpp
    DecoderWorker.cpp
)

set(HEADERS
    mainwindow.h
    capturethread.h
    EncoderWorker.h
    DecoderWorker.h
    video_defaults.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# link Qt + OpenCV
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets ${OpenCV_LIBS})

# ---------------------------------------------------------------------------
# FFmpeg manual include + lib linking (no .pc usage)
# ---------------------------------------------------------------------------
if (EXISTS "${FFMPEG_ROOT}/include")
    target_include_directories(${PROJECT_NAME} PRIVATE "${FFMPEG_ROOT}/include")
    message(STATUS "Using FFmpeg include: ${FFMPEG_ROOT}/include")
else()
    message(WARNING "FFMPEG_ROOT/include not found: ${FFMPEG_ROOT}/include")
endif()

# If the user provided explicit full paths in FFMPEG_LIBS, use them (explicit override).
if (FFMPEG_LIBS)
    message(STATUS "Using user-specified FFMPEG_LIBS:")
    foreach(p IN LISTS FFMPEG_LIBS)
        message(STATUS "  ${p}")
        target_link_libraries(${PROJECT_NAME} PRIVATE "${p}")
    endforeach()
else()
    # try to autodetect libraries in FFMPEG_ROOT/lib
    set(FF_LIB_DIR "${FFMPEG_ROOT}/lib")
    if (EXISTS "${FF_LIB_DIR}")
        message(STATUS "Searching for ffmpeg libs in: ${FF_LIB_DIR}")

        # try to find common ffmpeg libraries (static or shared)
        file(GLOB FF_AVCODEC   "${FF_LIB_DIR}/*avcodec*.a"  "${FF_LIB_DIR}/*avcodec*.lib"  "${FF_LIB_DIR}/*avcodec*.so"  "${FF_LIB_DIR}/*avcodec*.dll"  "${FF_LIB_DIR}/*avcodec*.dylib")
        file(GLOB FF_AVFORMAT  "${FF_LIB_DIR}/*avformat*.a" "${FF_LIB_DIR}/*avformat*.lib" "${FF_LIB_DIR}/*avformat*.so" "${FF_LIB_DIR}/*avformat*.dll" "${FF_LIB_DIR}/*avformat*.dylib")
        file(GLOB FF_AVUTIL    "${FF_LIB_DIR}/*avutil*.a"   "${FF_LIB_DIR}/*avutil*.lib"   "${FF_LIB_DIR}/*avutil*.so"   "${FF_LIB_DIR}/*avutil*.dll"   "${FF_LIB_DIR}/*avutil*.dylib")
        file(GLOB FF_SWSCALE   "${FF_LIB_DIR}/*swscale*.a"  "${FF_LIB_DIR}/*swscale*.lib"  "${FF_LIB_DIR}/*swscale*.so"  "${FF_LIB_DIR}/*swscale*.dll"  "${FF_LIB_DIR}/*swscale*.dylib")
        file(GLOB FF_SWRESAMPLE "${FF_LIB_DIR}/*swresample*.a" "${FF_LIB_DIR}/*swresample*.lib" "${FF_LIB_DIR}/*swresample*.so" "${FF_LIB_DIR}/*swresample*.dll" "${FF_LIB_DIR}/*swresample*.dylib")
        file(GLOB FF_POSTPROC  "${FF_LIB_DIR}/*postproc*.a" "${FF_LIB_DIR}/*postproc*.lib" "${FF_LIB_DIR}/*postproc*.so" "${FF_LIB_DIR}/*postproc*.dll" "${FF_LIB_DIR}/*postproc*.dylib")
        file(GLOB FF_OTHER     "${FF_LIB_DIR}/*av*.*" "${FF_LIB_DIR}/*libx264*.*" "${FF_LIB_DIR}/*libsvtav1*.*" "${FF_LIB_DIR}/*dav1d*.*")

        # build ordered list: prefer avcodec -> avformat -> avutil -> swscale -> swresample -> postproc -> others
        set(ORDERED_FF_LIBS)
        list(APPEND ORDERED_FF_LIBS ${FF_AVCODEC} ${FF_AVFORMAT} ${FF_AVUTIL} ${FF_SWSCALE} ${FF_SWRESAMPLE} ${FF_POSTPROC} ${FF_OTHER})

        # deduplicate and filter existing files
        set(DEDUP)
        foreach(f IN LISTS ORDERED_FF_LIBS)
            if (f AND NOT f STREQUAL "")
                list(FIND DEDUP "${f}" _idx)
                if (_idx EQUAL -1)
                    list(APPEND DEDUP "${f}")
                endif()
            endif()
        endforeach()

        if (DEDUP)
            message(STATUS "Linking FFmpeg libs (detected):")
            foreach(p IN LISTS DEDUP)
                message(STATUS "  ${p}")
                target_link_libraries(${PROJECT_NAME} PRIVATE "${p}")
            endforeach()
        else()
            message(WARNING "No FFmpeg libraries auto-detected in ${FF_LIB_DIR}. If your FFmpeg libs are present, consider setting FFMPEG_LIBS manually.")
        endif()
    else()
        message(WARNING "FFmpeg lib dir not found: ${FF_LIB_DIR}")
    endif()
endif()

# Link platform-specific system libs that are commonly required by static ffmpeg builds
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "Linking Windows system libs: ws2_32 bcrypt user32 gdi32 ole32")
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 bcrypt user32 gdi32 ole32)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Linking Linux system libs: pthread dl m")
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl m)
endif()

# End of CMakeLists.txt
