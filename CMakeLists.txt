project(simple-ftp)
cmake_minimum_required(VERSION 3.5)

include_directories(src)
include_directories(unp)
include_directories(plog/include)
include_directories()

file(GLOB UNP_SOURCES unp/*.c)
add_library(unp ${UNP_SOURCES})
target_compile_options(unp PRIVATE -lpthread)

file(GLOB SRC_SOURCES src/*.cpp)
add_library(src ${SRC_SOURCES})
target_link_libraries(src unp)
target_compile_options(src PRIVATE --std=c++14)

add_executable(sftp apps/sftp.cpp)
target_link_libraries(sftp src)
