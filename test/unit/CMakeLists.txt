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

# 添加测试
add_test(
    NAME rabbitmq_wrapper_unit_tests
    COMMAND test_rabbitmq_wrapper
)

# 设置测试属性
set_tests_properties(rabbitmq_wrapper_unit_tests
    PROPERTIES
    TIMEOUT 30
    LABELS "unit"
)

# 启用测试覆盖率收集 (如果启用)
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    target_compile_options(test_rabbitmq_wrapper
        PRIVATE --coverage
    )
    target_link_libraries(test_rabbitmq_wrapper
        PRIVATE --coverage
    )
endif()