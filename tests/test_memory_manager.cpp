#include <iostream>
#include <cassert>
#include "../src/memory/memory_manager.h"

using namespace llm::memory;

void test_basic_allocation() {
    std::cout << "Testing basic allocation..." << std::endl;
    HeapMemoryManager mgr(1000);
    MemoryStats stats = mgr.get_stats();
    assert(stats.total_bytes == 1000);
    assert(stats.allocated_bytes == 0);
    assert(stats.free_bytes == 1000);
    assert(stats.pressure == MemoryPressure::NORMAL);
    
    // Allocate weights
    void* w = mgr.allocate_weights(200);
    assert(w != nullptr);
    stats = mgr.get_stats();
    assert(stats.allocated_bytes == 200);
    assert(stats.free_bytes == 800);
    
    // Allocate temp
    void* t = mgr.allocate_temp(100);
    assert(t != nullptr);
    stats = mgr.get_stats();
    assert(stats.allocated_bytes == 300);
    assert(stats.free_bytes == 700);
    
    // Allocate kv_cache
    void* k = mgr.allocate_kv_cache(100, KVCacheCompressionStrategy::NONE);
    assert(k != nullptr);
    stats = mgr.get_stats();
    assert(stats.allocated_bytes == 400);
    assert(stats.free_bytes == 600);
    
    // Deallocate
    mgr.deallocate(w);
    stats = mgr.get_stats();
    assert(stats.allocated_bytes == 200);
    assert(stats.free_bytes == 800);
    
    mgr.deallocate(t);
    mgr.deallocate(k);
    stats = mgr.get_stats();
    assert(stats.allocated_bytes == 0);
    assert(stats.free_bytes == 1000);
    
    std::cout << "✓ Basic allocation tests passed." << std::endl;
}

void test_memory_pressure_and_callbacks() {
    std::cout << "Testing memory pressure and callbacks..." << std::endl;
    HeapMemoryManager mgr(1000);
    
    MemoryPressure last_pressure = MemoryPressure::NORMAL;
    int callback_count = 0;
    
    mgr.register_pressure_callback([&](MemoryPressure p) {
        last_pressure = p;
        callback_count++;
    });
    
    // Pressure limits:
    // < 0.5: NORMAL
    // < 0.7: MODERATE
    // < 0.9: HIGH
    // >= 0.9: CRITICAL
    
    // 1. MODERATE pressure (e.g. 550 bytes / 55%)
    void* p1 = mgr.allocate_weights(550);
    assert(p1 != nullptr);
    assert(last_pressure == MemoryPressure::MODERATE);
    assert(callback_count == 1);
    
    // 2. HIGH pressure (e.g. 750 bytes / 75%)
    void* p2 = mgr.allocate_weights(200); // total 750
    assert(p2 != nullptr);
    assert(last_pressure == MemoryPressure::HIGH);
    assert(callback_count == 2);
    
    // 3. CRITICAL pressure (e.g. 950 bytes / 95%)
    void* p3 = mgr.allocate_weights(200); // total 950
    assert(p3 != nullptr);
    assert(last_pressure == MemoryPressure::CRITICAL);
    assert(callback_count == 3);
    
    // 4. Out of Memory (OOM)
    void* p4 = mgr.allocate_weights(100); // 950 + 100 = 1050 > 1000 -> should fail
    assert(p4 == nullptr);
    assert(callback_count == 3); // no change in allocation, no callback
    
    // 5. Deallocate and check pressure decrease
    mgr.deallocate(p3); // total 750 (HIGH)
    assert(last_pressure == MemoryPressure::HIGH);
    assert(callback_count == 4);
    
    mgr.deallocate(p2); // total 550 (MODERATE)
    assert(last_pressure == MemoryPressure::MODERATE);
    assert(callback_count == 5);
    
    mgr.deallocate(p1); // total 0 (NORMAL)
    assert(last_pressure == MemoryPressure::NORMAL);
    assert(callback_count == 6);
    
    std::cout << "✓ Memory pressure and callback tests passed." << std::endl;
}

void test_stubs() {
    std::cout << "Testing stubs..." << std::endl;
    HeapMemoryManager mgr(1000);
    assert(mgr.evict_inactive_modules() == 0);
    assert(mgr.compress_kv_cache(KVCacheCompressionStrategy::QUANTIZED) == true);
    std::cout << "✓ Stub tests passed." << std::endl;
}

int main() {
    std::cout << "=== Memory Manager Test ===" << std::endl;
    test_basic_allocation();
    test_memory_pressure_and_callbacks();
    test_stubs();
    std::cout << "✓ All memory manager tests passed" << std::endl;
    return 0;
}
