cmake_minimum_required(VERSION 3.24)
project(CCTV_YOLO)

find_package(OpenCV REQUIRED)
find_package(OpenSSL REQUIRED)
add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)
message(STATUS "Found OpenCV version: ${OpenCV_VERSION}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
add_executable(app 
    src/main.cpp
    src/cctv.cpp
    src/getFrame.cpp
    src/inference.cpp
    src/alarm.cpp
    src/Handler.cpp
    src/configHandler.cpp
    src/WindowHandler.cpp
    src/iniHandler.cpp
)

target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/json
    ${CMAKE_SOURCE_DIR}/lib/cpp-httplib
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(app PRIVATE
    ${OpenCV_LIBS}
    OpenSSL::SSL
    OpenSSL::Crypto
)