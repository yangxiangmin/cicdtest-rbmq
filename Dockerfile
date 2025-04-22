FROM ubuntu:20.04

# 安装运行时依赖
RUN apt-get update && apt-get install -y \
    librabbitmq4 \
    libssl1.1 \
    && rm -rf /var/lib/apt/lists/*

# 复制构建好的二进制文件
COPY build/rabbitmq_wrapper /usr/local/bin/
COPY config/rabbitmq_wrapper.conf /etc/

# 设置日志目录
RUN mkdir -p /var/log/rabbitmq_wrapper && \
    chown nobody:nogroup /var/log/rabbitmq_wrapper

# 运行服务
USER nobody
CMD ["rabbitmq_wrapper", "--config", "/etc/rabbitmq_wrapper.conf"]