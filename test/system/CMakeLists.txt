# 系统测试配置
cmake_minimum_required(VERSION 3.12)

# 查找 Google Test 和 CURL
find_package(GTest REQUIRED)
find_package(CURL REQUIRED)
find_package(Jsoncpp REQUIRED)

# 创建系统测试可执行文件
add_executable(system_test_runner 
    system_tests.cpp
)

# 链接依赖库
target_link_libraries(system_test_runner
    PRIVATE
    rabbitmq_wrapper
    GTest::GTest
    GTest::Main
    CURL::libcurl       # 替换 ${CURL_LIBRARIES}
    JsonCpp::JsonCpp
    pthread
)

# 包含目录
target_include_directories(system_test_runner
    PRIVATE
    ${CURL_INCLUDE_DIRS}
)

# 添加测试
add_test(
    NAME rabbitmq_wrapper_system_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/system_test_runner
)

# 设置测试属性
set_tests_properties(rabbitmq_wrapper_system_tests
    PROPERTIES
    TIMEOUT 120
    LABELS "system"
    REQUIRED_FILES "${CMAKE_CURRENT_BINARY_DIR}/system_test_runner"
)

# 添加依赖，确保主库和集成测试先构建
add_dependencies(system_test_runner rabbitmq_wrapper)

# 复制 docker-compose 文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/docker-compose.yml
    ${CMAKE_CURRENT_BINARY_DIR}/docker-compose.yml
    COPYONLY
)

# 复制测试配置文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/system_test_config.json
    ${CMAKE_CURRENT_BINARY_DIR}/system_test_config.json
    COPYONLY
)