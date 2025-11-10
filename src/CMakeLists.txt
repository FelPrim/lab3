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
# Other dependencies
# ---------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(OpenCV REQUIRED)

# ---------------------------------------------------------------------------
# Sources & headers
# ---------------------------------------------------------------------------
set(SOURCES
    main.cpp
    mainwindow.cpp
	videocapture.cpp
	videodisplay.cpp
	videoencoder.cpp
	networkmanager.cpp
	videoselectiondialog.cpp
	videolayoutcalculator.cpp
	darktheme.cpp
	removevideodialog.cpp
	videodecoder.cpp
	networkdisplaybuffer.cpp
	framebuffer.cpp
 	simplefec.cpp
)

set(HEADERS
    video_defaults.h
    mainwindow.h
	videocapture.h
	videodisplay.h
	videoencoder.h
	networkmanager.h
	videoselectiondialog.h
	videolayoutcalculator.h
	darktheme.h
	removevideodialog.h
	videodecoder.h
	networkdisplaybuffer.h
    	simplefec.h
	framebuffer.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# ---------------------------------------------------------------------------
# FEC Library (feclib)
# ---------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Windows
    if(NOT DEFINED FEC_ROOT)
        set(FEC_ROOT "${CMAKE_SOURCE_DIR}/../deps/Windows/fec" CACHE PATH "FEC root")
    endif()
    message(STATUS "Using local FEC root: ${FEC_ROOT}")
    
    target_include_directories(${PROJECT_NAME} PRIVATE "${FEC_ROOT}/include")
    target_link_libraries(${PROJECT_NAME} PRIVATE "${FEC_ROOT}/lib/libfeclib.a")
    
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux
    if(NOT DEFINED FEC_ROOT)
        set(FEC_ROOT "${CMAKE_SOURCE_DIR}/../deps/Linux/fec" CACHE PATH "FEC root")
    endif()
    message(STATUS "Using local FEC root: ${FEC_ROOT}")
    
    target_include_directories(${PROJECT_NAME} PRIVATE "${FEC_ROOT}/include")
    target_link_libraries(${PROJECT_NAME} PRIVATE "${FEC_ROOT}/lib/libfeclib.a")
    
else()
    # macOS
    if(NOT DEFINED FEC_ROOT)
        set(FEC_ROOT "${CMAKE_SOURCE_DIR}/../deps/macOS/fec" CACHE PATH "FEC root")
    endif()
    message(STATUS "Using local FEC root: ${FEC_ROOT}")
    
    target_include_directories(${PROJECT_NAME} PRIVATE "${FEC_ROOT}/include")
    target_link_libraries(${PROJECT_NAME} PRIVATE "${FEC_ROOT}/lib/libfeclib.a")
endif()

# Link Qt + OpenCV
target_link_libraries(${PROJECT_NAME} PRIVATE 
	Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets 
    Qt6::Network 
    ${OpenCV_LIBS})

# ---------------------------------------------------------------------------
# FFmpeg
# ---------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Windows — используем локальный FFMPEG_ROOT
    if(NOT DEFINED FFMPEG_ROOT)
        set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/../deps/Windows/ffmpeg_static" CACHE PATH "FFmpeg root")
    endif()
    message(STATUS "Using local FFmpeg root: ${FFMPEG_ROOT}")

    if(EXISTS "${FFMPEG_ROOT}/lib/pkgconfig")
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            set(ENV{PKG_CONFIG_LIBDIR} "${FFMPEG_ROOT}/lib/pkgconfig")
            pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec libavutil libswscale libavformat)
            target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::AVCODEC)
        endif()
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 bcrypt user32 gdi32 ole32 z)

else()
    # Linux / macOS — используем системный FFmpeg через pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(AV REQUIRED libavcodec libavformat libavutil libswscale)

    if(AV_FOUND)
        message(STATUS "Using system FFmpeg via pkg-config")
        target_include_directories(${PROJECT_NAME} PRIVATE ${AV_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${AV_LIBRARIES})
        target_compile_options(${PROJECT_NAME} PRIVATE ${AV_CFLAGS_OTHER})
    else()
        message(FATAL_ERROR "pkg-config can't find libav* — install libav* -dev packages or set FFMPEG_ROOT")
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl m z)
endif()
