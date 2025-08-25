cmake_minimum_required(VERSION 3.24)
project(CCTV_YOLO)

find_package(OpenCV REQUIRED)
message(STATUS "Found OpenCV version: ${OpenCV_VERSION}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
add_executable(app 
    src/main.cpp
    src/cctv.cpp
    src/getFrame.cpp
    src/inference.cpp
    src/alarm.cpp
    src/read_config.cpp
)

target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/json
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(app PRIVATE
    ${OpenCV_LIBS}
)