#!/bin/bash

# 启动测试RabbitMQ服务器
docker run -d --name rabbitmq-test -p 5672:5672 rabbitmq:3-management
sleep 30

# 运行测试
./build/test/unit/test_rabbitmq_wrapper
./build/test/integration/integration_test_runner

# 清理
docker stop rabbitmq-test && docker rm rabbitmq-test