# 部署指南

## Docker Compose 部署

### 前置要求
- Docker 20.10+
- Docker Compose 2.0+

### 快速启动

```bash
# 一键部署
./scripts/deploy.sh

# 或手动部署
docker build -t gateway-system:latest .
docker-compose up -d
```

### 服务访问

**网关节点：**
- Gateway 1: `localhost:8081` (指标: `localhost:9091/metrics`)
- Gateway 2: `localhost:8082` (指标: `localhost:9092/metrics`)
- Gateway 3: `localhost:8083` (指标: `localhost:9093/metrics`)

**基础设施：**
- Redis: `localhost:6379`
- Prometheus: `http://localhost:9090`
- Grafana: `http://localhost:3000` (admin/admin)

### 测试连接

```bash
# 连接到 Gateway 1
echo "PING" | nc localhost 8081

# 查看指标
curl http://localhost:9091/metrics

# 查看所有服务状态
docker-compose ps
```

### 停止服务

```bash
docker-compose down

# 删除数据卷
docker-compose down -v
```

---

## Kubernetes 部署

### 前置要求
- Kubernetes 1.24+
- kubectl 配置完成
- 集群支持 LoadBalancer（或使用 NodePort）

### 部署步骤

```bash
# 1. 创建命名空间
kubectl apply -f k8s/deployment.yaml

# 2. 部署 Redis
kubectl apply -f k8s/redis.yaml

# 3. 等待 Redis 就绪
kubectl wait --for=condition=ready pod -l app=redis -n gateway-system --timeout=60s

# 4. 部署网关
kubectl apply -f k8s/deployment.yaml

# 5. 查看部署状态
kubectl get pods -n gateway-system
kubectl get svc -n gateway-system
```

### 访问服务

```bash
# 获取 LoadBalancer IP
kubectl get svc gateway-service -n gateway-system

# 端口转发（本地测试）
kubectl port-forward -n gateway-system svc/gateway-service 8080:8080 9090:9090
```

### 扩缩容

```bash
# 手动扩容
kubectl scale deployment gateway-deployment -n gateway-system --replicas=5

# 查看 HPA 状态
kubectl get hpa -n gateway-system

# HPA 自动扩缩容（基于 CPU/内存）
# - CPU > 70% 或 Memory > 80% 时扩容
# - 最小 3 个副本，最大 10 个副本
```

### 滚动更新

```bash
# 更新镜像
kubectl set image deployment/gateway-deployment \
  gateway=gateway-system:v2 \
  -n gateway-system

# 查看更新状态
kubectl rollout status deployment/gateway-deployment -n gateway-system

# 回滚
kubectl rollout undo deployment/gateway-deployment -n gateway-system
```

### 监控和日志

```bash
# 查看日志
kubectl logs -f -l app=gateway -n gateway-system

# 查看指标
kubectl top pods -n gateway-system

# 进入容器
kubectl exec -it <pod-name> -n gateway-system -- /bin/bash
```

### 清理

```bash
kubectl delete namespace gateway-system
```

---

## 配置说明

### 环境变量

| 变量 | 说明 | 默认值 |
|------|------|--------|
| `NODE_ID` | 节点唯一标识 | `gateway-1` |
| `REDIS_HOST` | Redis 地址 | `localhost` |
| `REDIS_PORT` | Redis 端口 | `6379` |
| `METRICS_PORT` | Prometheus 指标端口 | `9090` |

### 资源配置

**Docker Compose:**
- 无资源限制（开发环境）

**Kubernetes:**
- Requests: 256Mi 内存, 250m CPU
- Limits: 512Mi 内存, 500m CPU

### 健康检查

- **Liveness Probe:** 每 30 秒检查 `/metrics` 端点
- **Readiness Probe:** 每 10 秒检查 `/metrics` 端点
- **启动延迟:** 5-10 秒

---

## 故障排查

### 容器无法启动

```bash
# 查看日志
docker-compose logs gateway-1

# 检查镜像
docker images | grep gateway-system

# 重新构建
docker-compose build --no-cache
```

### 连接 Redis 失败

```bash
# 检查 Redis 状态
docker-compose ps redis
redis-cli -h localhost -p 6379 ping

# Kubernetes
kubectl logs -l app=redis -n gateway-system
```

### Prometheus 无法抓取指标

```bash
# 检查指标端点
curl http://localhost:9091/metrics

# 检查 Prometheus 配置
docker exec gateway-prometheus cat /etc/prometheus/prometheus.yml

# 查看 Prometheus targets
# 访问 http://localhost:9090/targets
```

---

## 性能调优

### Docker

```yaml
# docker-compose.yml
services:
  gateway-1:
    deploy:
      resources:
        limits:
          cpus: '2'
          memory: 1G
        reservations:
          cpus: '1'
          memory: 512M
```

### Kubernetes

```yaml
# 调整 HPA 阈值
spec:
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        averageUtilization: 60  # 降低阈值，更早扩容
```

---

## 生产环境建议

1. **使用持久化存储**
   - Redis 数据持久化
   - Prometheus 数据保留策略

2. **配置 TLS**
   - 网关 HTTPS
   - Redis TLS 连接

3. **监控告警**
   - Prometheus AlertManager
   - Grafana 告警规则

4. **备份策略**
   - Redis 定期备份
   - 配置文件版本控制

5. **日志聚合**
   - ELK Stack
   - Loki + Grafana
