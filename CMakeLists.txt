# set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/Arduino-CMake-Toolchain/Arduino-toolchain.cmake)

message("SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")

cmake_minimum_required(VERSION 3.20.0)
project(5FX-PedalBoard VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(HEADERS
    src/datastore.hpp
    src/datastore.m.hpp
)
set(SOURCES
    src/pdb.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
# target_arduino_link_libraries(${PROJECT_NAME} PRIVATE CORE)
# target_enable_arduino_upload(${PROJECT_NAME})