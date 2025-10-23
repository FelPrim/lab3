cmake_minimum_required(VERSION 3.16)
project(VideoCapture LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED)

set(SOURCES
    main.cpp
    mainwindow.cpp
    capturethread.cpp
)

set(HEADERS
    mainwindow.h
    capturethread.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt6::Widgets ${OpenCV_LIBS}
)
