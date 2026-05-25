# 分布式高并发网关系统

基于 C++20 协程与 Boost.Asio 的高性能网关服务器。

## 特性

- ✅ C++20 协程异步网络编程
- ✅ 连接生命周期管理（注册、心跳、超时）
- ✅ 多线程 Reactor 模型（io_context 池）
- ✅ 连接池与广播功能
- 🚧 分布式路由与状态同步
- 🚧 Prometheus 指标监控

## 构建

```bash
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++-18
ninja -C build
```

## 运行

```bash
# 单线程模式（默认）
./build/gateway_server 8080

# 多线程模式
./build/gateway_server 8080 4   # 4 个工作线程
./build/gateway_server 8080 8   # 8 个工作线程

# 指定 Prometheus 指标端口
./build/gateway_server 8080 4 9091  # 指标端口 9091
```

**参数说明：**
- 第一个参数：监听端口（默认 8080）
- 第二个参数：工作线程数（默认 1，单线程模式）
- 第三个参数：Prometheus 指标端口（默认 9090）

**Prometheus 指标：**
访问 `http://localhost:9090/metrics` 查看实时指标：
- `gateway_connections_total` - 总连接数
- `gateway_connections_active` - 活跃连接数
- `gateway_messages_received_total` - 接收消息数
- `gateway_messages_sent_total` - 发送消息数
- `gateway_latency_microseconds` - 延迟分布（P50/P99/P999）

## 部署

### Docker Compose（推荐用于开发/测试）

```bash
# 一键部署 3 节点集群
./scripts/deploy.sh

# 访问服务
# Gateway: localhost:8081, 8082, 8083
# Prometheus: http://localhost:9090
# Grafana: http://localhost:3000 (admin/admin)
```

### Kubernetes（生产环境）

```bash
# 部署到 K8s 集群
kubectl apply -f k8s/redis.yaml
kubectl apply -f k8s/deployment.yaml

# 查看状态
kubectl get pods -n gateway-system
```

详见 [DEPLOYMENT.md](DEPLOYMENT.md)

```bash
# 简单连接测试
echo -e "PING\nQUIT" | nc localhost 8080

# 功能测试（10 个连接，持续 30 秒）
./scripts/benchmark.sh 8080 10 30

# 多线程性能对比测试
./scripts/benchmark_threads.sh 8080 100 30
```

## 协议

**基础命令：**
- `PING` → `PONG`
- `QUIT` → `BYE` (断开连接)

**连接管理命令：**
- `STATS` → 返回连接统计信息
- `BROADCAST <message>` → 向所有连接广播消息

**心跳机制：**
- 心跳间隔: 10s
- 超时时间: 30s
- 自动清理超时连接

## 架构

### Phase 1: 基础网关 ✅
- C++20 协程 + Asio TCP 服务器
- PING/PONG 协议
- 心跳检测

### Phase 2: 连接管理 ✅
- ConnectionManager 全局连接池
- 线程安全的连接注册/注销
- STATS 和 BROADCAST 命令

### Phase 3: 多线程优化 ✅
- IoContextPool 线程池
- 连接按 hash 分配到不同线程
- 支持单线程/多线程模式切换

### Phase 4: 分布式路由 ✅
- 一致性哈希路由（150 虚拟节点）
- 节点注册与发现框架
- 消息路由器（本地/跨节点）
- 用户到节点映射

### Phase 5: 可观测性 ✅
- Prometheus 指标采集
- HTTP /metrics 端点
- 连接数、消息数、延迟统计
- 实时性能监控

### Phase 6: 容器化部署 ✅
- Dockerfile（多阶段构建）
- Docker Compose（3 节点集群）
- Kubernetes 部署配置
- HPA 自动扩缩容
- 健康检查和滚动更新

## 性能目标

- 单机 5-20 万空闲连接
- 万级活跃连接低延迟
- 多节点一致性哈希路由
- 毫秒级消息转发延迟

## 开发进度

详见 [PLAN.md](PLAN.md)
