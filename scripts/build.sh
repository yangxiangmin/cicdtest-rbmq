#!/bin/bash

# 创建构建目录
mkdir -p build
cd build

# 运行CMake和Make
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# 运行测试
ctest --output-on-failure

# 打包
cpack -G DEB