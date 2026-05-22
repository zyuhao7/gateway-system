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
```

**参数说明：**
- 第一个参数：监听端口（默认 8080）
- 第二个参数：工作线程数（默认 1，单线程模式）

## 测试

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

### Phase 4: 分布式路由 🚧
- 一致性哈希路由
- Redis 状态同步
- 节点发现与健康检查

### Phase 5: 可观测性 🚧
- Prometheus 指标导出
- Grafana 监控面板
- 性能压测与分析

### Phase 6: 容器化部署 🚧
- Docker 镜像
- Kubernetes 部署
- 自动扩缩容

## 性能目标

- 单机 5-20 万空闲连接
- 万级活跃连接低延迟
- 多节点一致性哈希路由
- 毫秒级消息转发延迟

## 开发进度

详见 [PLAN.md](PLAN.md)
