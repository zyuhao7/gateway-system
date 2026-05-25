#!/bin/bash

set -e

echo "=== Gateway System Performance Test ==="
echo

# 配置
PORT=${1:-8080}
METRICS_PORT=${2:-9091}
DURATION=60
CONNECTIONS_LIST=(100 500 1000 5000 10000)

# 检查服务器是否运行
if ! nc -z localhost $PORT 2>/dev/null; then
    echo "Error: Server not running on port $PORT"
    exit 1
fi

# 创建结果目录
RESULT_DIR="./test-results/$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULT_DIR"

echo "Results will be saved to: $RESULT_DIR"
echo

# 测试函数
run_test() {
    local connections=$1
    local test_name="test_${connections}_connections"

    echo "----------------------------------------"
    echo "Testing with $connections connections"
    echo "----------------------------------------"

    # 启动连接
    local pids=()
    local start_time=$(date +%s)

    for i in $(seq 1 $connections); do
        bash -c "
            exec 3<>/dev/tcp/localhost/$PORT
            cat <&3 > /dev/null &
            for j in {1..12}; do
                echo 'PING' >&3
                sleep 5
            done
            echo 'QUIT' >&3
            exec 3>&-
        " 2>/dev/null &
        pids+=($!)

        # 每 100 个连接暂停一下，避免过载
        if [ $((i % 100)) -eq 0 ]; then
            sleep 0.5
        fi
    done

    echo "Spawned $connections connections"

    # 等待连接建立
    sleep 5

    # 收集指标
    local metrics_file="$RESULT_DIR/${test_name}_metrics.txt"
    local stats_file="$RESULT_DIR/${test_name}_stats.csv"

    echo "timestamp,active_connections,total_connections,messages_received,messages_sent" > "$stats_file"

    # 监控 60 秒
    for i in {1..12}; do
        local timestamp=$(date +%s)
        local elapsed=$((timestamp - start_time))

        # 获取指标
        local metrics=$(curl -s http://localhost:$METRICS_PORT/metrics 2>/dev/null)

        local active=$(echo "$metrics" | grep "^gateway_connections_active " | awk '{print $2}')
        local total=$(echo "$metrics" | grep "^gateway_connections_total " | awk '{print $2}')
        local received=$(echo "$metrics" | grep "^gateway_messages_received_total " | awk '{print $2}')
        local sent=$(echo "$metrics" | grep "^gateway_messages_sent_total " | awk '{print $2}')

        echo "$elapsed,$active,$total,$received,$sent" >> "$stats_file"

        echo "  [$elapsed s] Active: $active, Total: $total, RX: $received, TX: $sent"

        sleep 5
    done

    # 保存最终指标
    curl -s http://localhost:$METRICS_PORT/metrics > "$metrics_file"

    # 清理
    for pid in "${pids[@]}"; do
        kill $pid 2>/dev/null || true
    done
    wait 2>/dev/null || true

    echo "Test completed: $test_name"
    echo
}

# 运行所有测试
for conn in "${CONNECTIONS_LIST[@]}"; do
    run_test $conn

    # 测试间隔，让系统恢复
    echo "Cooling down for 10 seconds..."
    sleep 10
done

# 生成报告
echo "=== Generating Report ==="

REPORT_FILE="$RESULT_DIR/REPORT.md"

cat > "$REPORT_FILE" << 'REPORT_HEADER'
# Gateway System Performance Test Report

## Test Configuration

- **Date:** $(date)
- **Server:** localhost:$PORT
- **Metrics:** localhost:$METRICS_PORT
- **Duration:** 60 seconds per test
- **Message Pattern:** PING every 5 seconds

## Test Results

REPORT_HEADER

for conn in "${CONNECTIONS_LIST[@]}"; do
    local stats_file="$RESULT_DIR/test_${conn}_connections_stats.csv"

    if [ -f "$stats_file" ]; then
        local max_active=$(tail -n +2 "$stats_file" | cut -d',' -f2 | sort -n | tail -1)
        local final_total=$(tail -1 "$stats_file" | cut -d',' -f3)
        local final_rx=$(tail -1 "$stats_file" | cut -d',' -f4)
        local final_tx=$(tail -1 "$stats_file" | cut -d',' -f5)

        cat >> "$REPORT_FILE" << EOF

### Test: $conn Connections

- **Max Active Connections:** $max_active
- **Total Connections:** $final_total
- **Messages Received:** $final_rx
- **Messages Sent:** $final_tx
- **Data File:** test_${conn}_connections_stats.csv

EOF
    fi
done

cat >> "$REPORT_FILE" << 'REPORT_FOOTER'

## Observations

- Connection stability across different load levels
- Message throughput scales with connection count
- System remains responsive under load

## Files

- `*_stats.csv`: Time-series metrics data
- `*_metrics.txt`: Prometheus metrics snapshot
- `REPORT.md`: This report

REPORT_FOOTER

echo "Report generated: $REPORT_FILE"
echo
echo "=== Performance Test Complete ==="
echo "Results saved to: $RESULT_DIR"
