cmake_minimum_required(VERSION 3.16)
project(VideoCapture LANGUAGES CXX)

# C++ standard
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_CXX_STANDARD 23)
else()
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
endif()

# ---------------------------------------------------------------------------
# FFmpeg root (локальная сборка)
# ---------------------------------------------------------------------------
if (NOT DEFINED FFMPEG_ROOT)
    if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/../../deps/Windows/ffmpeg_static" CACHE PATH "FFmpeg root")
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/../../deps/MacOS/ffmpeg" CACHE PATH "FFmpeg root")
    else()
        set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/../../deps/Linux/ffmpeg" CACHE PATH "FFmpeg root")
    endif()
endif()
message(STATUS "FFMPEG_ROOT = ${FFMPEG_ROOT}")

# ---------------------------------------------------------------------------
# Other dependencies
# ---------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED)

# ---------------------------------------------------------------------------
# Sources & headers
# ---------------------------------------------------------------------------
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

# Link Qt + OpenCV
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets ${OpenCV_LIBS})

# ---------------------------------------------------------------------------
# FFmpeg includes
# ---------------------------------------------------------------------------
if(EXISTS "${FFMPEG_ROOT}/include")
    target_include_directories(${PROJECT_NAME} PRIVATE "${FFMPEG_ROOT}/include")
    message(STATUS "Using FFmpeg include: ${FFMPEG_ROOT}/include")
endif()

# ---------------------------------------------------------------------------
# FFmpeg & external codec linking
# ---------------------------------------------------------------------------
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")

    # FFmpeg static libs (без postproc)
    set(FF_LIB_DIR "${FFMPEG_ROOT}/lib")
    file(GLOB FF_AVCODEC   "${FF_LIB_DIR}/*avcodec*.*")
    file(GLOB FF_AVFORMAT  "${FF_LIB_DIR}/*avformat*.*")
    file(GLOB FF_AVUTIL    "${FF_LIB_DIR}/*avutil*.*")
    file(GLOB FF_SWSCALE   "${FF_LIB_DIR}/*swscale*.*")
    file(GLOB FF_SWRESAMPLE "${FF_LIB_DIR}/*swresample*.*")

    set(ORDERED_FF_LIBS ${FF_AVCODEC} ${FF_AVFORMAT} ${FF_AVUTIL} ${FF_SWSCALE} ${FF_SWRESAMPLE})

    # External codec libraries
    foreach(codec IN ITEMS x265 svtav1 dav1d opus)
        set(CODEC_LIB_DIR "${FFMPEG_ROOT}/${codec}/lib")
        set(CODEC_INCLUDE_DIR "${FFMPEG_ROOT}/${codec}/include")
        if(EXISTS "${CODEC_LIB_DIR}")
            file(GLOB LIBS "${CODEC_LIB_DIR}/*.a" "${CODEC_LIB_DIR}/*.so")
            list(APPEND ORDERED_FF_LIBS ${LIBS})
            target_include_directories(${PROJECT_NAME} PRIVATE "${CODEC_INCLUDE_DIR}")
            message(STATUS "Added external codec include: ${CODEC_INCLUDE_DIR}")
        endif()
    endforeach()

    # Deduplicate libraries
    list(REMOVE_DUPLICATES ORDERED_FF_LIBS)

    # Link all libraries
    foreach(libpath IN LISTS ORDERED_FF_LIBS)
        if(libpath AND NOT libpath STREQUAL "")
            message(STATUS "Linking library: ${libpath}")
            target_link_libraries(${PROJECT_NAME} PRIVATE "${libpath}")
        endif()
    endforeach()

    # Extra system libs
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl m z stdc++)

elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Windows — можно использовать pkg-config или локальные libs
    if(EXISTS "${FFMPEG_ROOT}/lib/pkgconfig")
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            set(ENV{PKG_CONFIG_LIBDIR} "${FFMPEG_ROOT}/lib/pkgconfig")
            pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec libavutil libswscale libavformat)
            target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::AVCODEC)
        endif()
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 bcrypt user32 gdi32 ole32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # macOS — аналогично Windows, можно линковать локальные static libs
endif()
