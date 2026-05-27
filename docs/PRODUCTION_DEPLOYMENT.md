# 生产环境部署指南

## 部署架构

```
                    ┌─────────────┐
                    │   Nginx LB  │
                    │  (Layer 7)  │
                    └──────┬──────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
   ┌────▼────┐       ┌────▼────┐       ┌────▼────┐
   │Gateway-1│       │Gateway-2│       │Gateway-3│
   │ :8080   │       │ :8080   │       │ :8080   │
   └────┬────┘       └────┬────┘       └────┬────┘
        │                  │                  │
        └──────────────────┼──────────────────┘
                           │
                    ┌──────▼──────┐
                    │    Redis    │
                    │  (Cluster)  │
                    └─────────────┘
```

## 前置准备

### 1. 系统要求

**硬件配置**（单节点）:
- CPU: 8 核心+
- 内存: 16 GB+
- 磁盘: 100 GB SSD
- 网络: 1 Gbps+

**操作系统**:
- Ubuntu 22.04 LTS / CentOS 8+
- Kernel 5.10+

### 2. 系统优化

```bash
# 文件描述符限制
sudo tee -a /etc/security/limits.conf <<EOF
* soft nofile 1048576
* hard nofile 1048576
* soft nproc 65535
* hard nproc 65535
EOF

# 内核参数优化
sudo tee -a /etc/sysctl.conf <<EOF
# 网络优化
net.ipv4.ip_local_port_range = 1024 65535
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_fin_timeout = 30
net.core.somaxconn = 65535
net.core.netdev_max_backlog = 65535
net.ipv4.tcp_max_syn_backlog = 8192

# 内存优化
vm.swappiness = 10
vm.dirty_ratio = 60
vm.dirty_background_ratio = 2

# 连接跟踪
net.netfilter.nf_conntrack_max = 1048576
net.nf_conntrack_max = 1048576
EOF

sudo sysctl -p

# 重启生效
sudo reboot
```

### 3. 依赖安装

```bash
# 编译依赖
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    libssl-dev \
    libhiredis-dev \
    protobuf-compiler \
    libprotobuf-dev \
    libgrpc++-dev \
    libgtest-dev

# Docker (可选)
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker $USER
```

## 编译部署

### 1. 编译生产版本

```bash
cd /home/xuhao/code/gateway-system

# 清理旧构建
rm -rf build
mkdir build && cd build

# Release 模式编译
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)

# 验证二进制
./gateway_server --help
```

### 2. 安装到系统

```bash
# 创建部署目录
sudo mkdir -p /opt/gateway-system/{bin,config,logs,certs}

# 复制二进制
sudo cp gateway_server /opt/gateway-system/bin/

# 复制配置
sudo cp ../config/* /opt/gateway-system/config/

# 设置权限
sudo chown -R gateway:gateway /opt/gateway-system
sudo chmod +x /opt/gateway-system/bin/gateway_server
```

### 3. 创建系统用户

```bash
sudo useradd -r -s /bin/false gateway
```

## 配置管理

### 1. 环境配置

创建 `/opt/gateway-system/config/gateway.env`:

```bash
# 节点配置
NODE_ID=gateway-1
GATEWAY_PORT=8080
METRICS_PORT=9090
THREAD_COUNT=8

# Redis 配置
REDIS_HOST=redis.internal
REDIS_PORT=6379
REDIS_PASSWORD=your_redis_password

# 日志配置
LOG_LEVEL=info
LOG_PATH=/opt/gateway-system/logs

# 性能配置
BUFFER_POOL_SIZE=10000
MAX_CONNECTIONS=100000
```

### 2. Systemd 服务

创建 `/etc/systemd/system/gateway.service`:

```ini
[Unit]
Description=Gateway System Service
After=network.target redis.service
Wants=redis.service

[Service]
Type=simple
User=gateway
Group=gateway
WorkingDirectory=/opt/gateway-system
EnvironmentFile=/opt/gateway-system/config/gateway.env
ExecStart=/opt/gateway-system/bin/gateway_server ${GATEWAY_PORT} ${THREAD_COUNT} ${METRICS_PORT}
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure
RestartSec=10
LimitNOFILE=1048576
LimitNPROC=65535

# 日志
StandardOutput=journal
StandardError=journal
SyslogIdentifier=gateway

[Install]
WantedBy=multi-user.target
```

启动服务:

```bash
sudo systemctl daemon-reload
sudo systemctl enable gateway
sudo systemctl start gateway
sudo systemctl status gateway
```

## 负载均衡配置

### Nginx 配置

创建 `/etc/nginx/conf.d/gateway.conf`:

```nginx
upstream gateway_backend {
    least_conn;
    
    server gateway-1.internal:8080 max_fails=3 fail_timeout=30s;
    server gateway-2.internal:8080 max_fails=3 fail_timeout=30s;
    server gateway-3.internal:8080 max_fails=3 fail_timeout=30s;
    
    keepalive 1000;
}

server {
    listen 80;
    server_name gateway.example.com;
    
    # WebSocket 支持
    location / {
        proxy_pass http://gateway_backend;
        proxy_http_version 1.1;
        
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        
        proxy_connect_timeout 60s;
        proxy_send_timeout 300s;
        proxy_read_timeout 300s;
        
        proxy_buffering off;
    }
    
    # 健康检查
    location /health {
        access_log off;
        return 200 "healthy\n";
    }
}

# Metrics 端点（内网访问）
server {
    listen 9090;
    server_name localhost;
    
    allow 10.0.0.0/8;
    deny all;
    
    location /metrics {
        proxy_pass http://gateway-1.internal:9090/metrics;
    }
}
```

重启 Nginx:

```bash
sudo nginx -t
sudo systemctl reload nginx
```

## Redis 集群部署

### Redis Sentinel 配置

```bash
# 主节点
redis-server --port 6379 --requirepass your_password

# 从节点
redis-server --port 6380 --replicaof redis-master 6379 --requirepass your_password

# Sentinel
redis-sentinel /etc/redis/sentinel.conf
```

`/etc/redis/sentinel.conf`:

```
sentinel monitor gateway-redis redis-master 6379 2
sentinel auth-pass gateway-redis your_password
sentinel down-after-milliseconds gateway-redis 5000
sentinel parallel-syncs gateway-redis 1
sentinel failover-timeout gateway-redis 10000
```

## 监控部署

### 1. Prometheus 配置

`/etc/prometheus/prometheus.yml`:

```yaml
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  - job_name: 'gateway'
    static_configs:
      - targets:
        - 'gateway-1.internal:9090'
        - 'gateway-2.internal:9090'
        - 'gateway-3.internal:9090'
    relabel_configs:
      - source_labels: [__address__]
        target_label: instance
```

### 2. Grafana 配置

已包含在 `docker-compose.yml` 中，或手动安装:

```bash
sudo apt install -y grafana
sudo systemctl enable grafana-server
sudo systemctl start grafana-server
```

导入 Dashboard: `grafana/gateway-dashboard.json`

## 安全加固

### 1. TLS/SSL 配置

```bash
# 生成证书
cd /opt/gateway-system/certs
openssl req -x509 -newkey rsa:4096 -keyout server.key -out server.crt -days 365 -nodes

# 设置权限
chmod 600 server.key
chown gateway:gateway server.*
```

### 2. 防火墙规则

```bash
# 允许网关端口
sudo ufw allow 8080/tcp
sudo ufw allow 9090/tcp

# 限制 Redis 访问
sudo ufw allow from 10.0.0.0/8 to any port 6379

# 启用防火墙
sudo ufw enable
```

### 3. 访问控制

- Metrics 端点仅内网访问
- Redis 使用密码认证
- 定期更新依赖库

## 日志管理

### 1. 日志轮转

创建 `/etc/logrotate.d/gateway`:

```
/opt/gateway-system/logs/*.log {
    daily
    rotate 30
    compress
    delaycompress
    notifempty
    create 0640 gateway gateway
    sharedscripts
    postrotate
        systemctl reload gateway > /dev/null 2>&1 || true
    endscript
}
```

### 2. 集中日志

使用 rsyslog 或 ELK Stack 收集日志。

## 备份策略

### 1. Redis 备份

```bash
# 每日备份
0 2 * * * redis-cli --rdb /backup/redis/dump-$(date +\%Y\%m\%d).rdb
```

### 2. 配置备份

```bash
# 备份配置
tar -czf /backup/config-$(date +%Y%m%d).tar.gz /opt/gateway-system/config
```

## 故障恢复

### 1. 节点故障

```bash
# 检查服务状态
sudo systemctl status gateway

# 查看日志
sudo journalctl -u gateway -f

# 重启服务
sudo systemctl restart gateway
```

### 2. Redis 故障

Sentinel 自动故障转移，手动切换:

```bash
redis-cli -p 26379 sentinel failover gateway-redis
```

### 3. 回滚部署

```bash
# 停止服务
sudo systemctl stop gateway

# 恢复旧版本
sudo cp /backup/gateway_server.old /opt/gateway-system/bin/gateway_server

# 启动服务
sudo systemctl start gateway
```

## 性能调优

### 1. 线程数配置

根据 CPU 核心数调整:
- 8 核: 8-16 线程
- 16 核: 16-32 线程

### 2. 缓冲池大小

根据连接数调整:
- 10K 连接: 10000 buffers
- 100K 连接: 50000 buffers
- 1M 连接: 100000 buffers

### 3. TCP 调优

```bash
# 增加 TCP 缓冲区
net.ipv4.tcp_rmem = 4096 87380 16777216
net.ipv4.tcp_wmem = 4096 65536 16777216
```

## 监控告警

### 关键指标

1. **连接数**: gateway_connections_active
2. **P99 延迟**: gateway_latency_microseconds{quantile="0.99"}
3. **错误率**: rate(gateway_errors_total[1m])
4. **缓冲池利用率**: gateway_buffer_pool_in_use / gateway_buffer_pool_allocated

### 告警规则

- P99 延迟 > 10ms
- 错误率 > 10/s
- 连接数 > 90% 容量
- 缓冲池利用率 > 95%

## 灰度发布

### 1. 金丝雀部署

```bash
# 更新单个节点
sudo systemctl stop gateway@1
sudo cp new_gateway_server /opt/gateway-system/bin/
sudo systemctl start gateway@1

# 观察 5-10 分钟
# 无问题则继续更新其他节点
```

### 2. 蓝绿部署

维护两套环境，通过 Nginx 切换流量。

## 容量规划

### 单节点容量

- 最大连接数: 100K
- 推荐连接数: 50K
- 内存占用: ~4 GB
- CPU 使用: < 50%

### 扩容策略

- 连接数 > 80%: 增加节点
- CPU > 70%: 增加线程或节点
- 内存 > 80%: 优化缓冲池或增加节点

## 检查清单

部署前检查:
- [ ] 系统参数已优化
- [ ] 依赖已安装
- [ ] 配置文件已更新
- [ ] TLS 证书已配置
- [ ] 防火墙规则已设置
- [ ] 监控已部署
- [ ] 备份策略已配置
- [ ] 告警规则已设置
- [ ] 负载均衡已配置
- [ ] 压测已通过

## 联系支持

遇到问题请查看:
- 文档: `/opt/gateway-system/docs/`
- 日志: `/opt/gateway-system/logs/`
- 监控: http://grafana.internal:3000
