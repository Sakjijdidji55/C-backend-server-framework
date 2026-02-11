# C++ HTTP Server - 多阶段构建：编译 + 运行
#
# 构建: docker build -t cpp-server .
# 运行: docker run -p 8080:8080 cpp-server
# 访问: http://localhost:8080
#
# 阶段一：编译环境，安装依赖并构建
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# 安装构建依赖（自动下载）
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    build-essential \
    cmake \
    libmysqlclient-dev \
    libssl-dev \
    zlib1g-dev \
    libhiredis-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# 复制源码（.dockerignore 会排除 build、.git 等）
COPY . .

# 配置并编译（Linux 使用系统库：mysqlclient、hiredis、OpenSSL、Zlib）
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j$(nproc)

# 阶段二：运行环境，仅保留运行时依赖与可执行文件
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# 仅安装运行时依赖（与 builder 中库版本一致）
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libmysqlclient21 \
    libssl3 \
    zlib1g \
    libhiredis0.14 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# 从 builder 复制编译好的可执行文件
COPY --from=builder /app/build/bin/CBSF /app/CBSF

# 服务监听端口
EXPOSE 8080

# 启动服务
CMD ["./CBSF"]
