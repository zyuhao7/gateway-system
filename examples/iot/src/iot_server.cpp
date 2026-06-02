/**
 * @file iot_server.cpp
 * @brief IoT服务器示例 - 设备连接和数据采集
 */

#include "rate_limiter.hpp"
#include "message_storage.hpp"
#include <iostream>
#include <map>
#include <random>
#include <thread>
#include <chrono>

using namespace gateway;

// 设备信息
struct Device {
    std::string device_id;
    std::string device_type;
    bool online;
    int64_t last_seen;
    std::map<std::string, double> sensor_data;
};

class IoTServer {
public:
    IoTServer(uint16_t port) : port_(port) {
        // 配置速率限制
        IPRateLimiter::instance().configure(500, 1000);  // 500 req/s for IoT devices

        // 打开数据存储
        MessageStorage::instance().open("./data/iot_data");

        std::cout << "IoT Server initialized on port " << port_ << std::endl;
    }

    void start() {
        std::cout << "\n=== IoT Server Demo ===" << std::endl;
        std::cout << "\nSupported Devices:" << std::endl;
        std::cout << "  - Temperature Sensors" << std::endl;
        std::cout << "  - Humidity Sensors" << std::endl;
        std::cout << "  - Motion Detectors" << std::endl;
        std::cout << "  - Smart Switches" << std::endl;
        std::cout << std::endl;

        run_demo();
    }

private:
    void run_demo() {
        // 1. 设备注册
        std::cout << "[1] Device Registration" << std::endl;

        register_device("device_001", "temperature_sensor");
        register_device("device_002", "humidity_sensor");
        register_device("device_003", "motion_detector");

        std::cout << "  Total devices: " << devices_.size() << std::endl;
        std::cout << std::endl;

        // 2. 设备上线
        std::cout << "[2] Devices Coming Online" << std::endl;

        device_online("device_001");
        device_online("device_002");
        device_online("device_003");

        std::cout << "  Online devices: " << count_online_devices() << std::endl;
        std::cout << std::endl;

        // 3. 数据采集
        std::cout << "[3] Data Collection" << std::endl;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> temp_dist(20.0, 30.0);
        std::uniform_real_distribution<> humidity_dist(40.0, 80.0);

        for (int i = 0; i < 5; ++i) {
            std::cout << "  Cycle " << (i + 1) << ":" << std::endl;

            // 温度传感器
            double temp = temp_dist(gen);
            report_data("device_001", "temperature", temp);
            std::cout << "    device_001: " << temp << "°C" << std::endl;

            // 湿度传感器
            double humidity = humidity_dist(gen);
            report_data("device_002", "humidity", humidity);
            std::cout << "    device_002: " << humidity << "%" << std::endl;

            // 运动检测器
            bool motion = (i % 2 == 0);
            report_data("device_003", "motion", motion ? 1.0 : 0.0);
            std::cout << "    device_003: " << (motion ? "Motion detected" : "No motion") << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::cout << std::endl;

        // 4. 告警检测
        std::cout << "[4] Alert Detection" << std::endl;

        auto& device = devices_["device_001"];
        double current_temp = device.sensor_data["temperature"];

        if (current_temp > 28.0) {
            std::cout << "  ⚠️  Alert: High temperature detected!" << std::endl;
            std::cout << "  Device: device_001" << std::endl;
            std::cout << "  Temperature: " << current_temp << "°C" << std::endl;
            std::cout << "  Threshold: 28.0°C" << std::endl;

            // 发送告警通知
            send_alert("device_001", "temperature_high", current_temp);
        } else {
            std::cout << "  No alerts" << std::endl;
        }
        std::cout << std::endl;

        // 5. 设备控制
        std::cout << "[5] Device Control" << std::endl;

        send_command("device_001", "set_threshold", 25.0);
        std::cout << "  Command sent to device_001: set_threshold = 25.0°C" << std::endl;

        send_command("device_003", "set_sensitivity", 5.0);
        std::cout << "  Command sent to device_003: set_sensitivity = 5" << std::endl;
        std::cout << std::endl;

        // 6. 数据统计
        std::cout << "[6] Statistics" << std::endl;

        for (const auto& [device_id, device] : devices_) {
            std::cout << "  " << device_id << " (" << device.device_type << "):" << std::endl;
            for (const auto& [key, value] : device.sensor_data) {
                std::cout << "    " << key << ": " << value << std::endl;
            }
        }
        std::cout << std::endl;

        // 7. 设备离线
        std::cout << "[7] Devices Going Offline" << std::endl;

        device_offline("device_001");
        device_offline("device_002");
        device_offline("device_003");

        std::cout << "  Online devices: " << count_online_devices() << std::endl;
        std::cout << std::endl;

        std::cout << "=== Demo completed ===" << std::endl;
    }

    void register_device(const std::string& device_id, const std::string& device_type) {
        Device device;
        device.device_id = device_id;
        device.device_type = device_type;
        device.online = false;
        device.last_seen = 0;

        devices_[device_id] = device;
        std::cout << "  Device registered: " << device_id << " (" << device_type << ")" << std::endl;
    }

    void device_online(const std::string& device_id) {
        auto it = devices_.find(device_id);
        if (it != devices_.end()) {
            it->second.online = true;
            it->second.last_seen = std::chrono::system_clock::now().time_since_epoch().count();
            std::cout << "  " << device_id << " is online" << std::endl;
        }
    }

    void device_offline(const std::string& device_id) {
        auto it = devices_.find(device_id);
        if (it != devices_.end()) {
            it->second.online = false;
            std::cout << "  " << device_id << " is offline" << std::endl;
        }
    }

    void report_data(const std::string& device_id, const std::string& key, double value) {
        auto it = devices_.find(device_id);
        if (it != devices_.end()) {
            it->second.sensor_data[key] = value;
            it->second.last_seen = std::chrono::system_clock::now().time_since_epoch().count();

            // 存储数据点
            StoredMessage data_point;
            data_point.message_id = device_id + "_" + std::to_string(it->second.last_seen);
            data_point.from_user = device_id;
            data_point.to_user = "server";
            data_point.content = key + "=" + std::to_string(value);
            data_point.timestamp = it->second.last_seen;
            data_point.delivered = true;

            MessageStorage::instance().store_message(data_point);
        }
    }

    void send_alert(const std::string& device_id, const std::string& alert_type, double value) {
        // 发送告警到监控系统
        alert_count_++;
    }

    void send_command(const std::string& device_id, const std::string& command, double param) {
        auto it = devices_.find(device_id);
        if (it != devices_.end() && it->second.online) {
            command_count_++;
        }
    }

    int count_online_devices() {
        int count = 0;
        for (const auto& [_, device] : devices_) {
            if (device.online) count++;
        }
        return count;
    }

    uint16_t port_;
    std::map<std::string, Device> devices_;
    int alert_count_ = 0;
    int command_count_ = 0;
};

int main(int argc, char* argv[]) {
    uint16_t port = (argc > 1) ? std::stoi(argv[1]) : 8082;

    try {
        IoTServer server(port);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
