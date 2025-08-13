cmake_minimum_required(VERSION 3.5)
project(CCTVapp)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(OpenCV REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC On)

add_executable(CCTVapp
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

target_include_directories(CCTVapp PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(CCTVapp
    Qt5::Widgets
    ${OpenCV_LIBS}
)