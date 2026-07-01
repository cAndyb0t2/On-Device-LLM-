#include "hardware_detector.h"
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
    #include <powerbase.h>
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #include <sys/sysctl.h>
    #include <sys/types.h>
    #if TARGET_OS_MAC
        // macOS includes
    #elif TARGET_OS_IPHONE
        // iOS includes
    #endif
#elif defined(__ANDROID__)
    #include <android/api-level.h>
#elif defined(__linux__)
    #include <unistd.h>
#endif

namespace llm::hardware {

#ifdef _WIN32
std::unique_ptr<HardwareDetector> CreateWindowsHardwareDetector();
#endif

namespace {

#if defined(__APPLE__)

class AppleHardwareDetector : public HardwareDetector {
public:
    HardwareSpec detect() const override {
        HardwareSpec spec;
        
        #if TARGET_OS_IPHONE
        spec.os = OSType::IOS;
        spec.supported_backends = {BackendType::CPU, BackendType::COREML, BackendType::METAL};
        #else
        spec.os = OSType::MACOS;
        spec.supported_backends = {BackendType::CPU, BackendType::COREML, BackendType::METAL};
        #endif
        
        // CPU info via sysctl
        int nprocs;
        size_t len = sizeof(nprocs);
        sysctlbyname("hw.logicalcpu", &nprocs, &len, nullptr, 0);
        spec.cpu_cores = nprocs;
        
        // RAM info
        long pages = sysconf(_SC_PHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);
        spec.total_ram_bytes = pages * page_size;
        spec.available_ram_bytes = spec.total_ram_bytes / 2;  // Simplified
        
        spec.has_gpu = true;
        spec.tier = classify_tier(spec);
        
        return spec;
    }
    
    uint64_t get_available_ram() const override {
        // macOS/iOS specific implementation
        return 0;
    }
    
    int get_battery_percentage() const override {
        #if TARGET_OS_IPHONE
        return 100;  // Placeholder
        #else
        return -1;
        #endif
    }
    
    float get_device_temperature() const override {
        return 40.0f;
    }
    
    bool is_in_foreground() const override {
        #if TARGET_OS_IPHONE
        return true;  // Implement via UIApplication state
        #else
        return true;
        #endif
    }
    
    void register_state_change_callback(
        std::function<void(const HardwareSpec&)> callback) override {
        // Apple platform callback
    }
    
private:
    DeviceTier classify_tier(const HardwareSpec& spec) const {
        if (spec.available_ram_bytes < 2 * 1024 * 1024 * 1024ULL) {
            return DeviceTier::LOW_END;
        } else if (spec.available_ram_bytes < 8 * 1024 * 1024 * 1024ULL) {
            return DeviceTier::MID_RANGE;
        } else {
            return DeviceTier::HIGH_END;
        }
    }
};

#elif defined(__ANDROID__)

class AndroidHardwareDetector : public HardwareDetector {
public:
    HardwareSpec detect() const override {
        HardwareSpec spec;
        spec.os = OSType::ANDROID;
        spec.is_battery_powered = true;
        spec.supported_backends = {BackendType::CPU, BackendType::NNAPI, BackendType::VULKAN};
        
        // CPU cores
        spec.cpu_cores = sysconf(_SC_NPROCESSORS_ONLN);
        
        // RAM from /proc/meminfo
        spec.total_ram_bytes = 2 * 1024 * 1024 * 1024ULL;  // Placeholder
        spec.available_ram_bytes = 1 * 1024 * 1024 * 1024ULL;
        
        spec.has_gpu = true;
        spec.tier = classify_tier(spec);
        
        return spec;
    }
    
    uint64_t get_available_ram() const override {
        return 1 * 1024 * 1024 * 1024ULL;  // Placeholder
    }
    
    int get_battery_percentage() const override {
        return 75;  // Placeholder
    }
    
    float get_device_temperature() const override {
        return 38.0f;
    }
    
    bool is_in_foreground() const override {
        return true;  // Implement via Android lifecycle
    }
    
    void register_state_change_callback(
        std::function<void(const HardwareSpec&)> callback) override {
        // Android callback via JNI
    }
    
private:
    DeviceTier classify_tier(const HardwareSpec& spec) const {
        if (spec.total_ram_bytes < 3 * 1024 * 1024 * 1024ULL) {
            return DeviceTier::LOW_END;
        } else if (spec.total_ram_bytes < 8 * 1024 * 1024 * 1024ULL) {
            return DeviceTier::MID_RANGE;
        } else {
            return DeviceTier::HIGH_END;
        }
    }
};

#elif defined(__linux__)

class LinuxHardwareDetector : public HardwareDetector {
public:
    HardwareSpec detect() const override {
        HardwareSpec spec;
        spec.os = OSType::LINUX;
        spec.supported_backends = {BackendType::CPU, BackendType::CUDA, BackendType::VULKAN};
        
        spec.cpu_cores = sysconf(_SC_NPROCESSORS_ONLN);
        
        long pages = sysconf(_SC_PHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);
        spec.total_ram_bytes = pages * page_size;
        spec.available_ram_bytes = spec.total_ram_bytes / 2;
        
        spec.tier = classify_tier(spec);
        
        return spec;
    }
    
    uint64_t get_available_ram() const override {
        return 0;  // Read from /proc/meminfo
    }
    
    int get_battery_percentage() const override {
        return -1;
    }
    
    float get_device_temperature() const override {
        return 45.0f;
    }
    
    bool is_in_foreground() const override {
        return true;
    }
    
    void register_state_change_callback(
        std::function<void(const HardwareSpec&)> callback) override {
        // Linux event monitoring
    }
    
private:
    DeviceTier classify_tier(const HardwareSpec& spec) const {
        if (spec.total_ram_bytes < 4 * 1024 * 1024 * 1024ULL) {
            return DeviceTier::LOW_END;
        } else if (spec.total_ram_bytes < 16 * 1024 * 1024 * 1024ULL) {
            return DeviceTier::MID_RANGE;
        } else {
            return DeviceTier::HIGH_END;
        }
    }
};

#else

class FallbackHardwareDetector : public HardwareDetector {
public:
    HardwareSpec detect() const override {
        HardwareSpec spec;
        spec.os = OSType::UNKNOWN;
        spec.cpu_cores = 4;
        spec.total_ram_bytes = 4 * 1024 * 1024 * 1024ULL;
        spec.available_ram_bytes = 2 * 1024 * 1024 * 1024ULL;
        spec.supported_backends = {BackendType::CPU};
        spec.tier = DeviceTier::MID_RANGE;
        return spec;
    }
    
    uint64_t get_available_ram() const override { return 2 * 1024 * 1024 * 1024ULL; }
    int get_battery_percentage() const override { return -1; }
    float get_device_temperature() const override { return 40.0f; }
    bool is_in_foreground() const override { return true; }
    void register_state_change_callback(std::function<void(const HardwareSpec&)>) override {}
};

#endif // platform detectors

}  // anonymous namespace

std::unique_ptr<HardwareDetector> CreateNativeHardwareDetector() {
#ifdef _WIN32
    return CreateWindowsHardwareDetector();
#elif defined(__APPLE__)
    return std::make_unique<AppleHardwareDetector>();
#elif defined(__ANDROID__)
    return std::make_unique<AndroidHardwareDetector>();
#elif defined(__linux__)
    return std::make_unique<LinuxHardwareDetector>();
#else
    return std::make_unique<FallbackHardwareDetector>();
#endif
}

}  // namespace llm::hardware
