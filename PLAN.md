# 分布式高并发网关系统开发计划

## 项目目标

构建基于 C++20 协程与 Boost.Asio 的分布式网关系统，展示底层系统编程和分布式架构能力。

**核心指标：**
- 单机支撑 5-20 万空闲连接
- 万级活跃连接下 P99 延迟 < 10ms
- 多节点一致性哈希路由
- 完整可观测性（Prometheus + Grafana）

---

## 开发阶段

### ✅ Phase 0: 环境配置
**状态：已完成**

- [√] Ubuntu 24.04 + GCC 13 + Clang 18
- [√] Boost.Asio 1.83
- [√] CMake + Ninja 构建系统
- [√] 压测工具（wrk, iperf3）
- [√] 观测工具（perf, flamegraph）
- [√] Docker + Prometheus + Grafana

---

### ✅ Phase 1: C++20 协程 + Asio 基础网关
**状态：已完成**

**目标：** 实现基础 TCP 服务器，验证 C++20 协程异步模型

**已完成：**
- [√] TCP acceptor 使用 `co_await` 接受连接
- [√] Connection 类管理单个连接生命周期
- [√] 异步读写循环（read_loop / write_message）
- [√] PING/PONG 协议 + 消息回显
- [√] 心跳定时器（30s 间隔，90s 超时）
- [√] 优雅关闭（SIGINT/SIGTERM）
- [√] 基础压测脚本

**产出：**
- `src/main.cpp`, `src/server.cpp`, `src/connection.cpp`
- `include/server.hpp`, `include/connection.hpp`
- `scripts/benchmark.sh`

---

### ✅ Phase 2: 连接生命周期管理
**状态：已完成**

**目标：** 完善连接管理，支持大规模连接场景

**已完成：**
- [√] 全局连接池（ConnectionManager）
  - 连接注册/注销
  - 按 ID 查找连接
  - 连接数统计（总数/活跃数）
  - 广播消息到所有连接
- [√] 服务器集成
  - BROADCAST 命令支持
  - STATS 命令查看连接统计
  - 连接生命周期自动管理

**待优化（Phase 3+）：**
- [ ] 用户认证与会话管理
- [ ] 背压控制与慢客户端检测
- [ ] 优雅断线重连
- [ ] 定时器轮优化

**产出：**
- `include/connection_manager.hpp`
- `src/connection_manager.cpp`
- 线程安全的单例模式连接池

---

### ✅ Phase 3: 多线程 Reactor 与无锁优化
**状态：已完成**

**目标：** 提升多核利用率，降低锁竞争

**任务清单：**
- [√] 多线程 IO 线程池
  - 每线程独立 `io_context`
  - 连接按 hash 分配到线程
  - IoContextPool 类管理线程池
- [√] Server 支持单线程/多线程模式
  - 命令行参数控制线程数
  - 单线程模式向后兼容
- [√] 性能对比测试脚本
  - benchmark_threads.sh 测试不同线程数
  - 监控连接数和性能指标

**已实现：**
- `IoContextPool` 类：管理多个 io_context 和工作线程
- `Server` 类：支持单线程和多线程两种模式
- 连接按 ID hash 分配到不同线程
- 使用 atomic 计数器实现 round-robin 负载均衡
- 性能测试脚本：对比 1/2/4/8 线程性能

**待优化（Phase 4 前）：**
- [ ] 线程本地消息队列（降低锁竞争）
- [ ] 对象池与内存池（减少内存分配）
- [ ] 无锁队列（跨线程消息传递）
- [ ] CPU 火焰图分析

---

### ✅ Phase 4: 分布式路由与状态同步
**状态：已完成（基础框架）**

**目标：** 实现多网关节点协同工作

**已完成：**
- [√] 一致性哈希路由
  - MurmurHash3 哈希算法
  - 150 虚拟节点支持
  - 均匀分布验证（测试 1000 个 key）
- [√] 节点注册与发现框架
  - NodeRegistry 类（支持 Redis 集成）
  - 节点心跳机制（5s 间隔，15s 超时）
  - 节点健康检查
- [√] 消息路由器
  - MessageRouter 类
  - 本地用户消息投递
  - 跨节点路由框架（待实现通信协议）
- [√] 测试脚本
  - 一致性哈希分布测试
  - 节点添加/删除验证

**待完善（需要 Redis/gRPC）：**
- [ ] Redis 实际集成（节点列表存储）
- [ ] 跨节点通信协议（gRPC 或 TCP）
- [ ] 状态同步实现
- [ ] 节点扩缩容实际测试

**产出：**
- `consistent_hash.hpp/cpp` - 一致性哈希实现
- `node_registry.hpp/cpp` - 节点注册与发现
- `message_router.hpp/cpp` - 消息路由
- `scripts/test_distributed.sh` - 分布式测试脚本

---

### ✅ Phase 5: 可观测性与压测
**状态：已完成**

**目标：** 完整的监控体系和性能验证

**已完成：**
- [√] Prometheus 指标暴露
  - gateway_connections_total/active - 连接数统计
  - gateway_messages_received/sent_total - 消息吞吐
  - gateway_latency_microseconds - P50/P99/P999 延迟
  - 原子计数器实现线程安全
- [√] HTTP /metrics 端点
  - MetricsServer 类（简单 HTTP 服务器）
  - Prometheus 文本格式导出
  - 独立端口（默认 9090）
- [√] 压测脚本
  - benchmark.sh - 基础连接测试
  - benchmark_threads.sh - 多线程性能对比
  - 修复连接持久化问题（/dev/tcp）

**待完善：**
- [ ] Grafana dashboard JSON
- [ ] 内存泄漏检测（Valgrind/ASan）

**产出：**
- `metrics.hpp/cpp` - 指标采集器
- `metrics_server.hpp/cpp` - HTTP 指标服务器
- `scripts/benchmark.sh` - 压测脚本

---

### ✅ Phase 6: 容器化部署与扩缩容
**状态：已完成**

**目标：** 生产级部署方案

**已完成：**
- [√] Dockerfile
  - 多阶段构建（builder + runtime）
  - Ubuntu 24.04 + Clang 18
  - 非 root 用户运行（gateway:1000）
  - 健康检查（/metrics 端点）
- [√] Docker Compose
  - 3 节点网关集群（8081-8083）
  - Redis + Prometheus + Grafana
  - 一键部署脚本（scripts/deploy.sh）
- [√] Kubernetes 部署
  - Deployment + Service + HPA
  - 资源限制（256Mi-512Mi, 250m-500m CPU）
  - HPA 自动扩缩容（3-10 副本，CPU 70%）
  - 滚动更新策略（maxSurge: 1, maxUnavailable: 0）
- [√] 健康检查
  - Liveness probe（30s 间隔）
  - Readiness probe（10s 间隔）
  - 优雅关闭（SIGTERM 处理）

**产出：**
- `Dockerfile` - 多阶段构建配置
- `docker-compose.yml` - 完整集群编排
- `k8s/deployment.yaml` - K8s 部署配置
- `k8s/redis.yaml` - Redis StatefulSet
- `DEPLOYMENT.md` - 部署指南

---

### ✅ Phase 8: 性能测试与报告
**状态：已完成**

**目标：** 系统性能验证与优化建议

**已完成：**
- [√] 自动化性能测试脚本
  - performance_test.sh - 多级负载测试（100-10K 连接）
  - 60 秒测试时长，5 秒采样间隔
  - CSV 时序数据 + Prometheus 指标快照
  - 自动生成测试报告
- [√] CPU 火焰图生成
  - generate_flamegraph.sh - perf + FlameGraph 集成
  - 30 秒性能采样
  - SVG 火焰图 + 性能报告
- [√] 性能报告模板
  - PERFORMANCE.md - 完整测试报告框架
  - 连接容量、多线程、消息吞吐测试场景
  - CPU/内存分析、优化建议
  - 分布式路由验证、容器化部署验证

**待执行：**
- [ ] 实际运行性能测试填充数据
- [ ] 生成真实火焰图
- [ ] 百万连接压测（需要系统调优）

**产出：**
- `scripts/performance_test.sh` - 性能测试脚本
- `scripts/generate_flamegraph.sh` - 火焰图生成脚本
- `PERFORMANCE.md` - 性能报告模板

---

## 最终交付物

### 代码仓库
- GitHub: https://github.com/zyuhao7/gateway-system
- 完整源码 + 构建脚本
- CI/CD 配置（GitHub Actions）

### 文档
- [√] README.md（项目介绍）
- [√] PLAN.md（开发计划）
- [√] DEPLOYMENT.md（部署指南）
- [√] PERFORMANCE.md（性能测试报告模板）
- [ ] ARCHITECTURE.md（架构设计）

### 演示材料
- [ ] 架构图（draw.io / Excalidraw）
- [ ] 压测视频（Grafana 实时监控）
- [ ] 技术分享 PPT

---

## 时间规划

| 阶段 | 预计工作量 | 目标完成时间 |
|------|-----------|-------------|
| Phase 0 | 0.5 天 | ✅ 已完成 |
| Phase 1 | 1 天 | ✅ 已完成 |
| Phase 2 | 2 天 | ✅ 已完成 |
| Phase 3 | 3 天 | ✅ 已完成 |
| Phase 4 | 4 天 | ✅ 已完成 |
| Phase 5 | 2 天 | ✅ 已完成 |
| Phase 6 | 2 天 | ✅ 已完成 |
| Phase 8 | 1 天 | ✅ 已完成 |
| 文档整理 | 1 天 | 进行中 |
| **总计** | **16.5 天** | - |

---

## 技术亮点

1. **C++20 协程**：彻底消除回调地狱，同步风格写异步代码
2. **无锁并发**：线程本地队列 + 对象池，降低锁竞争
3. **一致性哈希**：分布式路由，支持动态扩缩容
4. **可观测性**：Prometheus + Grafana + 火焰图，全方位性能分析
5. **生产级部署**：Docker + K8s，一键部署完整集群

---

## 风险与应对

### 风险 1：百万连接目标过高
**应对：** 降低目标到 5-20 万空闲连接，重点展示架构设计而非绝对数值

### 风险 2：无锁结构调试困难
**应对：** 优先使用线程本地队列，仅在必要时引入无锁队列

### 风险 3：分布式状态一致性复杂
**应对：** 简化场景，只做最终一致性，不追求强一致

### 风险 4：时间不足
**应对：** Phase 4-6 可选，优先完成 Phase 1-3 + 压测报告

---

## 下一步行动

**当前进度：** Phase 0-6, 8 已完成，核心功能就绪

**立即任务：**
1. 运行性能测试填充 PERFORMANCE.md 数据
2. 生成 CPU 火焰图分析热点
3. 编写 ARCHITECTURE.md 架构文档
4. 集成 Redis 实现节点发现（Phase 4 待完善）

**本周目标：** 完成性能测试报告，准备生产环境试运行
