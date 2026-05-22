# gateway-system
分布式网关系统
# 分布式高并发网关系统

基于 C++20 协程与 Boost.Asio 的高性能网关服务器。

## 特性

- ✅ C++20 协程异步网络编程
- ✅ 连接注册、心跳检测、超时管理
- ✅ PING/PONG 协议、消息回显
- 🚧 多线程 Reactor 模型
- 🚧 分布式路由与状态同步
- 🚧 Prometheus 指标监控

## 构建

```bash
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++-18
ninja -C build
```

## 运行

```bash
./build/gateway_server [port]
```


## 测试

```bash
# 测试

# 简单测试
echo -e "PING\nQUIT" | nc localhost 8080

# 压测
./scripts/benchmark.sh
```

## 协议

- `PING` → `PONG`
- `QUIT` → `BYE` (断开连接)
- 其他消息 → `ECHO: <msg>`
- 心跳间隔: 30s，超时: 90s

## 目标

- 单机 5-20 万空闲连接
- 万级活跃连接低延迟
- 多节点一致性哈希路由
