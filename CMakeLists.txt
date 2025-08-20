cmake_minimum_required(VERSION 3.24)
project(CCTV_YOLO)

find_package(OpenCV REQUIRED)
message(STATUS "Found OpenCV version: ${OpenCV_VERSION}")
add_executable(app 
    src/main.cpp
    src/cctv.cpp
    src/getFrame.cpp
    src/inference.cpp
)

target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(app PRIVATE
    ${OpenCV_LIBS}
)