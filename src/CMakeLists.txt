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
# Dependencies
# ---------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Test)
find_package(OpenCV REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O1")

# ---------------------------------------------------------------------------
# Sources & headers (updated for new structure)
# ---------------------------------------------------------------------------
set(SOURCES
    main.cpp
    
    # Logic files
    logic/videocapture.cpp
    logic/videodisplay.cpp
    logic/videoencoder.cpp
    logic/videodecoder.cpp
    logic/framebuffer.cpp
    logic/bufferedvideodecoder.cpp
    
    # Network files
    network/udp/networkmanager.cpp
   # network/udp/networkdisplaybuffer.cpp
    network/networkfacade.cpp
    network/streammanager.cpp
    network/handshake.cpp
    #  network/tcp/tcpclient.cpp
    network/tcp/tcpmanager.cpp
    network/udp/udpmanager.cpp
    network/udp/fecbuffer.cpp
    network/udp/packetgroupbuffer.cpp
    network/udp/frameprocessor.cpp
    
    # UI files
    ui/mainwindow.cpp
    ui/videoselectiondialog.cpp
    ui/darktheme.cpp
    
    ui/streamcontrolpanel.cpp
    ui/streamidinputwidget.cpp
    ui/deviceselectorwidget.cpp

    ui/conferencewindow.cpp
    ui/streamerwidget.cpp
    ui/viewerwidget.cpp  
    ui/videogridwidget.cpp
    ui/streamselectorwidget.cpp
    ui/conferencecontrolpanel.cpp
    ui/maincontrolpanel.cpp
    
)

set(HEADERS
    video_defaults.h
    
    # Logic headers
    logic/videocapture.h
    logic/videodisplay.h
    logic/videoencoder.h
    logic/videodecoder.h
    logic/framebuffer.h
    logic/bufferedvideodecoder.h

    # Network headers
    network/udp/networkmanager.h
 #   network/udp/networkdisplaybuffer.h
    network/networkfacade.h
    network/streammanager.h
    network/handshake.h
    #    network/tcp/tcpclient.h
    network/tcp/tcpmanager.h
    network/udp/udpmanager.h
    network/udp/frameprocessor.h
    network/udp/network_packet.h
    network/endianness.h
    network/udp/fecbuffer.h
    network/udp/packetgroupbuffer.h
    
    # UI headers
    ui/mainwindow.h
    ui/videoselectiondialog.h
    ui/darktheme.h
    
   
    ui/streamcontrolpanel.h
    ui/streamidinputwidget.h
    ui/deviceselectorwidget.h
    
    ui/id_utils.h

    ui/conferencewindow.h
    ui/streamerwidget.h
    ui/viewerwidget.h
    ui/videogridwidget.h
    ui/streamselectorwidget.h
    ui/conferencecontrolpanel.h
    ui/maincontrolpanel.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/logic
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}/network/tcp
    ${CMAKE_CURRENT_SOURCE_DIR}/network/udp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

# ---------------------------------------------------------------------------
# FEC Library (feclib) - from original version
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

# ---------------------------------------------------------------------------
# FFmpeg - from original version
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

# ---------------------------------------------------------------------------
# Link dependencies (combined)
# ---------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets 
    Qt6::Network 
    Qt6::Test
    ${OpenCV_LIBS})

# Debug info
message(STATUS "Source files: ${SOURCES}")
message(STATUS "Header files: ${HEADERS}")
