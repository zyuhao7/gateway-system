#!/bin/bash

set -e

echo "=== Gateway System Deployment ==="
echo

# 检查 Docker
if ! command -v docker &> /dev/null; then
    echo "Error: Docker not installed"
    exit 1
fi

# 检查 Docker Compose
if ! command -v docker-compose &> /dev/null; then
    echo "Error: Docker Compose not installed"
    exit 1
fi

# 构建镜像
echo "[1/4] Building Docker image..."
docker build -t gateway-system:latest .

# 启动服务
echo "[2/4] Starting services..."
docker-compose up -d

# 等待服务启动
echo "[3/4] Waiting for services to be ready..."
sleep 10

# 健康检查
echo "[4/4] Health check..."
for port in 9091 9092 9093; do
    if curl -sf http://localhost:$port/metrics > /dev/null; then
        echo "  ✓ Gateway on port $port is healthy"
    else
        echo "  ✗ Gateway on port $port is not responding"
    fi
done

echo
echo "=== Deployment Complete ==="
echo
echo "Services:"
echo "  Gateway 1: http://localhost:8081 (metrics: http://localhost:9091/metrics)"
echo "  Gateway 2: http://localhost:8082 (metrics: http://localhost:9092/metrics)"
echo "  Gateway 3: http://localhost:8083 (metrics: http://localhost:9093/metrics)"
echo "  Redis:     localhost:6379"
echo "  Prometheus: http://localhost:9090"
echo "  Grafana:   http://localhost:3000 (admin/admin)"
echo
echo "To view logs:"
echo "  docker-compose logs -f gateway-1"
echo
echo "To stop:"
echo "  docker-compose down"
