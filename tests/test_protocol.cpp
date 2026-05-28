/**
 * @file test_protocol.cpp
 * @brief 协议测试
 */

#include "protocol.hpp"
#include <iostream>
#include <cassert>

using namespace gateway;

void test_message_id_generation() {
    std::cout << "[Test 1] Message ID generation..." << std::endl;

    auto id1 = Message::generate_message_id();
    auto id2 = Message::generate_message_id();

    std::cout << "  ID1: " << id1 << std::endl;
    std::cout << "  ID2: " << id2 << std::endl;

    assert(id1 != id2);
    assert(id1.length() == 32);

    std::cout << "✓ Message ID generation passed" << std::endl;
}

void test_message_serialization() {
    std::cout << "\n[Test 2] Message serialization..." << std::endl;

    Message msg;
    msg.message_id = "test_msg_001";
    msg.type = MessageType::MESSAGE;
    msg.timestamp = Message::current_timestamp();
    msg.priority = MessagePriority::NORMAL;
    msg.from_user = "alice";
    msg.to_user = "bob";
    msg.content = "Hello, Bob!";
    msg.require_ack = true;

    std::string serialized = msg.serialize();
    std::cout << "  Serialized: " << serialized << std::endl;

    auto deserialized = Message::deserialize(serialized);
    assert(deserialized.has_value());
    assert(deserialized->message_id == msg.message_id);
    assert(deserialized->type == msg.type);
    assert(deserialized->from_user == msg.from_user);
    assert(deserialized->to_user == msg.to_user);
    assert(deserialized->content == msg.content);
    assert(deserialized->require_ack == msg.require_ack);

    std::cout << "✓ Message serialization passed" << std::endl;
}

void test_connect_message() {
    std::cout << "\n[Test 3] Connect message..." << std::endl;

    ConnectMessage conn;
    conn.user_id = "user123";
    conn.token = "secret_token_xyz";
    conn.client_info["platform"] = "web";
    conn.client_info["version"] = "1.0.0";

    Message msg = conn.to_message();
    std::cout << "  Message: " << msg.serialize() << std::endl;

    assert(msg.type == MessageType::CONNECT);
    assert(msg.from_user == "user123");
    assert(msg.require_ack == true);

    auto parsed = ConnectMessage::from_message(msg);
    assert(parsed.has_value());
    assert(parsed->user_id == conn.user_id);
    assert(parsed->token == conn.token);

    std::cout << "✓ Connect message passed" << std::endl;
}

void test_heartbeat_message() {
    std::cout << "\n[Test 4] Heartbeat message..." << std::endl;

    HeartbeatMessage hb;
    hb.client_timestamp = Message::current_timestamp();

    Message msg = hb.to_message();
    std::cout << "  Message: " << msg.serialize() << std::endl;

    assert(msg.type == MessageType::HEARTBEAT);
    assert(msg.require_ack == false);

    auto parsed = HeartbeatMessage::from_message(msg);
    assert(parsed.has_value());
    assert(parsed->client_timestamp == hb.client_timestamp);

    std::cout << "✓ Heartbeat message passed" << std::endl;
}

void test_chat_message() {
    std::cout << "\n[Test 5] Chat message..." << std::endl;

    ChatMessage chat;
    chat.to_user = "bob";
    chat.content = "Hello, how are you?";
    chat.priority = MessagePriority::NORMAL;
    chat.require_ack = true;

    Message msg = chat.to_message("alice");
    std::cout << "  Message: " << msg.serialize() << std::endl;

    assert(msg.type == MessageType::MESSAGE);
    assert(msg.from_user == "alice");
    assert(msg.to_user == "bob");
    assert(msg.content == chat.content);

    auto parsed = ChatMessage::from_message(msg);
    assert(parsed.has_value());
    assert(parsed->to_user == chat.to_user);
    assert(parsed->content == chat.content);

    std::cout << "✓ Chat message passed" << std::endl;
}

void test_ack_message() {
    std::cout << "\n[Test 6] ACK message..." << std::endl;

    AckMessage ack;
    ack.ack_message_id = "msg_to_ack_123";
    ack.success = true;

    Message msg = ack.to_message();
    std::cout << "  Message: " << msg.serialize() << std::endl;

    assert(msg.type == MessageType::ACK);
    assert(msg.ack_id == ack.ack_message_id);

    auto parsed = AckMessage::from_message(msg);
    assert(parsed.has_value());
    assert(parsed->ack_message_id == ack.ack_message_id);
    assert(parsed->success == true);

    std::cout << "✓ ACK message passed" << std::endl;
}

void test_response_message() {
    std::cout << "\n[Test 7] Response message..." << std::endl;

    ResponseMessage resp;
    resp.success = true;
    resp.message = "Operation successful";
    resp.data["count"] = 42;
    resp.data["status"] = "ok";

    Message msg = resp.to_message();
    std::cout << "  Message: " << msg.serialize() << std::endl;

    assert(msg.type == MessageType::RESPONSE);
    assert(msg.content == resp.message);

    auto parsed = ResponseMessage::from_message(msg);
    assert(parsed.has_value());
    assert(parsed->success == true);
    assert(parsed->message == resp.message);

    std::cout << "✓ Response message passed" << std::endl;
}

int main() {
    std::cout << "=== Protocol Test ===" << std::endl;
    std::cout << std::endl;

    test_message_id_generation();
    test_message_serialization();
    test_connect_message();
    test_heartbeat_message();
    test_chat_message();
    test_ack_message();
    test_response_message();

    std::cout << "\n=== All tests passed ===" << std::endl;
    return 0;
}
