#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace llm::privacy {

/// User memory storage with encryption
class SecureMemoryStore {
public:
    virtual ~SecureMemoryStore() = default;
    
    /// Initialize with encryption key (or derive from password)
    virtual bool initialize(const std::string& encryption_password) = 0;
    
    /// Store a message/conversation turn
    virtual bool store_message(
        const std::string& conversation_id,
        const std::string& role,           // "user" or "assistant"
        const std::string& content) = 0;
    
    /// Retrieve conversation history
    virtual std::vector<std::pair<std::string, std::string>> get_conversation(
        const std::string& conversation_id,
        uint32_t max_turns = 10) = 0;
    
    /// Delete a conversation
    virtual bool delete_conversation(const std::string& conversation_id) = 0;
    
    /// Export all conversations (for backup/export, returns encrypted blob)
    virtual std::string export_encrypted_data() const = 0;
    
    /// Clear all memory (factory reset)
    virtual bool clear_all_data() = 0;
};

/// Privacy enforcement policy
struct PrivacyPolicy {
    bool allow_cloud_sync = false;
    bool allow_analytics = false;
    bool require_encryption = true;
    bool disable_network_access = false;
    uint32_t data_retention_days = 30;  // 0 = never delete
    bool allow_external_model_calls = false;
};

/// Privacy controller
class PrivacyManager {
public:
    virtual ~PrivacyManager() = default;
    
    /// Set privacy policy
    virtual void set_policy(const PrivacyPolicy& policy) = 0;
    
    /// Get current policy
    virtual PrivacyPolicy get_policy() const = 0;
    
    /// Check if operation is allowed by current policy
    virtual bool is_operation_allowed(const std::string& operation) const = 0;
    
    /// Get user memory store
    virtual SecureMemoryStore* get_memory_store() = 0;
    
    /// Audit log (encrypted)
    virtual void log_operation(const std::string& operation, bool allowed) = 0;
};

}  // namespace llm::privacy
