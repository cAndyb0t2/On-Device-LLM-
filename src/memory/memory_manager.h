#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <memory>
#include <functional>

namespace llm::memory {

/// KV cache compression strategy
enum class KVCacheCompressionStrategy {
    NONE,        // Store full KV cache
    QUANTIZED,   // 8-bit or 4-bit quantization
    SUMMARIZED,  // Summary tokens instead of full history
    EVICTED      // Drop oldest tokens
};

/// Memory pressure level
enum class MemoryPressure {
    NORMAL,   // >50% available
    MODERATE, // 30-50% available
    HIGH,     // 10-30% available
    CRITICAL  // <10% available
};

/// Statistics about memory usage
struct MemoryStats {
    uint64_t total_bytes;
    uint64_t allocated_bytes;
    uint64_t weights_bytes;
    uint64_t kv_cache_bytes;
    uint64_t temp_buffer_bytes;
    uint64_t free_bytes;
    MemoryPressure pressure;
};

class MemoryManager {
public:
    virtual ~MemoryManager() = default;
    
    /// Allocate memory for model weights (memory-mapped if possible)
    virtual void* allocate_weights(uint64_t size, bool use_mmap = true) = 0;
    
    /// Allocate temporary buffers for intermediate computations
    virtual void* allocate_temp(uint64_t size) = 0;
    
    /// Allocate KV cache with specified compression strategy
    virtual void* allocate_kv_cache(
        uint64_t size,
        KVCacheCompressionStrategy strategy) = 0;
    
    /// Deallocate any buffer
    virtual void deallocate(void* ptr) = 0;
    
    /// Get current memory statistics
    virtual MemoryStats get_stats() const = 0;
    
    /// Evict unused modules to free memory
    virtual uint64_t evict_inactive_modules() = 0;
    
    /// Compress KV cache (lossy operation)
    virtual bool compress_kv_cache(KVCacheCompressionStrategy strategy) = 0;
    
    /// Register callback for memory pressure changes
    virtual void register_pressure_callback(
        std::function<void(MemoryPressure)> callback) = 0;
};

/// Standard heap-based memory manager
class HeapMemoryManager : public MemoryManager {
public:
    explicit HeapMemoryManager(uint64_t max_memory_bytes);
    
    void* allocate_weights(uint64_t size, bool use_mmap = true) override;
    void* allocate_temp(uint64_t size) override;
    void* allocate_kv_cache(uint64_t size, KVCacheCompressionStrategy strategy) override;
    void deallocate(void* ptr) override;
    MemoryStats get_stats() const override;
    uint64_t evict_inactive_modules() override;
    bool compress_kv_cache(KVCacheCompressionStrategy strategy) override;
    void register_pressure_callback(std::function<void(MemoryPressure)> callback) override;
    
private:
    uint64_t max_memory_;
    uint64_t allocated_;
    std::vector<std::pair<void*, uint64_t>> allocations_;
    std::function<void(MemoryPressure)> pressure_callback_;
    
    MemoryPressure calculate_pressure() const;
};

}  // namespace llm::memory
