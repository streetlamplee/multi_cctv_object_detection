cmake_minimum_required(VERSION 3.10)
project(cvui_project)

find_package(OpenCV REQUIRED)

add_executable(cctv main.cpp)
target_include_directories(cctv PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(cctv PRIVATE ${OpenCV_LIBS})
