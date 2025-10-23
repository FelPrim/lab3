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

# Default FFMPEG_ROOT (can be overridden with -DFFMPEG_ROOT=...)
if (NOT DEFINED FFMPEG_ROOT)
    if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/../../deps/Windows/ffmpeg_static" CACHE PATH "Path to FFmpeg root")
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/../../deps/MacOS/ffmpeg" CACHE PATH "Path to FFmpeg root")
    else()
        set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/../../deps/Linux/ffmpeg" CACHE PATH "Path to FFmpeg root")
    endif()
endif()

message(STATUS "FFMPEG_ROOT = ${FFMPEG_ROOT}")

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED)

# --- sources / headers (add new worker files) ---
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
)

# Try pkg-config (preferred) and point it to our deps pkgconfig if present
find_package(PkgConfig QUIET)
set(FF_PKG_OK FALSE)

if(PkgConfig_FOUND)
    if(EXISTS "${FFMPEG_ROOT}/lib/pkgconfig")
        # Make pkg-config search our ffmpeg .pc files first
        set(ENV{PKG_CONFIG_LIBDIR} "${FFMPEG_ROOT}/lib/pkgconfig")
        message(STATUS "Set PKG_CONFIG_LIBDIR=${FFMPEG_ROOT}/lib/pkgconfig")
    endif()

    pkg_check_modules(AVCODEC QUIET IMPORTED_TARGET libavcodec libavutil libswscale)
    if (AVCODEC_FOUND)
        message(STATUS "Found FFmpeg via pkg-config")
        set(FF_PKG_OK TRUE)
    else()
        message(STATUS "pkg-config did not find libav* in PKG_CONFIG_LIBDIR (or pkg-config not configured).")
    endif()
else()
    message(STATUS "pkg-config not found.")
endif()

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link Qt and OpenCV first
target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt6::Widgets ${OpenCV_LIBS}
)

# If pkg-config worked — use imported target
if (FF_PKG_OK)
    target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::AVCODEC)
else()
    # Manual fallback: include and link libs from FFMPEG_ROOT
    if (EXISTS "${FFMPEG_ROOT}/include")
        target_include_directories(${PROJECT_NAME} PRIVATE "${FFMPEG_ROOT}/include")
        message(STATUS "Using FFmpeg include: ${FFMPEG_ROOT}/include")
    else()
        message(WARNING "FFMPEG include dir not found: ${FFMPEG_ROOT}/include")
    endif()

    # Prefer full-path linking: try to find libraries in ${FFMPEG_ROOT}/lib
    set(FF_LIB_DIR "${FFMPEG_ROOT}/lib")
    if (NOT EXISTS "${FF_LIB_DIR}")
        message(WARNING "FFmpeg lib dir not found: ${FF_LIB_DIR}")
    else()
        message(STATUS "Searching FFmpeg libs in: ${FF_LIB_DIR}")

        # helper function: find file by pattern (case-insensitive-ish)
        file(GLOB FFMPEG_LIB_FILES
            "${FF_LIB_DIR}/*avcodec*"
            "${FF_LIB_DIR}/*avutil*"
            "${FF_LIB_DIR}/*swscale*"
        )

        if (FFMPEG_LIB_FILES)
            foreach(libpath IN LISTS FFMPEG_LIB_FILES)
                # Only add files that look like libs (.lib, .a, .so, .dll)
                get_filename_component(ext "${libpath}" EXT)
                string(TOLOWER "${ext}" ext_lower)
                if (ext_lower STREQUAL ".a" OR ext_lower STREQUAL ".lib" OR ext_lower STREQUAL ".so" OR ext_lower STREQUAL ".dll" OR ext_lower STREQUAL ".dylib")
                    message(STATUS "Will link FFmpeg lib: ${libpath}")
                    target_link_libraries(${PROJECT_NAME} PRIVATE "${libpath}")
                endif()
            endforeach()
        else()
            message(WARNING "No FFmpeg libraries found by pattern in ${FF_LIB_DIR}. You may need to set FFMPEG_ROOT explicitly.")
        endif()

        # On some platforms link additional system libs often needed for static builds
        if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
            message(STATUS "Adding common Windows system libs for static linking")
            target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 bcrypt)
        elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
            # sometimes need pthread, dl, m, z
            target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl m)
        endif()
    endif()
endif()
