# 百万连接压测指南

## 系统准备

### 1. 调整系统限制

```bash
# 临时调整（当前会话）
ulimit -n 1048576

# 永久调整（需要 root 权限）
sudo tee -a /etc/security/limits.conf <<EOF
* soft nofile 1048576
* hard nofile 1048576
EOF

# 调整内核参数
sudo tee -a /etc/sysctl.conf <<EOF
net.ipv4.ip_local_port_range = 1024 65535
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_fin_timeout = 30
net.core.somaxconn = 65535
net.core.netdev_max_backlog = 65535
EOF

sudo sysctl -p
```

### 2. 启动服务器

```bash
cd /home/xuhao/code/gateway-system/build

# 启动网关服务器
./gateway_server &

# 验证服务器运行
curl http://localhost:9090/metrics
```

## 压测执行

### 方式一：使用自动化脚本

```bash
cd /home/xuhao/code/gateway-system
./scripts/stress_test.sh
```

脚本会自动执行以下测试场景：
- 1K 连接（预热）
- 10K 连接（小规模）
- 50K 连接（中等规模）
- 100K 连接（大规模）
- 500K 连接（超大规模）
- 1M 连接（百万连接）

### 方式二：手动测试

```bash
cd /home/xuhao/code/gateway-system/build

# 测试 10K 连接
./stress_test_client localhost 8080 10000

# 测试 100K 连接
./stress_test_client localhost 8080 100000

# 测试 1M 连接
./stress_test_client localhost 8080 1000000
```

## 监控指标

### 实时监控

```bash
# 查看服务器指标
watch -n 1 'curl -s http://localhost:9090/metrics | grep gateway_'

# 查看系统资源
htop

# 查看网络连接
watch -n 1 'ss -s'
```

### 关键指标

1. **连接指标**
   - `gateway_connections_total` - 总连接数
   - `gateway_connections_active` - 活跃连接数

2. **消息指标**
   - `gateway_messages_received_total` - 接收消息数
   - `gateway_messages_sent_total` - 发送消息数

3. **缓冲池指标**
   - `gateway_buffer_pool_allocated` - 分配的缓冲区数
   - `gateway_buffer_pool_in_use` - 使用中的缓冲区数
   - `gateway_buffer_pool_available` - 可用缓冲区数

4. **延迟指标**
   - `gateway_latency_microseconds{quantile="0.5"}` - P50 延迟
   - `gateway_latency_microseconds{quantile="0.99"}` - P99 延迟

## 性能目标

| 指标 | 目标值 | 说明 |
|------|--------|------|
| 最大连接数 | 1,000,000 | 单节点百万连接 |
| 连接成功率 | > 99% | 连接建立成功率 |
| P99 延迟 | < 10ms | 消息处理延迟 |
| 内存使用 | < 8GB | 百万连接内存占用 |
| CPU 使用 | < 80% | 平均 CPU 使用率 |

## 故障排查

### 连接失败

```bash
# 检查文件描述符限制
ulimit -n

# 检查端口范围
cat /proc/sys/net/ipv4/ip_local_port_range

# 检查 TIME_WAIT 连接
ss -tan | grep TIME_WAIT | wc -l
```

### 内存不足

```bash
# 查看内存使用
free -h

# 查看进程内存
ps aux | grep gateway_server

# 调整缓冲池大小（修改 buffer_pool.hpp）
# DEFAULT_POOL_SIZE = 500  # 减少预分配
```

### 性能瓶颈

```bash
# CPU 分析
perf record -g ./gateway_server
perf report

# 内存分析
valgrind --tool=massif ./gateway_server

# 网络分析
iftop -i lo
```

## 预期结果

### 小规模测试 (10K 连接)
- 连接时间: < 10 秒
- 内存使用: ~100 MB
- CPU 使用: < 20%

### 中等规模测试 (100K 连接)
- 连接时间: < 60 秒
- 内存使用: ~1 GB
- CPU 使用: < 50%

### 百万连接测试 (1M 连接)
- 连接时间: < 300 秒
- 内存使用: ~8 GB
- CPU 使用: < 80%
- 缓冲池命中率: > 95%

## 注意事项

1. **客户端限制**: 单个客户端进程也受文件描述符限制，可能需要多个客户端进程
2. **网络带宽**: 本地回环测试不受网络限制，实际部署需考虑带宽
3. **测试时长**: 百万连接测试建议运行 5-10 分钟观察稳定性
4. **资源清理**: 测试后确保所有连接正常关闭，避免资源泄漏
