#!/bin/bash

# 定义项目根目录和构建目录路径
PROJECT_ROOT_DIR=$(dirname "$0")
BUILD_DIR="$PROJECT_ROOT_DIR/build"

# 检测并删除旧的构建目录
if [ -d "$BUILD_DIR" ]; then
    echo "Removing existing build directory..."
    rm -rf "$BUILD_DIR"
fi

# 创建新的构建目录
echo "Creating new build directory..."
mkdir "$BUILD_DIR"
cd "$BUILD_DIR"

# 运行 CMake 配置
echo "Configuring project with CMake..."
cmake ..

# 构建项目
echo "Building project..."
make

# 可以选择在这里运行可执行文件
# echo "Running executables..."
# ./bin/worker
# ./bin/master

echo "Build completed successfully."
