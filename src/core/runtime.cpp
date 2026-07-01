#include "runtime.h"
#include <iostream>

namespace llm::core {

class LLMRuntimeImpl : public LLMRuntime {
public:
    LLMRuntimeImpl() = default;
    
    ~LLMRuntimeImpl() {
        shutdown();
    }
    
    bool initialize(const std::string& model_path) override {
        model_path_ = model_path;
        
        // Initialize hardware detector
        hardware_detector_ = hardware::CreateNativeHardwareDetector();
        hardware_spec_ = hardware_detector_->detect();
        
        std::cout << "Detected hardware tier: "
                  << static_cast<int>(hardware_spec_.tier) << std::endl;
        
        // Initialize memory manager with detected available RAM
        memory_manager_ = std::make_unique<memory::HeapMemoryManager>(
            hardware_spec_.available_ram_bytes);
        
        return true;
    }
    
    std::string generate(const std::string& prompt, uint32_t max_tokens) override {
        std::cout << "\n--- Starting Generation ---" << std::endl;
        std::cout << "Memory stats before generation:" << std::endl;
        print_memory_stats();
        
        // 1. Lazy load backend and allocate required RAM
        if (!load_backend_resources()) {
            return "Error: Failed to allocate required RAM to run inference.";
        }
        
        std::cout << "Memory stats during generation (RAM allocated):" << std::endl;
        print_memory_stats();
        
        // 2. Perform mock generation
        std::string response = "Generated response (stub) using backend: " + backend_->get_name();
        std::cout << "Inference completed. Response: \"" << response << "\"" << std::endl;
        
        // 3. Deallocate memory immediately
        unload_backend_resources();
        
        std::cout << "Memory stats after generation (RAM released):" << std::endl;
        print_memory_stats();
        std::cout << "---------------------------\n" << std::endl;
        
        return response;
    }
    
    bool generate_streaming(
        const std::string& prompt,
        std::function<void(const std::string&)> on_token) override {
        
        std::cout << "\n--- Starting Streaming Generation ---" << std::endl;
        std::cout << "Memory stats before streaming:" << std::endl;
        print_memory_stats();
        
        // 1. Lazy load backend and allocate required RAM
        if (!load_backend_resources()) {
            return false;
        }
        
        std::cout << "Memory stats during streaming (RAM allocated):" << std::endl;
        print_memory_stats();
        
        // 2. Stream tokens
        if (on_token) {
            on_token("Token1");
            on_token(" Token2");
            on_token(" (streamed from " + backend_->get_name() + ")");
        }
        
        // 3. Deallocate memory immediately
        unload_backend_resources();
        
        std::cout << "Memory stats after streaming (RAM released):" << std::endl;
        print_memory_stats();
        std::cout << "-------------------------------------\n" << std::endl;
        
        return true;
    }
    
    hardware::HardwareSpec get_device_spec() const override {
        return hardware_spec_;
    }
    
    memory::MemoryStats get_memory_stats() const override {
        if (!memory_manager_) {
            memory::MemoryStats stats{};
            stats.total_bytes = 0;
            stats.allocated_bytes = 0;
            stats.weights_bytes = 0;
            stats.kv_cache_bytes = 0;
            stats.temp_buffer_bytes = 0;
            stats.free_bytes = 0;
            stats.pressure = memory::MemoryPressure::NORMAL;
            return stats;
        }
        return memory_manager_->get_stats();
    }
    
    void shutdown() override {
        unload_backend_resources();
    }
    
    hal::InferenceBackend* get_backend() override {
        return backend_.get();
    }
    
    modules::ModuleManager* get_module_manager() override {
        return module_manager_.get();
    }
    
    scheduler::AdaptiveScheduler* get_scheduler() override {
        return scheduler_.get();
    }
    
    privacy::PrivacyManager* get_privacy_manager() override {
        return privacy_manager_.get();
    }
    
private:
    bool load_backend_resources() {
        // Determine RAM sizes depending on Hardware Tier
        uint64_t weight_size = 0;
        uint64_t kv_size = 0;
        
        switch (hardware_spec_.tier) {
            case hardware::DeviceTier::LOW_END:
                weight_size = 256 * 1024 * 1024ULL;  // 256 MB
                kv_size = 32 * 1024 * 1024ULL;       // 32 MB
                break;
            case hardware::DeviceTier::MID_RANGE:
                weight_size = 1024 * 1024 * 1024ULL; // 1 GB
                kv_size = 128 * 1024 * 1024ULL;      // 128 MB
                break;
            case hardware::DeviceTier::HIGH_END:
                weight_size = 2048 * 1024 * 1024ULL; // 2 GB
                kv_size = 256 * 1024 * 1024ULL;      // 256 MB
                break;
        }
        
        std::cout << "Attempting to allocate " << (weight_size / (1024*1024)) 
                  << " MB for weights and " << (kv_size / (1024*1024)) 
                  << " MB for KV cache..." << std::endl;
                  
        // Allocate weights
        allocated_ram_ptr_ = memory_manager_->allocate_weights(weight_size, true);
        if (!allocated_ram_ptr_) {
            std::cerr << "OOM: Failed to allocate weight memory." << std::endl;
            return false;
        }
        
        // Allocate KV cache
        allocated_kv_ptr_ = memory_manager_->allocate_kv_cache(kv_size, memory::KVCacheCompressionStrategy::NONE);
        if (!allocated_kv_ptr_) {
            std::cerr << "OOM: Failed to allocate KV Cache memory." << std::endl;
            // Clean up weights
            memory_manager_->deallocate(allocated_ram_ptr_);
            allocated_ram_ptr_ = nullptr;
            return false;
        }
        
        // Convert BackendType enums to strings
        std::vector<std::string> backend_strs;
        for (auto b : hardware_spec_.supported_backends) {
            switch (b) {
                case hardware::BackendType::CPU: backend_strs.push_back("CPU"); break;
                case hardware::BackendType::CUDA: backend_strs.push_back("CUDA"); break;
                case hardware::BackendType::DIRECTML: backend_strs.push_back("DIRECTML"); break;
                case hardware::BackendType::VULKAN: backend_strs.push_back("VULKAN"); break;
                case hardware::BackendType::METAL: backend_strs.push_back("METAL"); break;
                case hardware::BackendType::NNAPI: backend_strs.push_back("NNAPI"); break;
                case hardware::BackendType::COREML: backend_strs.push_back("COREML"); break;
                case hardware::BackendType::ONNX_MOBILE: backend_strs.push_back("ONNX_MOBILE"); break;
            }
        }
        
        // Create backend
        backend_ = hal::BackendFactory::create_optimal(backend_strs);
        if (!backend_) {
            std::cerr << "Failed to create optimal backend." << std::endl;
            unload_backend_resources();
            return false;
        }
        
        // Initialize backend
        uint32_t max_ctx = (hardware_spec_.tier == hardware::DeviceTier::LOW_END) ? 512 : 2048;
        if (!backend_->initialize(model_path_, max_ctx)) {
            std::cerr << "Failed to initialize backend." << std::endl;
            unload_backend_resources();
            return false;
        }
        
        return true;
    }
    
    void unload_backend_resources() {
        if (allocated_ram_ptr_) {
            std::cout << "Deallocating backend weight memory..." << std::endl;
            memory_manager_->deallocate(allocated_ram_ptr_);
            allocated_ram_ptr_ = nullptr;
        }
        if (allocated_kv_ptr_) {
            std::cout << "Deallocating KV Cache memory..." << std::endl;
            memory_manager_->deallocate(allocated_kv_ptr_);
            allocated_kv_ptr_ = nullptr;
        }
        backend_.reset();
    }
    
    void print_memory_stats() const {
        auto stats = memory_manager_->get_stats();
        std::cout << "  Total RAM: " << (stats.total_bytes / (1024 * 1024)) << " MB" << std::endl;
        std::cout << "  Allocated: " << (stats.allocated_bytes / (1024 * 1024)) << " MB" << std::endl;
        std::cout << "  Free:      " << (stats.free_bytes / (1024 * 1024)) << " MB" << std::endl;
        std::cout << "  Pressure:  " << static_cast<int>(stats.pressure) << std::endl;
    }

    std::string model_path_;
    void* allocated_ram_ptr_ = nullptr;
    void* allocated_kv_ptr_ = nullptr;
    
    std::unique_ptr<hardware::HardwareDetector> hardware_detector_;
    hardware::HardwareSpec hardware_spec_;
    std::unique_ptr<memory::MemoryManager> memory_manager_;
    std::unique_ptr<hal::InferenceBackend> backend_;
    std::unique_ptr<modules::ModuleManager> module_manager_;
    std::unique_ptr<scheduler::AdaptiveScheduler> scheduler_;
    std::unique_ptr<privacy::PrivacyManager> privacy_manager_;
};

std::unique_ptr<LLMRuntime> LLMRuntime::create() {
    return std::make_unique<LLMRuntimeImpl>();
}

}  // namespace llm::core
