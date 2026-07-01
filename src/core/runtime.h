#pragma once

#include <string>
#include <memory>
#include <vector>
#include "hardware/hardware_detector.h"
#include "hal/backend.h"
#include "memory/memory_manager.h"
#include "modules/module_system.h"
#include "scheduler/adaptive_scheduler.h"
#include "privacy/privacy_manager.h"

namespace llm::core {

/// Main LLM inference runtime
class LLMRuntime {
public:
    /// Create runtime with auto-detected hardware
    static std::unique_ptr<LLMRuntime> create();
    
    /// Initialize runtime with model and config
    virtual bool initialize(const std::string& model_path) = 0;
    
    /// Generate text completion
    virtual std::string generate(
        const std::string& prompt,
        uint32_t max_tokens = 256) = 0;
    
    /// Stream-based generation with callback
    virtual bool generate_streaming(
        const std::string& prompt,
        std::function<void(const std::string& token)> on_token) = 0;
    
    /// Get current device profile
    virtual hardware::HardwareSpec get_device_spec() const = 0;
    
    /// Get memory usage stats
    virtual memory::MemoryStats get_memory_stats() const = 0;
    
    /// Shutdown and cleanup
    virtual void shutdown() = 0;
    
    /// Get inference backend
    virtual hal::InferenceBackend* get_backend() = 0;
    
    /// Get module manager
    virtual modules::ModuleManager* get_module_manager() = 0;
    
    /// Get scheduler
    virtual scheduler::AdaptiveScheduler* get_scheduler() = 0;
    
    /// Get privacy manager
    virtual privacy::PrivacyManager* get_privacy_manager() = 0;
};

}  // namespace llm::core
