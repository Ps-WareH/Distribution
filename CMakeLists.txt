cmake_minimum_required(VERSION 3.10)
project(MapReduce)
set(CMAKE_CXX_STANDARD 17)
# 设置库和可执行文件的输出路径

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 构建动态库 libmrFunc.so
add_library(mrFunc SHARED src/mrFunc.cpp)

# 构建主程序并链接动态库
add_executable(worker src/worker.cpp)
add_executable(master src/master.cpp)

# 链接时指定库目录
target_link_directories(worker PRIVATE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

# 链接主程序和动态库
target_link_libraries(worker PRIVATE dl)  # Linux 上需要链接 `dl` 库
