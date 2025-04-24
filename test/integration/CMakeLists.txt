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

# 添加测试
add_test(
    NAME rabbitmq_wrapper_integration_tests
    COMMAND integration_test_runner
)

# 设置测试属性
set_tests_properties(rabbitmq_wrapper_integration_tests
    PROPERTIES
    TIMEOUT 60
    LABELS "integration"
    ENVIRONMENT "RABBITMQ_HOST=localhost;RABBITMQ_PORT=5672"
)

# 复制测试配置文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/test_config.json
    ${CMAKE_CURRENT_BINARY_DIR}/test_config.json
    COPYONLY
)

# 添加依赖，确保主库先构建
add_dependencies(integration_test_runner rabbitmq_wrapper)