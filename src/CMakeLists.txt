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

# ---------------------------------------------------------------------------
# Test executables (commented out as in original)
# ---------------------------------------------------------------------------
#enable_testing()
#qt_add_executable(test_myfec test_myfec.cpp myfec.cpp)
#qt_add_executable(test_packet_format test_packet_format.cpp myfec.cpp)
#target_link_libraries(test_myfec PRIVATE Qt6::Test Qt6::Core ${OpenCV_LIBS})
#target_link_libraries(test_packet_format PRIVATE Qt6::Test Qt6::Core ${OpenCV_LIBS})
#target_include_directories(test_myfec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
#target_include_directories(test_packet_format PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
#add_test(NAME test_myfec COMMAND test_myfec)
#add_test(NAME test_packet_format COMMAND test_packet_format)

# ---------------------------------------------------------------------------
# Sources & headers (combined from old and new)
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
    myfec.cpp
    
    new/tcpclient.cpp

    # New GUI files
    new/streammanager.cpp
    new/streampublisherwindow.cpp
    new/streamviewerwindow.cpp
    new/streamcontrolpanel.cpp
    new/streamvideodisplaypanel.cpp
    new/streamidinputwidget.cpp
    new/deviceselectorwidget.cpp
    new/streamstatswidget.cpp
    new/streamwindow.cpp
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
    framebuffer.h
    myfec.h
    network_packet.h
    
    # New GUI headers
    new/streammanager.h
    new/streampublisherwindow.h
    new/streamviewerwindow.h
    new/streamcontrolpanel.h
    new/streamvideodisplaypanel.h
    new/streamidinputwidget.h
    new/deviceselectorwidget.h
    new/streamstatswidget.h
    new/tcpclient.h
    new/streamwindow.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

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
