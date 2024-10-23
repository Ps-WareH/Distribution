cmake_minimum_required(VERSION 3.10)
project(MapReduce)
set(CMAKE_CXX_STANDARD 17)
# 设置库和可执行文件的输出路径
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_compile_options(-w)
include_directories(include)

#the thread library of the system.
find_package(Threads REQUIRED)

#include(cmake/build.cmake)
#include(cmake/develop.cmake)
#add_library(mrFunc SHARED src/mrFunc.cpp)
# 构建主程序并链接动态库
add_executable(worker src/worker.cpp)
add_executable(master src/master.cpp)
#find cppzmq wrapper, installed by make of cppzmq
find_package(cppzmq)
#find cppzmq wrapper, installed by make of cppzmq
#find_package(libzmq)

target_link_libraries(worker mrFunc cppzmq)
target_link_libraries(master mrFunc cppzmq)


