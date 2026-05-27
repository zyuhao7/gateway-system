#!/bin/bash
# 生产环境部署脚本

set -e

DEPLOY_DIR="/opt/gateway-system"
SERVICE_NAME="gateway"
BACKUP_DIR="/backup/gateway"

echo "=== Gateway System Deployment Script ==="
echo ""

# 检查权限
if [ "$EUID" -ne 0 ]; then
    echo "Error: Please run as root"
    exit 1
fi

# 创建部署目录
echo "Creating deployment directories..."
mkdir -p ${DEPLOY_DIR}/{bin,config,logs,certs}
mkdir -p ${BACKUP_DIR}

# 创建系统用户
if ! id -u gateway > /dev/null 2>&1; then
    echo "Creating gateway user..."
    useradd -r -s /bin/false gateway
fi

# 备份旧版本
if [ -f "${DEPLOY_DIR}/bin/gateway_server" ]; then
    echo "Backing up old version..."
    cp ${DEPLOY_DIR}/bin/gateway_server ${BACKUP_DIR}/gateway_server.$(date +%Y%m%d_%H%M%S)
fi

# 复制二进制
echo "Installing gateway server..."
cp build/gateway_server ${DEPLOY_DIR}/bin/
chmod +x ${DEPLOY_DIR}/bin/gateway_server

# 复制配置
if [ -d "config" ]; then
    echo "Installing configuration files..."
    cp -r config/* ${DEPLOY_DIR}/config/
fi

# 生成 TLS 证书（如果不存在）
if [ ! -f "${DEPLOY_DIR}/certs/server.crt" ]; then
    echo "Generating TLS certificates..."
    openssl req -x509 -newkey rsa:4096 \
        -keyout ${DEPLOY_DIR}/certs/server.key \
        -out ${DEPLOY_DIR}/certs/server.crt \
        -days 365 -nodes \
        -subj "/C=CN/ST=Beijing/L=Beijing/O=Gateway/CN=gateway.local"
    chmod 600 ${DEPLOY_DIR}/certs/server.key
fi

# 设置权限
echo "Setting permissions..."
chown -R gateway:gateway ${DEPLOY_DIR}
chmod 755 ${DEPLOY_DIR}/bin/gateway_server

# 安装 systemd 服务
echo "Installing systemd service..."
cp deploy/gateway.service /etc/systemd/system/
systemctl daemon-reload

# 启用服务
echo "Enabling service..."
systemctl enable ${SERVICE_NAME}

# 检查服务状态
if systemctl is-active --quiet ${SERVICE_NAME}; then
    echo ""
    echo "Service is currently running. Restart to apply changes:"
    echo "  sudo systemctl restart ${SERVICE_NAME}"
else
    echo ""
    echo "Starting service..."
    systemctl start ${SERVICE_NAME}
fi

echo ""
echo "=== Deployment Complete ==="
echo ""
echo "Service status:"
systemctl status ${SERVICE_NAME} --no-pager || true
echo ""
echo "Useful commands:"
echo "  Start:   sudo systemctl start ${SERVICE_NAME}"
echo "  Stop:    sudo systemctl stop ${SERVICE_NAME}"
echo "  Restart: sudo systemctl restart ${SERVICE_NAME}"
echo "  Status:  sudo systemctl status ${SERVICE_NAME}"
echo "  Logs:    sudo journalctl -u ${SERVICE_NAME} -f"
echo ""
echo "Metrics endpoint: http://localhost:9090/metrics"
echo "Gateway endpoint: tcp://localhost:8080"
