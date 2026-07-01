#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include <functional>

namespace llm::modules {

enum class ModuleType {
    BASE_MODEL,      // Core LLM (always loaded)
    REASONING,       // Extended chain-of-thought
    RETRIEVAL,       // RAG/knowledge retrieval
    MULTIMODAL,      // Image/audio processing
    TOOL_USE,        // Function calling
    VISION,          // Vision transformer
    SPEECH           // Speech recognition/synthesis
};

struct ModuleConfig {
    ModuleType type;
    std::string model_path;
    uint64_t memory_estimate_bytes;
    uint32_t priority;  // Higher = load first
    bool optional;
};

class Module {
public:
    virtual ~Module() = default;
    
    /// Get module type
    virtual ModuleType get_type() const = 0;
    
    /// Get module name
    virtual std::string get_name() const = 0;
    
    /// Load module weights into memory
    virtual bool load() = 0;
    
    /// Unload module from memory (frees resources)
    virtual bool unload() = 0;
    
    /// Check if module is currently loaded
    virtual bool is_loaded() const = 0;
    
    /// Get memory footprint in bytes
    virtual uint64_t get_memory_footprint() const = 0;
    
    /// Module-specific inference (overridden by subclasses)
    virtual bool execute(const std::string& input, std::string* output) = 0;
};

class ModuleManager {
public:
    virtual ~ModuleManager() = default;
    
    /// Register a module
    virtual void register_module(const ModuleConfig& config) = 0;
    
    /// Request to load a module (may defer if low memory)
    virtual bool load_module(ModuleType type) = 0;
    
    /// Unload a module
    virtual void unload_module(ModuleType type) = 0;
    
    /// Check if module is loaded
    virtual bool is_module_loaded(ModuleType type) const = 0;
    
    /// Get module by type
    virtual Module* get_module(ModuleType type) = 0;
    
    /// Load all essential modules for current hardware tier
    virtual bool load_tier_defaults() = 0;
    
    /// Unload modules to free memory (returns freed bytes)
    virtual uint64_t unload_inactive_modules() = 0;
};

}  // namespace llm::modules
