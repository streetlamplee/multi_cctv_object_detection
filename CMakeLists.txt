cmake_minimum_required(VERSION 3.18)
project(CCTV_YOLO)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(OpenCV REQUIRED)
find_package(PahoMqttCpp REQUIRED)
pkg_check_modules(MODBUS REQUIRED libmodbus)

add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)
message(STATUS "Found OpenCV version: ${OpenCV_VERSION}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
add_executable(app 
    src/main.cpp
    src/cctv/cctv.cpp
    src/cctv/getFrame.cpp
    src/inference/inference.cpp
    src/alarm/alarm.cpp
    src/config/Handler.cpp
    src/config/configHandler.cpp
    # src/WindowHandler.cpp
    src/config/iniHandler.cpp
    src/alarm/alarmManager.cpp
    src/_modbus/modbus_handler.c
    src/config/ini.c
)

target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/json
    ${CMAKE_SOURCE_DIR}/lib/cpp-httplib
    ${OpenCV_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
)

target_link_libraries(app PRIVATE
    ${OpenCV_LIBS}
    ${OPENSSL_LIBRARIES}
    ${MODBUS_STATIC_LIBRARIES}
    Threads::Threads
    PahoMqttCpp::paho-mqttpp3
)
