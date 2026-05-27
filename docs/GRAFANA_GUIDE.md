# Grafana Dashboard 使用指南

## 快速启动

### 1. 启动监控栈

```bash
cd /home/xuhao/code/gateway-system

# 启动 Prometheus + Grafana + Redis
docker-compose up -d prometheus grafana redis

# 验证服务状态
docker-compose ps
```

### 2. 访问 Grafana

- **URL**: http://localhost:3000
- **用户名**: admin
- **密码**: admin

首次登录后会提示修改密码（可跳过）。

### 3. 查看 Dashboard

Dashboard 已自动导入，路径：
- Home → Dashboards → Gateway System Dashboard

## Dashboard 面板说明

### 连接监控
1. **Active Connections** - 实时活跃连接数
   - 显示当前保持的连接数
   - 用于监控系统负载

2. **Connection Rate** - 连接速率
   - 每秒新建连接数
   - 每秒关闭连接数
   - 用于识别流量模式

### 消息监控
3. **Message Throughput** - 消息吞吐量
   - 每秒接收消息数
   - 每秒发送消息数
   - 用于监控业务流量

4. **Message Latency** - 消息延迟
   - P50: 中位数延迟
   - P99: 99% 请求延迟
   - P999: 99.9% 请求延迟
   - **告警**: P99 > 10ms 触发告警

### 资源监控
5. **Buffer Pool Usage** - 缓冲池使用情况
   - Allocated: 总分配数
   - In Use: 使用中数量
   - Available: 可用数量
   - 用于评估内存优化效果

6. **Buffer Pool Efficiency** - 缓冲池效率
   - 显示使用率百分比
   - 绿色 (>80%): 高效利用
   - 黄色 (50-80%): 正常
   - 红色 (<50%): 利用率低

7. **Error Rate** - 错误率
   - 每秒错误数
   - **告警**: 错误率 > 10/s 触发告警

### 统计面板
8. **Connection Statistics** - 总连接数
9. **Messages Received** - 总接收消息数
10. **Messages Sent** - 总发送消息数

## 告警配置

Dashboard 包含两个预配置告警：

### 1. High P99 Latency
- **条件**: P99 延迟 > 10ms
- **评估周期**: 5 分钟
- **触发**: 平均值超过阈值

### 2. High Error Rate
- **条件**: 错误率 > 10/s
- **评估周期**: 5 分钟
- **触发**: 平均值超过阈值

## 自定义配置

### 修改刷新间隔

Dashboard 默认 5 秒刷新，可在右上角修改：
- 5s (默认)
- 10s
- 30s
- 1m

### 添加新面板

1. 点击右上角 "Add panel"
2. 选择可视化类型（Graph, Stat, Gauge 等）
3. 配置 Prometheus 查询
4. 保存 Dashboard

### 常用 PromQL 查询

```promql
# 连接增长率
rate(gateway_connections_total[5m])

# 消息吞吐量
rate(gateway_messages_received_total[1m]) + rate(gateway_messages_sent_total[1m])

# 缓冲池利用率
gateway_buffer_pool_in_use / gateway_buffer_pool_allocated * 100

# 错误率
rate(gateway_errors_total[1m])

# P99 延迟趋势
gateway_latency_microseconds{quantile="0.99"}
```

## 压测监控

### 启动压测并监控

```bash
# 终端 1: 启动网关
./build/gateway_server 8081 4 9091

# 终端 2: 运行压测
./build/stress_test_client localhost 8081 10000

# 浏览器: 打开 Grafana Dashboard
# http://localhost:3000
```

### 关键指标观察

压测期间重点关注：
1. **Active Connections** - 是否达到目标连接数
2. **Message Latency** - P99 是否 < 10ms
3. **Buffer Pool Efficiency** - 是否 > 80%
4. **Error Rate** - 是否为 0

## 导出与分享

### 导出 Dashboard

1. Dashboard 设置 → JSON Model
2. 复制 JSON 内容
3. 保存到文件

### 导入 Dashboard

1. Dashboards → Import
2. 上传 JSON 文件或粘贴 JSON
3. 选择 Prometheus 数据源
4. 点击 Import

## 故障排查

### Grafana 无法连接 Prometheus

```bash
# 检查 Prometheus 状态
curl http://localhost:9090/-/healthy

# 检查网络连接
docker exec gateway-grafana ping prometheus

# 查看 Grafana 日志
docker logs gateway-grafana
```

### Dashboard 无数据

```bash
# 检查网关是否暴露指标
curl http://localhost:9091/metrics

# 检查 Prometheus 是否抓取
curl http://localhost:9090/api/v1/targets

# 验证 PromQL 查询
curl 'http://localhost:9090/api/v1/query?query=gateway_connections_active'
```

### 告警不触发

1. 检查 Grafana Alerting 配置
2. 验证告警规则条件
3. 查看 Alerting → Alert Rules

## 最佳实践

1. **定期检查**: 每天查看 Dashboard，了解系统状态
2. **压测验证**: 每次发布前进行压测并观察指标
3. **告警响应**: 收到告警后立即检查相关面板
4. **历史分析**: 使用时间范围选择器分析历史趋势
5. **导出报告**: 定期导出关键指标截图用于汇报

## 进阶功能

### 变量配置

可添加变量实现多节点切换：
- 节点 ID
- 时间范围
- 刷新间隔

### 注解（Annotations）

标记重要事件：
- 部署时间
- 配置变更
- 故障发生

### 快照分享

创建快照分享给团队：
1. Share → Snapshot
2. 设置过期时间
3. 复制链接分享
