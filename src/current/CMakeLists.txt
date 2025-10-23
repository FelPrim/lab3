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

# ---------- FFmpeg linking (cross-platform, explicit external deps handling) ----------
# Предполагается, что FFMPEG_ROOT уже задан (как вы делали ранее)

# Allow user to provide extra libs explicitly (full names without -l or full paths)
# Example: -DFFMPEG_EXTRA_LIBS="opus;z;pthread"
if(NOT DEFINED FFMPEG_EXTRA_LIBS)
    set(FFMPEG_EXTRA_LIBS "" CACHE STRING "Semicolon-separated list of extra libs required by static FFmpeg (e.g. opus;z;pthread).")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "Platform: Windows — will try using .pc files in ${FFMPEG_ROOT}/lib/pkgconfig if present, otherwise manual linking")
    if (EXISTS "${FFMPEG_ROOT}/lib/pkgconfig")
        # Use pkg-config files on Windows if you want
        find_package(PkgConfig QUIET)
        if (PkgConfig_FOUND)
            set(ENV{PKG_CONFIG_LIBDIR} "${FFMPEG_ROOT}/lib/pkgconfig")
            pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec libavutil libswscale)
            target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::AVCODEC)
        else()
            message(WARNING "pkg-config not found on Windows; using manual linking fallback.")
            # fallthrough to manual linking below
        endif()
    endif()
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT EXISTS "${FFMPEG_ROOT}/lib/pkgconfig")
    message(STATUS "Platform: Linux or ignoring .pc — try pkg-config (static) and manual fallback if needed")

    # set PKG_CONFIG_PATH to local FFmpeg pkgconfig dir so pkg-config can find .pc if present
    if (EXISTS "${FFMPEG_ROOT}/lib/pkgconfig")
        set(ENV{PKG_CONFIG_LIBDIR} "${FFMPEG_ROOT}/lib/pkgconfig")
        message(STATUS "PKG_CONFIG_LIBDIR=${FFMPEG_ROOT}/lib/pkgconfig")
    endif()

    # Try to get static link flags via pkg-config (this may list transitive deps)
    find_package(PkgConfig QUIET)
    if (PkgConfig_FOUND)
        # Try to find libavcodec with static flags (pkg-config should be configured to search our .pc)
        # Note: pkg_check_modules in CMake doesn't expose --static flag easily; but if PKG_CONFIG_LIBDIR points to your static .pc which contain proper Libs.private, this will pull transitive deps.
        pkg_check_modules(AVCODEC QUIET IMPORTED_TARGET libavcodec libavutil libswscale)
        if (AVCODEC_FOUND)
            message(STATUS "Found FFmpeg via pkg-config (may include transitive libs). Linking PkgConfig::AVCODEC")
            target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::AVCODEC)
        else()
            message(STATUS "pkg-config did not find libav* (or .pc missing). Will attempt manual linking from ${FFMPEG_ROOT}/lib")
        endif()
    else()
        message(STATUS "pkg-config not available on system; will use manual linking fallback.")
    endif()

    # Manual fallback: add include and link libs found in FFMPEG_ROOT/lib
    if (EXISTS "${FFMPEG_ROOT}/include")
        target_include_directories(${PROJECT_NAME} PRIVATE "${FFMPEG_ROOT}/include")
        message(STATUS "Added FFmpeg include: ${FFMPEG_ROOT}/include")
    endif()

    set(FF_LIB_DIR "${FFMPEG_ROOT}/lib")
    if (EXISTS "${FF_LIB_DIR}")
        # Collect known ffmpeg libs (prefer exact candidates)
        file(GLOB FF_AVCODEC   "${FF_LIB_DIR}/*avcodec*.*")
        file(GLOB FF_AVFORMAT  "${FF_LIB_DIR}/*avformat*.*")
        file(GLOB FF_AVUTIL    "${FF_LIB_DIR}/*avutil*.*")
        file(GLOB FF_SWSCALE   "${FF_LIB_DIR}/*swscale*.*")
        file(GLOB FF_SWRESAMPLE "${FF_LIB_DIR}/*swresample*.*")
        file(GLOB FF_POSTPROC  "${FF_LIB_DIR}/*postproc*.*")
        # optional: libx264, libopus, libvpx etc. we will try to find them
        file(GLOB FF_X264 "${FF_LIB_DIR}/*x264*.*")
        file(GLOB FF_OPUS "${FF_LIB_DIR}/*opus*.*")

        set(ORDERED_FF_LIBS)
        list(APPEND ORDERED_FF_LIBS ${FF_AVCODEC} ${FF_AVFORMAT} ${FF_AVUTIL} ${FF_SWSCALE} ${FF_SWRESAMPLE} ${FF_POSTPROC})
        # append codec externals if present (e.g. libx264, opus)
        list(APPEND ORDERED_FF_LIBS ${FF_X264} ${FF_OPUS})

        # deduplicate and add
        set(DEDUP)
        foreach(p IN LISTS ORDERED_FF_LIBS)
            if(p AND NOT p STREQUAL "")
                list(FIND DEDUP "${p}" _idx)
                if(_idx EQUAL -1)
                    list(APPEND DEDUP "${p}")
                endif()
            endif()
        endforeach()

        if (DEDUP)
            message(STATUS "Linking detected FFmpeg libs from ${FF_LIB_DIR}:")
            foreach(libpath IN LISTS DEDUP)
                message(STATUS "  ${libpath}")
                target_link_libraries(${PROJECT_NAME} PRIVATE "${libpath}")
            endforeach()
        else()
            message(WARNING "No ffmpeg libraries auto-detected in ${FF_LIB_DIR}")
        endif()
    else()
        message(WARNING "FFMPEG lib dir not found: ${FF_LIB_DIR}")
    endif()

    # Now ensure we link extra external libs that ffmpeg may depend on (e.g. opus).
    # Try pkg-config for opus first, otherwise try to find library in FFMPEG_ROOT/lib or system paths.
    find_package(PkgConfig QUIET)
    if (PkgConfig_FOUND)
        pkg_check_modules(OPUS_PKG QUIET opus)
        if (OPUS_PKG_FOUND)
            message(STATUS "Found opus via pkg-config; linking it")
            target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::OPUS_PKG)
        endif()
    endif()

    # Manual search for libopus (in FFMPEG_ROOT/lib or system)
    if (NOT OPUS_PKG_FOUND)
        find_library(OPUS_LIB NAMES opus PATHS "${FF_LIB_DIR}" "/usr/lib" "/usr/local/lib" NO_DEFAULT_PATH)
        if (NOT OPUS_LIB)
            # try default system paths
            find_library(OPUS_LIB NAMES opus PATHS "/usr/lib" "/usr/local/lib")
        endif()
        if (OPUS_LIB)
            message(STATUS "Found libopus: ${OPUS_LIB} -> linking")
            target_link_libraries(${PROJECT_NAME} PRIVATE "${OPUS_LIB}")
        else()
            if (FFMPEG_EXTRA_LIBS)
                message(STATUS "User requested extra libs: ${FFMPEG_EXTRA_LIBS}; linking them")
                foreach(x IN LISTS FFMPEG_EXTRA_LIBS)
                    target_link_libraries(${PROJECT_NAME} PRIVATE "${x}")
                endforeach()
            else()
                message(WARNING "libopus not found automatically. If your ffmpeg static build depends on opus, set -DFFMPEG_EXTRA_LIBS=opus or provide full paths.")
            endif()
        endif()
    endif()

    # Finally add platform system libs often required for static builds
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl m)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
        target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 bcrypt user32 gdi32 ole32)
    endif()
endif()
# ---------------------------------------------------------------------------
