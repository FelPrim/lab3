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


# ---------------------------------------------------------------------------
# Linux: ручная линковка статического FFmpeg и зависимостей
# ---------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT EXISTS "${FFMPEG_ROOT}/lib/pkgconfig")
    message(STATUS "Linux — manual linking of FFmpeg and external codecs")

    # Добавляем include директории FFmpeg
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${FFMPEG_ROOT}/include"
        "${FFMPEG_ROOT}/dav1d/include"
        "${FFMPEG_ROOT}/opus/include"
        "${FFMPEG_ROOT}/svtav1/include"
        "${FFMPEG_ROOT}/x265/include"
    )
    message(STATUS "Added FFmpeg and codec includes")

    # Собираем список статических библиотек FFmpeg (убрал libpostproc.a)
    set(FF_LIBS
        "${FFMPEG_ROOT}/lib/libavcodec.a"
        "${FFMPEG_ROOT}/lib/libavformat.a"
        "${FFMPEG_ROOT}/lib/libavutil.a"
        "${FFMPEG_ROOT}/lib/libswscale.a"
        "${FFMPEG_ROOT}/lib/libswresample.a"
    )

    # Внешние кодеки
    list(APPEND FF_LIBS
        "${FFMPEG_ROOT}/dav1d/lib/libdav1d.a"
        "${FFMPEG_ROOT}/opus/lib/libopus.a"
        "${FFMPEG_ROOT}/svtav1/lib/libSvtAv1Enc.a"
        "${FFMPEG_ROOT}/x265/lib/libx265.a"
    )

    # Линкуем все библиотеки
    target_link_libraries(${PROJECT_NAME} PRIVATE ${FF_LIBS})

    # Добавляем системные библиотеки для статической линковки
    target_link_libraries(${PROJECT_NAME} PRIVATE
        pthread
        dl
        m
        z
    )

    message(STATUS "Linked FFmpeg, external codecs and system libraries")
endif()
