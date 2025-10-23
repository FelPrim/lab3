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
# FFmpeg include
# ---------------------------------------------------------------------------
if (EXISTS "${FFMPEG_ROOT}/include")
    target_include_directories(${PROJECT_NAME} PRIVATE "${FFMPEG_ROOT}/include")
    message(STATUS "Using FFmpeg include: ${FFMPEG_ROOT}/include")
else()
    message(WARNING "FFMPEG_ROOT/include not found: ${FFMPEG_ROOT}/include")
endif()

# ---------------------------------------------------------------------------
# FFmpeg linking (cross-platform)
# ---------------------------------------------------------------------------

# Platform-specific handling
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Windows: optionally use .pc files
    if (EXISTS "${FFMPEG_ROOT}/lib/pkgconfig")
        find_package(PkgConfig QUIET)
        if (PkgConfig_FOUND)
            set(ENV{PKG_CONFIG_LIBDIR} "${FFMPEG_ROOT}/lib/pkgconfig")
            pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec libavutil libswscale)
            target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::AVCODEC)
        else()
            message(WARNING "pkg-config not found on Windows; fallback to manual linking")
        endif()
    endif()
endif()



if (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT EXISTS "${FFMPEG_ROOT}/lib/pkgconfig")
    message(STATUS "Linux or ignoring .pc files — manual linking")

    # FFmpeg include
    target_include_directories(${PROJECT_NAME} PRIVATE "${FFMPEG_ROOT}/include")
    message(STATUS "Using FFmpeg include: ${FFMPEG_ROOT}/include")

    # Collect FFmpeg static libs
    set(FF_LIB_DIR "${FFMPEG_ROOT}/lib")
    file(GLOB FF_LIBS "${FF_LIB_DIR}/*.a")

    # External codec libraries (opus, x265, svt, dav1d)
    foreach(codec IN ITEMS opus x265 svt dav1d)
        set(CODEC_LIB_DIR "${FFMPEG_ROOT}/${codec}/lib")
        set(CODEC_INCLUDE_DIR "${FFMPEG_ROOT}/${codec}/include")
        if(EXISTS "${CODEC_LIB_DIR}")
            file(GLOB CODEC_LIBS "${CODEC_LIB_DIR}/*.a")
            list(APPEND FF_LIBS ${CODEC_LIBS})
            target_include_directories(${PROJECT_NAME} PRIVATE "${CODEC_INCLUDE_DIR}")
            message(STATUS "Added external codec include: ${CODEC_INCLUDE_DIR}")
        endif()
    endforeach()

    # Deduplicate
    list(REMOVE_DUPLICATES FF_LIBS)

    # Link all static libs first
    if(FF_LIBS)
        message(STATUS "Linking FFmpeg and external codec static libs:")
        foreach(lib IN LISTS FF_LIBS)
            message(STATUS "  ${lib}")
            target_link_libraries(${PROJECT_NAME} PRIVATE "${lib}")
        endforeach()
    else()
        message(WARNING "No FFmpeg libraries detected in ${FF_LIB_DIR} or codec folders")
    endif()

    # System libs required for static linking
    # ORDER MATTERS: libraries that others depend on should come after them
    set(FF_SYSTEM_LIBS z pthread dl m rt)
    foreach(syslib IN LISTS FF_SYSTEM_LIBS)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${syslib})
    endforeach()
endif()
