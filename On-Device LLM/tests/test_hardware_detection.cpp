#include <iostream>
#include <cassert>
#include "../src/hardware/hardware_detector.h"
#include "../src/core/runtime.h"

using namespace llm::hardware;

int main() {
    std::cout << "=== Hardware Detection Test ===" << std::endl;
    
    auto detector = CreateNativeHardwareDetector();
    assert(detector != nullptr);
    
    HardwareSpec spec = detector->detect();
    
    std::cout << "Platform: " << static_cast<int>(spec.os) << std::endl;
    std::cout << "CPU Cores: " << spec.cpu_cores << std::endl;
    std::cout << "Total RAM: " << (spec.total_ram_bytes / (1024*1024*1024)) << " GB" << std::endl;
    std::cout << "Available RAM: " << (spec.available_ram_bytes / (1024*1024*1024)) << " GB" << std::endl;
    std::cout << "Device Tier: " << static_cast<int>(spec.tier) << std::endl;
    
    // Verify tier classification
    assert(spec.tier != DeviceTier::LOW_END || 
           spec.available_ram_bytes < 4 * 1024 * 1024 * 1024ULL);
    
    std::cout << "✓ All hardware detection tests passed" << std::endl;
    
    std::cout << "\n=== LLM Runtime Lazy Allocation Test ===" << std::endl;
    auto runtime = llm::core::LLMRuntime::create();
    assert(runtime != nullptr);
    
    // Before initialization, memory should be 0
    auto stats_before = runtime->get_memory_stats();
    std::cout << "Initial allocated memory: " << stats_before.allocated_bytes << " bytes" << std::endl;
    assert(stats_before.allocated_bytes == 0);
    
    // Initialize runtime
    bool init_ok = runtime->initialize("models/base_7b.gguf");
    assert(init_ok);
    
    // After initialization, memory should still be 0 (lazy)
    auto stats_init = runtime->get_memory_stats();
    std::cout << "Memory after initialization: " << stats_init.allocated_bytes << " bytes" << std::endl;
    assert(stats_init.allocated_bytes == 0);
    
    // Call generate (triggers load, allocation, execution, unload, deallocation)
    std::string response = runtime->generate("Translate 'hello' to French", 100);
    std::cout << "Generate response: " << response << std::endl;
    assert(!response.empty());
    
    // After generate, memory should be fully released (back to 0)
    auto stats_after = runtime->get_memory_stats();
    std::cout << "Memory after generate: " << stats_after.allocated_bytes << " bytes" << std::endl;
    assert(stats_after.allocated_bytes == 0);
    
    // Call generate_streaming
    bool stream_ok = runtime->generate_streaming("Explain AI", [](const std::string& token) {
        std::cout << token << std::flush;
    });
    std::cout << std::endl;
    assert(stream_ok);
    
    // After generate_streaming, memory should be back to 0
    auto stats_stream_after = runtime->get_memory_stats();
    std::cout << "Memory after streaming: " << stats_stream_after.allocated_bytes << " bytes" << std::endl;
    assert(stats_stream_after.allocated_bytes == 0);
    
    std::cout << "✓ All LLM runtime lazy allocation tests passed" << std::endl;
    
    return 0;
}
