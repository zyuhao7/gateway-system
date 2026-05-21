#!/bin/bash

set -e

PORT=${1:-8080}
CONNECTIONS=${2:-1000}
DURATION=${3:-30}

echo "=== Gateway Benchmark ==="
echo "Port: $PORT"
echo "Connections: $CONNECTIONS"
echo "Duration: ${DURATION}s"
echo

# 检查服务器是否运行
if ! nc -z localhost $PORT 2>/dev/null; then
    echo "Error: Server not running on port $PORT"
    exit 1
fi

# 并发连接测试
echo "Testing concurrent connections..."
for i in $(seq 1 $CONNECTIONS); do
    (echo "PING"; sleep $DURATION; echo "QUIT") | nc localhost $PORT > /dev/null 2>&1 &
done

echo "Spawned $CONNECTIONS connections"
echo "Waiting ${DURATION}s..."
sleep $DURATION

# 统计
ACTIVE=$(netstat -an | grep ":$PORT" | grep ESTABLISHED | wc -l)
echo
echo "Active connections: $ACTIVE"  

# 清理
pkill -P $$ nc 2>/dev/null || true
wait 2>/dev/null || true

echo "Benchmark complete"
