# 集成测试配置
cmake_minimum_required(VERSION 3.12)

# 查找 Google Test
find_package(GTest REQUIRED)

# 创建集成测试可执行文件
add_executable(integration_test_runner 
    integration_tests.cpp
)

# 链接依赖库
target_link_libraries(integration_test_runner
    PRIVATE
    rabbitmq_wrapper
    GTest::GTest
    GTest::Main
    pthread
)

# 添加依赖，确保主库先构建
add_dependencies(integration_test_runner rabbitmq_wrapper)