cmake_minimum_required(VERSION 3.24)
project(CCTV_YOLO)

find_package(OpenCV REQUIRED)

add_executable(test1 test.cpp)

target_include_directories(test1 PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test1 PRIVATE ${OpenCV_INCLUDE_DIRS})
