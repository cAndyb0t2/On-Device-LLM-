#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <functional>

namespace llm::hardware {

enum class DeviceTier {
    LOW_END,    // 1B-3B params: <4GB RAM
    MID_RANGE,  // 3B-7B params: 4-16GB RAM
    HIGH_END    // 7B-13B+ params: 16GB+ RAM
};

enum class BackendType {
    CPU,        // Universal fallback (llama.cpp / ONNX Runtime)
    CUDA,       // NVIDIA GPUs
    DIRECTML,   // Windows AMD/Intel
    VULKAN,     // Cross-platform
    METAL,      // Apple Silicon / macOS
    NNAPI,      // Android Neural Networks API
    COREML,     // iOS / macOS
    ONNX_MOBILE // Mobile ONNX Runtime
};

enum class OSType {
    ANDROID,
    IOS,
    WINDOWS,
    MACOS,
    LINUX,
    RASPBIAN,
    UNKNOWN
};

struct HardwareSpec {
    OSType os;
    std::string os_version;
    
    // CPU info
    uint32_t cpu_cores;
    uint32_t cpu_frequency_mhz;
    bool has_neon;  // ARM NEON
    bool has_sve;   // ARM SVE
    
    // Memory
    uint64_t total_ram_bytes;
    uint64_t available_ram_bytes;
    
    // GPU
    bool has_gpu;
    std::string gpu_name;
    uint64_t gpu_vram_bytes;
    std::vector<BackendType> supported_backends;
    
    // Thermal & Power
    bool is_battery_powered;
    int battery_percent;
    float current_temperature_celsius;
    float max_safe_temperature_celsius;
    
    // Features
    bool has_fpu;
    bool has_int8_acceleration;
    bool supports_simd;
    
    DeviceTier tier;
};

class HardwareDetector {
public:
    virtual ~HardwareDetector() = default;
    
    /// Detect current hardware capabilities and constraints
    virtual HardwareSpec detect() const = 0;
    
    /// Get current available RAM (changes over time)
    virtual uint64_t get_available_ram() const = 0;
    
    /// Get current battery percentage (0-100, or -1 if not battery-powered)
    virtual int get_battery_percentage() const = 0;
    
    /// Get current device temperature in Celsius
    virtual float get_device_temperature() const = 0;
    
    /// Check if device is in background/foreground
    virtual bool is_in_foreground() const = 0;
    
    /// Register callback for hardware state changes (e.g., thermal throttling, low memory)
    virtual void register_state_change_callback(
        std::function<void(const HardwareSpec&)> callback) = 0;
};

// Platform-specific factory
std::unique_ptr<HardwareDetector> CreateNativeHardwareDetector();

}  // namespace llm::hardware
