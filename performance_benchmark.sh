#!/bin/bash

echo "=== Gateway System Performance Benchmark ==="
echo "Date: $(date)"
echo

# 1. 测试所有单元测试性能
echo "[1] Unit Tests Performance"
cd build
time ctest --output-on-failure 2>&1 | grep "tests passed"
echo

# 2. IM服务器性能测试
echo "[2] IM Server Performance Test"
echo "  Testing authentication + room management + message storage..."
time ./examples/im/im_server > /tmp/im_perf.log 2>&1
echo "  ✓ Completed"
echo

# 3. 游戏服务器性能测试  
echo "[3] Game Server Performance Test"
echo "  Testing matchmaking + room + state sync..."
time ./examples/game/game_server > /tmp/game_perf.log 2>&1
echo "  ✓ Completed"
echo

# 4. IoT服务器性能测试
echo "[4] IoT Server Performance Test"
echo "  Testing device connections + data collection..."
time ./examples/iot/iot_server > /tmp/iot_perf.log 2>&1
echo "  ✓ Completed"
echo

# 5. 推送服务器性能测试
echo "[5] Push Server Performance Test"
echo "  Testing subscriptions + message push..."
time ./examples/push/push_server > /tmp/push_perf.log 2>&1
echo "  ✓ Completed"
echo

# 6. 内存使用统计
echo "[6] Memory Usage Analysis"
echo "  IM Server: $(grep -o '[0-9]\+ offline' /tmp/im_perf.log | wc -l) offline messages processed"
echo "  Game Server: $(grep -o 'scored [0-9]\+' /tmp/game_perf.log | wc -l) game events"
echo "  IoT Server: $(grep -o 'device_[0-9]\+' /tmp/iot_perf.log | wc -l) device operations"
echo "  Push Server: $(grep -o 'Delivered: [0-9]\+' /tmp/push_perf.log | wc -l) push operations"
echo

# 7. 对象池性能测试
echo "[7] Buffer Pool Performance"
./test_buffer_pool 2>&1 | grep -E "ops/sec|Performance"
echo

echo "=== Benchmark Complete ==="
