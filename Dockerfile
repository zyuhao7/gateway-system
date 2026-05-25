# 多阶段构建 Dockerfile
# Stage 1: 构建阶段
FROM ubuntu:24.04 AS builder

# 安装构建依赖
RUN apt-get update && apt-get install -y \
    clang-18 \
    cmake \
    ninja-build \
    libboost-all-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# 设置工作目录
WORKDIR /build

# 复制源代码
COPY . .

# 构建
RUN cmake -B build -G Ninja \
    -DCMAKE_CXX_COMPILER=clang++-18 \
    -DCMAKE_BUILD_TYPE=Release \
    && ninja -C build

# Stage 2: 运行阶段
FROM ubuntu:24.04

# 安装运行时依赖
RUN apt-get update && apt-get install -y \
    libboost-system1.83.0 \
    libboost-thread1.83.0 \
    && rm -rf /var/lib/apt/lists/*

# 创建非 root 用户
RUN useradd -m -u 1000 gateway && \
    mkdir -p /app && \
    chown -R gateway:gateway /app

WORKDIR /app

# 从构建阶段复制二进制文件
COPY --from=builder /build/build/gateway_server .

# 切换到非 root 用户
USER gateway

# 暴露端口
EXPOSE 8080 9090

# 健康检查
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:9090/metrics || exit 1

# 启动命令
ENTRYPOINT ["./gateway_server"]
CMD ["8080", "4", "9090"]
