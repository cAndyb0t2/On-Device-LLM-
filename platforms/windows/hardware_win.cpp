#include "hardware/hardware_detector.h"
#include <windows.h>
#include <memory>
#include <functional>

namespace llm::hardware {

class WindowsHardwareDetector : public HardwareDetector {
public:
    HardwareSpec detect() const override {
        HardwareSpec spec;
        spec.os = OSType::WINDOWS;
        
        // Detect CPU
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        spec.cpu_cores = sysinfo.dwNumberOfProcessors;
        spec.cpu_frequency_mhz = 2400;  // Placeholder
        
        // Detect RAM
        MEMORYSTATUSEX meminfo;
        meminfo.dwLength = sizeof(meminfo);
        GlobalMemoryStatusEx(&meminfo);
        spec.total_ram_bytes = meminfo.ullTotalPhys;
        spec.available_ram_bytes = meminfo.ullAvailPhys;
        
        // Detect GPU (simplified)
        spec.has_gpu = true;  // Most modern Windows systems have GPU
        spec.supported_backends = {BackendType::CPU, BackendType::DIRECTML, BackendType::CUDA};
        
        // Determine tier
        spec.tier = classify_tier(spec);
        
        // Features
        spec.has_fpu = true;
        spec.has_int8_acceleration = true;
        spec.supports_simd = true;
        
        return spec;
    }
    
    uint64_t get_available_ram() const override {
        MEMORYSTATUSEX meminfo;
        meminfo.dwLength = sizeof(meminfo);
        GlobalMemoryStatusEx(&meminfo);
        return meminfo.ullAvailPhys;
    }
    
    int get_battery_percentage() const override {
        return -1;  // Windows laptops may have battery, but not always
    }
    
    float get_device_temperature() const override {
        return 45.0f;  // Placeholder
    }
    
    bool is_in_foreground() const override {
        return true;  // Desktop assumption
    }
    
    void register_state_change_callback(
        std::function<void(const HardwareSpec&)> callback) override {
        // Windows callback implementation
        state_callback_ = callback;
    }
    
private:
    DeviceTier classify_tier(const HardwareSpec& spec) const {
        if (spec.available_ram_bytes < 4 * 1024 * 1024 * 1024ULL) {
            return DeviceTier::LOW_END;
        } else if (spec.available_ram_bytes < 16 * 1024 * 1024 * 1024ULL) {
            return DeviceTier::MID_RANGE;
        } else {
            return DeviceTier::HIGH_END;
        }
    }
    
    std::function<void(const HardwareSpec&)> state_callback_;
};

std::unique_ptr<HardwareDetector> CreateWindowsHardwareDetector() {
    return std::make_unique<WindowsHardwareDetector>();
}

}  // namespace llm::hardware
