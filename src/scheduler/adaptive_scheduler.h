#pragma once

#include <chrono>
#include <cstdint>
#include <functional>

namespace llm::scheduler {

enum class ThermalState {
    NORMAL,      // <60°C
    WARM,        // 60-75°C
    HOT,         // 75-85°C
    CRITICAL     // >85°C
};

enum class PowerState {
    AC_POWER,
    BATTERY_HIGH,    // >50%
    BATTERY_MEDIUM,  // 20-50%
    BATTERY_LOW,     // <20%
    BATTERY_CRITICAL // <5%
};

struct InferenceSchedule {
    uint32_t max_context_length;
    uint32_t batch_size;
    float compute_budget_ms;  // Max time per forward pass
    bool use_reduced_precision;
    bool pause_inference;
};

/// Adaptive inference scheduler that adjusts behavior based on device state
class AdaptiveScheduler {
public:
    virtual ~AdaptiveScheduler() = default;
    
    /// Get current thermal state
    virtual ThermalState get_thermal_state() const = 0;
    
    /// Get current power state
    virtual PowerState get_power_state() const = 0;
    
    /// Get recommended inference schedule based on current constraints
    virtual InferenceSchedule get_recommended_schedule() const = 0;
    
    /// Check if inference should be paused (thermal/battery/foreground state)
    virtual bool should_pause_inference() const = 0;
    
    /// Apply throttling to reduce device strain (increase latency)
    virtual bool apply_throttling(float reduction_factor) = 0;
    
    /// Estimate remaining battery life at current power draw
    virtual std::chrono::minutes estimate_battery_life() const = 0;
    
    /// Update device state (call periodically)
    virtual void update_device_state() = 0;
    
    /// Register callback for when device state changes
    virtual void register_state_change_callback(
        std::function<void(ThermalState, PowerState)> callback) = 0;
};

}  // namespace llm::scheduler
