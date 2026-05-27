#!/bin/bash
# 百万连接压测脚本

set -e

echo "=== Gateway Million Connection Stress Test ==="
echo ""

# 检查系统限制
echo "Checking system limits..."
echo "Current ulimit -n: $(ulimit -n)"
echo "Recommended: ulimit -n 1048576"
echo ""

# 检查服务器是否运行
if ! pgrep -x "gateway_server" > /dev/null; then
    echo "Error: gateway_server is not running"
    echo "Please start the server first:"
    echo "  cd build && ./gateway_server"
    exit 1
fi

echo "Gateway server is running"
echo ""

# 测试配置
HOST="localhost"
PORT="8080"

# 测试场景
declare -a SCENARIOS=(
    "1000:Warmup (1K connections)"
    "10000:Small scale (10K connections)"
    "50000:Medium scale (50K connections)"
    "100000:Large scale (100K connections)"
    "500000:Massive scale (500K connections)"
    "1000000:Million connections"
)

# 运行测试
for scenario in "${SCENARIOS[@]}"; do
    CONNECTIONS="${scenario%%:*}"
    DESCRIPTION="${scenario#*:}"

    echo "=========================================="
    echo "Test: $DESCRIPTION"
    echo "=========================================="
    echo ""

    # 运行压测客户端
    ./build/stress_test_client "$HOST" "$PORT" "$CONNECTIONS" &
    CLIENT_PID=$!

    # 等待测试完成或超时
    TIMEOUT=300  # 5分钟超时

    if timeout "$TIMEOUT" tail --pid=$CLIENT_PID -f /dev/null; then
        echo "Test completed successfully"
    else
        echo "Test timeout or failed"
        kill -9 $CLIENT_PID 2>/dev/null || true
    fi

    echo ""
    echo "Waiting 30 seconds before next test..."
    sleep 30

    # 检查服务器指标
    echo "Fetching server metrics..."
    curl -s http://localhost:9090/metrics | grep -E "gateway_(connections|messages|buffer_pool)" || true
    echo ""
done

echo "=========================================="
echo "All tests completed"
echo "=========================================="
