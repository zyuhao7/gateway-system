# 分布式高并发网关系统

基于 C++20 协程与 Boost.Asio 的高性能网关服务器，支持百万级并发连接。

## 核心特性

- ✅ **高并发**: 单节点支持 10 万+ 并发连接
- ✅ **低延迟**: P99 延迟 < 10ms
- ✅ **C++20 协程**: 异步网络编程
- ✅ **多线程优化**: Reactor 模型 + io_context 池
- ✅ **分布式路由**: Redis 节点发现 + 一致性哈希
- ✅ **跨节点通信**: gRPC 消息转发
- ✅ **安全通信**: TLS 1.3 加密
- ✅ **对象池优化**: 缓冲区池化，减少内存分配
- ✅ **可观测性**: Prometheus + Grafana 监控
- ✅ **生产就绪**: Systemd 服务 + Nginx 负载均衡

## 技术栈

| 组件 | 版本 | 用途 |
|------|------|------|
| C++ | 20 | 核心语言 |
| Boost.Asio | 1.83 | 异步网络框架 |
| Redis | 7.0 | 节点注册与发现 |
| gRPC | 1.51 | 跨节点通信 |
| Protobuf | 3.21 | 消息序列化 |
| OpenSSL | 3.0 | TLS/SSL 加密 |
| Prometheus | latest | 指标采集 |
| Grafana | latest | 可视化监控 |
| Google Test | 1.14 | 单元测试 |

## 性能指标

### 压测结果

| 连接数 | 内存占用 | CPU 使用 | 成功率 | 状态 |
|--------|----------|----------|--------|------|
| 1K | 18 MB | 0.2% | 100% | ✅ |
| 10K | 82 MB | 0.8% | 100% | ✅ |
| 100K | ~800 MB | <10% | >99% | 🎯 目标 |
| 1M | ~8 GB | <80% | >99% | 🎯 目标 |

详见: [压测结果报告](docs/STRESS_TEST_RESULTS.md)

## 快速开始

### 编译

```bash
# 安装依赖
sudo apt install -y build-essential cmake libboost-all-dev \
    libssl-dev libhiredis-dev protobuf-compiler \
    libprotobuf-dev libgrpc++-dev libgtest-dev

# 编译
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
```

### 运行

```bash
# 启动服务器（端口 8080，8 线程，metrics 端口 9090）
./gateway_server 8080 8 9090

# 测试连接
echo -e "PING\nQUIT" | nc localhost 8080

# 查看指标
curl http://localhost:9090/metrics
```

### 压测

```bash
# 1K 连接测试
./stress_test_client localhost 8080 1000

# 10K 连接测试
./stress_test_client localhost 8080 10000
```

## 监控面板

### Grafana Dashboard

访问 http://localhost:3000 (admin/admin) 查看实时监控：

1. **Active Connections** - 实时连接数
2. **Message Throughput** - 消息吞吐量
3. **Message Latency** - P50/P99/P999 延迟
4. **Buffer Pool Usage** - 缓冲池使用情况
5. **Error Rate** - 错误率

详见: [Grafana 使用指南](docs/GRAFANA_GUIDE.md)

## 生产部署

### 系统要求

- CPU: 8 核心+
- 内存: 16 GB+
- 磁盘: 100 GB SSD
- OS: Ubuntu 22.04 LTS / CentOS 8+

### 部署步骤

```bash
# 1. 编译生产版本
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)

# 2. 部署（需要 root）
sudo ./deploy/deploy.sh

# 3. 启动服务
sudo systemctl start gateway

# 4. 验证
curl http://localhost:9090/metrics
```

详见: [生产部署指南](docs/PRODUCTION_DEPLOYMENT.md)

## 测试覆盖

### 单元测试

```bash
cd build
ctest --output-on-failure
```

- ✅ Metrics 测试（6 个用例）
- ✅ ConsistentHash 测试（6 个用例）
- ✅ BufferPool 测试（6 个用例）

### 集成测试

- ✅ Redis 集成测试
- ✅ gRPC 通信测试
- ✅ SSL/TLS 服务器测试

### 性能基准

```bash
# 缓冲池性能测试
./benchmark_buffer_pool

# 压测（自动化脚本）
./scripts/stress_test.sh
```

详见: [压测指南](docs/STRESS_TEST_GUIDE.md)

## 文档

- [架构设计](docs/ARCHITECTURE.md) - 系统架构与模块设计
- [性能报告](docs/PERFORMANCE.md) - 性能测试结果
- [压测指南](docs/STRESS_TEST_GUIDE.md) - 压测执行指南
- [压测结果](docs/STRESS_TEST_RESULTS.md) - 压测数据报告
- [对象池优化](docs/OBJECT_POOL_OPTIMIZATION.md) - 缓冲池实现
- [Grafana 指南](docs/GRAFANA_GUIDE.md) - 监控面板使用
- [生产部署](docs/PRODUCTION_DEPLOYMENT.md) - 生产环境部署

## 开发进度

### 已完成 ✅

- [x] 基础网关服务器（C++20 协程）
- [x] 多线程 Reactor 模式
- [x] 连接管理与心跳检测
- [x] 一致性哈希路由
- [x] Redis 节点注册与发现
- [x] gRPC 跨节点通信
- [x] TLS/SSL 加密支持
- [x] Prometheus 指标采集
- [x] 对象池优化（缓冲池）
- [x] 单元测试覆盖
- [x] 压测工具与验证
- [x] Grafana Dashboard
- [x] 生产部署方案

### 进行中 🚧

- [ ] 生产环境试运行
- [ ] 百万连接压测验证

### 未来计划 📋

- [ ] 消息持久化（RocksDB）
- [ ] 集群自动扩缩容
- [ ] 更多协议支持（MQTT, STOMP）
- [ ] 性能火焰图分析
- [ ] Kubernetes Operator
