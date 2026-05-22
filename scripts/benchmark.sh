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

# Phase 1: 基础连接测试
echo "[Phase 1] Testing PING/PONG..."
(echo "PING"; sleep 1) | nc localhost $PORT | head -1

# Phase 2: 连接管理测试
echo "[Phase 2] Testing ConnectionManager features..."

# 测试 STATS 命令
echo -n "Initial stats: "
echo "STATS" | nc localhost $PORT | head -1

# 并发连接测试
echo "[Phase 3] Spawning $CONNECTIONS concurrent connections..."
for i in $(seq 1 $CONNECTIONS); do
    (echo "PING"; sleep $DURATION; echo "QUIT") | nc localhost $PORT > /dev/null 2>&1 &
done

# 等待连接建立
sleep 2

# 查询连接统计
echo -n "Active connections (via STATS): "
echo "STATS" | nc localhost $PORT | head -1

# 测试广播功能
echo "[Phase 4] Testing BROADCAST..."
echo "BROADCAST Hello from benchmark!" | nc localhost $PORT > /dev/null 2>&1 &
sleep 1

# 持续监控
echo "[Phase 5] Monitoring for ${DURATION}s..."
for i in $(seq 1 $((DURATION / 5))); do
    sleep 5
    ACTIVE=$(netstat -an 2>/dev/null | grep ":$PORT" | grep ESTABLISHED | wc -l)
    echo "  [${i}/${DURATION/5}] Active TCP connections: $ACTIVE"
done

# 最终统计
echo
echo "=== Final Statistics ==="
echo -n "ConnectionManager stats: "
echo "STATS" | nc localhost $PORT | head -1
ACTIVE=$(netstat -an 2>/dev/null | grep ":$PORT" | grep ESTABLISHED | wc -l)
echo "TCP ESTABLISHED count: $ACTIVE"

# 清理
echo
echo "Cleaning up..."
pkill -P $$ nc 2>/dev/null || true
wait 2>/dev/null || true

echo "Benchmark complete"
