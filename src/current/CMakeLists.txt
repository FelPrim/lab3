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

# ----- FFMPEG_ROOT (can override with -DFFMPEG_ROOT=...) -----
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

# source files
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

# Try pkg-config (preferred), but allow fallback to manual linking
find_package(PkgConfig QUIET)
set(USE_PKGCONFIG OFF)
if (PkgConfig_FOUND)
    if (EXISTS "${FFMPEG_ROOT}/lib/pkgconfig")
        # prefer local pkgconfig dir
        set(ENV{PKG_CONFIG_LIBDIR} "${FFMPEG_ROOT}/lib/pkgconfig")
        message(STATUS "PKG_CONFIG_LIBDIR set to ${FFMPEG_ROOT}/lib/pkgconfig")
    endif()

    # Try to find avcodec+avutil+swscale (quiet)
    pkg_check_modules(AVCODEC QUIET IMPORTED_TARGET libavcodec libavutil libswscale)
    if (AVCODEC_FOUND)
        message(STATUS "Found FFmpeg via pkg-config")
        set(USE_PKGCONFIG ON)
    else()
        message(STATUS "pkg-config did not find required FFmpeg modules")
    endif()
else()
    message(STATUS "pkg-config not found, will use manual linking fallback")
endif()

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Always link Qt and OpenCV first
target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt6::Widgets ${OpenCV_LIBS}
)

if (USE_PKGCONFIG)
    # Use imported target from pkg-config which carries include/link flags
    target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::AVCODEC)
else()
    # Manual fallback: include and link libs from FFMPEG_ROOT
    if (EXISTS "${FFMPEG_ROOT}/include")
        target_include_directories(${PROJECT_NAME} PRIVATE "${FFMPEG_ROOT}/include")
        message(STATUS "Using FFmpeg include: ${FFMPEG_ROOT}/include")
    else()
        message(WARNING "FFMPEG include dir not found: ${FFMPEG_ROOT}/include")
    endif()

    set(FF_LIB_DIR "${FFMPEG_ROOT}/lib")
    if (EXISTS "${FF_LIB_DIR}")
        message(STATUS "Searching FFmpeg libs in: ${FF_LIB_DIR}")

        # Prefer explicit list/order for link-time stability (important for static linking)
        # Try to find exact files (libavcodec.*, libavutil.*, libswscale.*). If not found, fallback to glob.
        set(FF_LIBS_TO_FIND avcodec avutil swscale avformat swresample postproc)
        set(FOUND_FF_LIB_PATHS)

        foreach(name IN LISTS FF_LIBS_TO_FIND)
            # match common patterns
            file(GLOB lib_matches
                "${FF_LIB_DIR}/*${name}*.a"
                "${FF_LIB_DIR}/*${name}*.lib"
                "${FF_LIB_DIR}/*${name}*.so"
                "${FF_LIB_DIR}/*${name}*.dll"
                "${FF_LIB_DIR}/*${name}*.dylib"
            )
            if (lib_matches)
                list(APPEND FOUND_FF_LIB_PATHS ${lib_matches})
            else()
                message(STATUS "Library ${name} not found explicitly in ${FF_LIB_DIR} (this may be ok if not required).")
            endif()
        endforeach()

        # If we didn't find anything by name, try generic glob as last resort
        if (NOT FOUND_FF_LIB_PATHS)
            file(GLOB FFMPEG_LIB_FILES
                "${FF_LIB_DIR}/*avcodec*"
                "${FF_LIB_DIR}/*avutil*"
                "${FF_LIB_DIR}/*swscale*"
            )
            list(APPEND FOUND_FF_LIB_PATHS ${FFMPEG_LIB_FILES})
        endif()

        if (FOUND_FF_LIB_PATHS)
            # Link in a deterministic order: avcodec -> avformat -> avutil -> swscale -> others
            # We'll build ordered list by matching keywords
            set(ORDERED_LIBS)
            foreach(pat IN ITEMS avcodec avformat avutil swscale swresample postproc)
                foreach(path IN LISTS FOUND_FF_LIB_PATHS)
                    string(REGEX MATCH "${pat}" _match "${path}")
                    if (_match)
                        list(APPEND ORDERED_LIBS ${path})
                    endif()
                endforeach()
            endforeach()
            # Append any remaining
            foreach(path IN LISTS FOUND_FF_LIB_PATHS)
                list(FIND ORDERED_LIBS ${path} _idx)
                if (_idx EQUAL -1)
                    list(APPEND ORDERED_LIBS ${path})
                endif()
            endforeach()

            message(STATUS "Linking FFmpeg libs (manual):")
            foreach(p IN LISTS ORDERED_LIBS)
                message(STATUS "  ${p}")
                target_link_libraries(${PROJECT_NAME} PRIVATE "${p}")
            endforeach()
        else()
            message(WARNING "No ffmpeg libraries found in ${FF_LIB_DIR}. You may need to set FFMPEG_ROOT explicitly to correct location.")
        endif()

        # Add platform-specific system libs often required for static FFmpeg on Windows/Linux
        if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
            message(STATUS "Linking Windows system libs for FFmpeg static: ws2_32;bcrypt;user32;gdi32;ole32")
            target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 bcrypt user32 gdi32 ole32)
        elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
            message(STATUS "Linking Linux system libs: pthread;dl;m")
            target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl m)
        endif()
    else()
        message(WARNING "FFmpeg lib dir not found: ${FF_LIB_DIR}")
    endif()
endif()

# End of CMakeLists.txt
