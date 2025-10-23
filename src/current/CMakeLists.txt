cmake_minimum_required(VERSION 3.16)
project(VideoCapture LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
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

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
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
else()
     # Не линкуем локальные статические ffmpeg, используем системные (pkg-config)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(AV REQUIRED libavcodec libavformat libavutil libswscale)

    if (AV_FOUND)
        message(STATUS "Using system FFmpeg via pkg-config")
        target_include_directories(${PROJECT_NAME} PRIVATE ${AV_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${AV_LIBRARIES})
        # гарантируем, что флаги компиляции pkg-config тоже пропадут в цели
        target_compile_options(${PROJECT_NAME} PRIVATE ${AV_CFLAGS_OTHER})
    else()
        message(FATAL_ERROR "pkg-config can't find libav* — install libav* -dev packages or set FFMPEG_ROOT")
    endif()

    # лишние системные либы (если нужно)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl m z)
endif()
