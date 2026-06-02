#include "include/auth.hpp"
#include <iostream>

int main() {
    gateway::AuthManager::instance().set_secret("test_secret_key_123");
    
    std::string token = gateway::AuthManager::instance().generate_token("user_test");
    std::cout << "Generated: " << token << std::endl;
    
    auto user_id = gateway::AuthManager::instance().verify_token(token);
    if (user_id) {
        std::cout << "Verified: " << *user_id << std::endl;
    } else {
        std::cout << "Verification failed" << std::endl;
        
        // 手动验证
        auto jwt = gateway::JWTToken::verify(token, "test_secret_key_123");
        if (jwt) {
            std::cout << "Manual verify OK: " << jwt->user_id << std::endl;
        } else {
            std::cout << "Manual verify failed too" << std::endl;
        }
    }
    
    return 0;
}
