#!/bin/bash

VERSION=$(git describe --tags --always)

# 构建Docker镜像
docker build -t rabbitmq-wrapper:$VERSION .

# 推送镜像到仓库
docker tag rabbitmq-wrapper:$VERSION your-registry/rabbitmq-wrapper:$VERSION
docker push your-registry/rabbitmq-wrapper:$VERSION

# 部署到Kubernetes
kubectl set image deployment/rabbitmq-wrapper rabbitmq-wrapper=your-registry/rabbitmq-wrapper:$VERSION