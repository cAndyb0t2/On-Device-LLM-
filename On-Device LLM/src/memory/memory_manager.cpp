#include "memory_manager.h"
#include <algorithm>
#include <iostream>

namespace llm::memory {

HeapMemoryManager::HeapMemoryManager(uint64_t max_memory_bytes)
    : max_memory_(max_memory_bytes), allocated_(0) {}

void* HeapMemoryManager::allocate_weights(uint64_t size, bool use_mmap) {
    if (allocated_ + size > max_memory_) {
        std::cerr << "Memory allocation failed: " << size << " bytes requested, "
                  << (max_memory_ - allocated_) << " bytes available" << std::endl;
        return nullptr;
    }
    
    void* ptr = new uint8_t[size];
    allocations_.push_back({ptr, size});
    allocated_ += size;
    
    if (pressure_callback_) {
        pressure_callback_(calculate_pressure());
    }
    
    return ptr;
}

void* HeapMemoryManager::allocate_temp(uint64_t size) {
    return allocate_weights(size, false);
}

void* HeapMemoryManager::allocate_kv_cache(
    uint64_t size,
    KVCacheCompressionStrategy strategy) {
    return allocate_weights(size, false);
}

void HeapMemoryManager::deallocate(void* ptr) {
    auto it = std::find_if(allocations_.begin(), allocations_.end(),
        [ptr](const auto& p) { return p.first == ptr; });
    
    if (it != allocations_.end()) {
        allocated_ -= it->second;
        delete[] static_cast<uint8_t*>(ptr);
        allocations_.erase(it);
        
        if (pressure_callback_) {
            pressure_callback_(calculate_pressure());
        }
    }
}

MemoryStats HeapMemoryManager::get_stats() const {
    MemoryStats stats;
    stats.total_bytes = max_memory_;
    stats.allocated_bytes = allocated_;
    stats.free_bytes = max_memory_ - allocated_;
    stats.pressure = calculate_pressure();
    return stats;
}

uint64_t HeapMemoryManager::evict_inactive_modules() {
    // Stub: In real implementation, unload optional modules
    return 0;
}

bool HeapMemoryManager::compress_kv_cache(KVCacheCompressionStrategy strategy) {
    // Stub: Compress KV cache based on strategy
    return true;
}

void HeapMemoryManager::register_pressure_callback(
    std::function<void(MemoryPressure)> callback) {
    pressure_callback_ = callback;
}

MemoryPressure HeapMemoryManager::calculate_pressure() const {
    float usage_ratio = static_cast<float>(allocated_) / max_memory_;
    
    if (usage_ratio < 0.5f) return MemoryPressure::NORMAL;
    if (usage_ratio < 0.7f) return MemoryPressure::MODERATE;
    if (usage_ratio < 0.9f) return MemoryPressure::HIGH;
    return MemoryPressure::CRITICAL;
}

}  // namespace llm::memory
