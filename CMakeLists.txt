cmake_minimum_required(VERSION 3.12)
project(RabbitMQWrapper)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 如果不设置CMAKE_INSTALL_PREFIX（安装目录前缀），则缺省为：/usr/local/lib
set(CMAKE_INSTALL_PREFIX "/opt/rabbitmq-test-yxm")

# 设置版本信息
configure_file(
    ${CMAKE_SOURCE_DIR}/include/version.h.in
    ${CMAKE_SOURCE_DIR}/include/version.h
)

# 查找依赖
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# 安装了 rabbitmq-c，但未生成 CMake 配置文件RabbitMQConfig.cmake
# find_package(RabbitMQ REQUIRED)
# 替换 find_package(RabbitMQ REQUIRED)

set(RABBITMQ_INCLUDE_DIR "/usr/local/include")  # 根据实际安装路径调整
set(RABBITMQ_LIBRARY "/usr/lib64/librabbitmq.so") # 或 .a 文件

# 验证路径是否存在
# if(NOT EXISTS ${RABBITMQ_INCLUDE_DIR}/amqp.h)
#     message(FATAL_ERROR "RabbitMQ headers not found in ${RABBITMQ_INCLUDE_DIR}")
# endif()

if(NOT EXISTS ${RABBITMQ_LIBRARY})
    message(FATAL_ERROR "RabbitMQ library not found at ${RABBITMQ_LIBRARY}")
endif()

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
    ${RABBITMQ_INCLUDE_DIR}
)

# 主库
add_library(rabbitmq_wrapper SHARED
    src/rabbitmqclient.cpp
    src/rmMidLib.cpp
    src/rbloger.cpp
)

target_link_libraries(rabbitmq_wrapper
    ${CMAKE_THREAD_LIBS_INIT}
    ${OPENSSL_LIBRARIES}
    ${RABBITMQ_LIBRARY}
)

# 安装规则
install(TARGETS rabbitmq_wrapper
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES include/rabbitmqclient.h
    DESTINATION include
)

# 包含测试
# 启用 CMake 的测试功能。CMake 本身并不直接运行测试，它通过其他工具（如 CTest）来管理测试。因此，调用 enable_testing() 后，CMake 会为项目启用相关的测试功能，使得你可以使用 CTest 进行测试的运行和管理。
enable_testing()
# 将 test/unit 目录添加到当前 CMake 项目的构建过程中，并在该目录下查找 CMakeLists.txt 文件。这样，test/unit 目录中的测试文件将作为子目录来构建。
add_subdirectory(test/unit)
add_subdirectory(test/integration)
#add_subdirectory(test/integration_p2p)
#add_subdirectory(test/system)