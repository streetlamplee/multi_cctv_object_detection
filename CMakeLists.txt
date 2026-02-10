cmake_minimum_required(VERSION 3.18)
project(CCTV_YOLO)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(OpenCV REQUIRED)
find_package(PahoMqttCpp REQUIRED)
find_package(nlohmann_json REQUIRED)
pkg_check_modules(MODBUS REQUIRED libmodbus)

add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)
message(STATUS "Found OpenCV version: ${OpenCV_VERSION}")

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -g")
add_executable(app 
    src/main.cpp
    src/cctv/cctv.cpp
    src/cctv/getFrame.cpp
    src/inference/inference.cpp
    src/alarm/alarm.cpp
    src/config/Handler.cpp
    src/config/configHandler.cpp
    # src/WindowHandler.cpp
    # src/config/iniHandler.cpp
    src/alarm/alarmManager.cpp
    src/_modbus/modbus_handler.c
    src/config/ini.c
    src/config/INIReader.cpp
    src/mqttManager/mqttManager.cpp
    src/server/server.cpp
    # src/inference/rknn.cpp
)

target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/json
    ${CMAKE_SOURCE_DIR}/lib/cpp-httplib
    ${CMAKE_SOURCE_DIR}/sysroot
    ${OpenCV_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    /usr/local/include
)

target_link_libraries(app PRIVATE
    ${OpenCV_LIBS}
    ${OPENSSL_LIBRARIES}
    ${MODBUS_STATIC_LIBRARIES}
    Threads::Threads
    PahoMqttCpp::paho-mqttpp3
    nlohmann_json::nlohmann_json
    rknnrt
)
