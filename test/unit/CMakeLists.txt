# 单元测试配置
cmake_minimum_required(VERSION 3.12)

# 查找 Google Test
find_package(GTest REQUIRED)

# 创建单元测试可执行文件
add_executable(test_rabbitmq_wrapper 
    test_rabbitmq_wrapper.cpp
)

# 链接依赖库
target_link_libraries(test_rabbitmq_wrapper
    PRIVATE
    rabbitmq_wrapper
    GTest::GTest
    GTest::Main
    pthread
)

# 添加依赖，确保主库先构建
add_dependencies(test_rabbitmq_wrapper rabbitmq_wrapper)