# 第二阶段：运行阶段，使用 ol_base 镜像来运行程序
FROM dockhub.ghtchina.com:6060/ims/arm/ol/base as ght_runner

# 设置工作目录
WORKDIR /usr/workspace

# system_test_runner 到目标镜像的 工作目录
COPY build/test/system/system_test_runner rbtest ./

# 运行 system_test_runner
# CMD ["./system_test_runner"]
ENTRYPOINT ["./system_test_runner"]
