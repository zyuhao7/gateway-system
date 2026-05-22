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

### Phase 4: 分布式路由与状态同步
**状态：待开始**

**目标：** 实现多网关节点协同工作

**任务清单：**
- [ ] 一致性哈希路由
  - 用户 ID → 网关节点映射
  - 虚拟节点支持
- [ ] 节点发现与注册
  - Redis/etcd 存储节点列表
  - 心跳检测与故障摘除
- [ ] 跨节点消息转发
  - 用户 A（节点1）→ 用户 B（节点2）
  - gRPC 或自定义 TCP 协议
- [ ] 状态同步
  - 用户在线状态
  - 路由表变更通知
- [ ] 节点扩缩容
  - 新节点加入时重新分配连接
  - 节点下线时连接迁移

**预期产出：**
- 3 节点集群部署
- 跨节点消息延迟测试
- 扩缩容演示视频

---

### Phase 5: 可观测性与压测
**状态：待开始**

**目标：** 完整的监控体系和性能验证

**任务清单：**
- [ ] Prometheus 指标暴露
  - 连接数（总数/活跃/空闲）
  - 消息吞吐（QPS）
  - P50/P99/P999 延迟
  - 队列积压深度
- [ ] Grafana 面板
  - 实时连接数曲线
  - 延迟分布热力图
  - 节点负载对比
- [ ] 自研压测器
  - 支持 TCP/WebSocket
  - 可配置消息频率
  - 模拟慢客户端
- [ ] 性能分析
  - perf + flamegraph 生成火焰图
  - 热点函数优化
  - 内存泄漏检测（Valgrind/ASan）

**预期产出：**
- Grafana dashboard JSON
- 压测报告（Markdown + 图表）
- 火焰图 SVG

---

### Phase 6: 容器化部署与扩缩容
**状态：待开始**

**目标：** 生产级部署方案

**任务清单：**
- [ ] Dockerfile
  - 多阶段构建
  - 最小化镜像体积
- [ ] Docker Compose
  - 网关 + Redis + Prometheus + Grafana
  - 一键启动完整环境
- [ ] Kubernetes 部署
  - Deployment + Service + ConfigMap
  - HPA 自动扩缩容
  - 滚动更新策略
- [ ] 健康检查
  - Liveness / Readiness probe
  - 优雅关闭（SIGTERM 处理）

**预期产出：**
- `Dockerfile`, `docker-compose.yml`
- `k8s/` 目录（deployment.yaml 等）
- 部署文档

---

## 最终交付物

### 代码仓库
- GitHub: https://github.com/zyuhao7/gateway-system
- 完整源码 + 构建脚本
- CI/CD 配置（GitHub Actions）

### 文档
- [√] README.md（项目介绍）
- [√] PLAN.md（开发计划）
- [ ] ARCHITECTURE.md（架构设计）
- [ ] PERFORMANCE.md（性能测试报告）
- [ ] DEPLOYMENT.md（部署指南）

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
| Phase 3 | 3 天 | - |
| Phase 4 | 4 天 | - |
| Phase 5 | 2 天 | - |
| Phase 6 | 2 天 | - |
| 文档整理 | 1 天 | - |
| **总计** | **15.5 天** | - |

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

**当前进度：** Phase 2 已完成，准备开始 Phase 3

**立即任务：**
1. 实现多线程 IO 线程池（每线程独立 io_context）
2. 连接按 hash 分配到线程
3. 线程本地消息队列优化
4. 对象池与内存池实现

**本周目标：** 完成 Phase 3 多线程优化，进行多核性能压测
