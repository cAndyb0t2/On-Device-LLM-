# On-Device LLM Runtime — Copilot Instructions

## Project Overview

Universal, privacy-first AI assistant runtime that runs fully on-device across Android, iOS, Windows, macOS, Linux, and Raspberry Pi. Core features: adaptive hardware detection, modular architecture, memory-efficient inference, and privacy enforcement.

**GitHub**: https://github.com/your-org/on-device-llm

## Architecture Quick Reference

### Core Components

| Component | Location | Purpose |
|-----------|----------|---------|
| **Hardware Detection** | `src/hardware/` | Auto-detect device tier (low/mid/high) and capabilities |
| **HAL (Hardware Abstraction Layer)** | `src/hal/` | Backend selector for CPU, CUDA, Metal, NNAPI, CoreML, DirectML |
| **Memory Manager** | `src/memory/` | Allocation, KV cache compression, memory-mapped weights |
| **Module System** | `src/modules/` | Load/unload optional modules (reasoning, retrieval, multimodal) |
| **Adaptive Scheduler** | `src/scheduler/` | Monitor thermal, battery, foreground state → adjust inference |
| **Privacy Manager** | `src/privacy/` | Enforce offline-only, encrypt user memory, audit logging |
| **Runtime** | `src/core/runtime.h/cpp` | Main orchestrator, ties everything together |

### Platform Bindings

- **Android** (`platforms/android/`) — NNAPI, JNI, Vulkan
- **iOS** (`platforms/ios/`) — CoreML, Metal, Swift interface
- **Windows** (`platforms/windows/`) — DirectML, CUDA, WinAPI
- **Linux** (`platforms/linux/`) — CUDA, Vulkan, system libs
- **macOS** (`platforms/macos/`) — Metal, CoreML

## Development Workflow

### When Adding a New Hardware Backend

1. **Extend `src/hal/backend.h`** — Add new `BackendType` enum value
2. **Implement in `src/hal/backends/<name>_backend.h/cpp`**
   - Inherit from `InferenceBackend`
   - Implement `initialize()`, `forward()`, `prefill()`, `decode()`
   - Return memory usage in `get_memory_usage()`
3. **Register in `BackendFactory::create()`** — Add case for new backend
4. **Test** — `tests/test_<backend>_backend.cpp`
5. **Document** in README under "Multi-Platform Support"

### When Optimizing for a Device Tier

**Low-end (<4GB RAM):**
- Load only base model
- Context ≤512 tokens
- Use 4-bit quantization
- Skip optional modules
- See: `get_recommended_schedule()` in `AdaptiveScheduler`

**Mid-range (4–16GB):**
- Can load base + 1 optional module
- Context 2048 tokens
- 4-bit quantization acceptable
- Load retrieval or reasoning on demand

**High-end (16GB+):**
- Load base + multiple modules
- Context 4096 tokens
- FP16 or 8-bit quantization
- Full feature set enabled

### When Implementing Privacy Features

1. **Encrypted storage** → Extend `SecureMemoryStore` in `src/privacy/`
2. **Policy enforcement** → Add checks in `PrivacyManager::is_operation_allowed()`
3. **Audit logging** → Call `privacy_manager_->log_operation()` before sensitive ops
4. **Test** → `tests/test_privacy.cpp`

### When Adding Module Types

1. Add enum to `ModuleType` in `src/modules/module_system.h`
2. Create module implementation in `src/modules/<module_name>.h/cpp`
3. Register in `ModuleManager::register_module()`
4. Update `config/default_config.json` with memory estimate and priority

## Coding Standards

### C++ Style

- **Language**: C++17 (prefer standard library over custom code)
- **Namespaces**: All code under `llm::<subsystem>` (e.g., `llm::hal`, `llm::memory`)
- **Headers**: Use `#pragma once`, include guards minimal
- **Const correctness**: Enforce on methods and parameters
- **Error handling**: Return `bool` or use exceptions for critical paths
- **Documentation**: Doxygen-style comments for public APIs

### File Organization

```cpp
// Header: MyClass.h
#pragma once

namespace llm::subsystem {

class MyClass {
public:
    virtual ~MyClass() = default;
    
    virtual bool do_work() = 0;
};

}  // namespace llm::subsystem
```

```cpp
// Implementation: MyClass.cpp
#include "MyClass.h"

namespace llm::subsystem {

bool MyClass::do_work() {
    // Implementation
    return true;
}

}  // namespace llm::subsystem
```

### Python Style

- **PEP 8** — 4-space indents, type hints
- **Logging** — Use `logging` module, not `print()`
- **Error handling** — Explicit exception handling, meaningful messages

## Testing Strategy

### Unit Tests

Run individually:
```bash
cd build
./test_hardware_detection
./test_memory_manager
```

Add new test:
```cpp
// tests/test_<component>.cpp
#include <iostream>
#include <cassert>
#include "../src/<component>/<header>.h"

int main() {
    // Setup
    auto obj = create_test_object();
    
    // Test
    assert(obj->method() == expected);
    
    // Cleanup
    std::cout << "✓ All tests passed" << std::endl;
    return 0;
}
```

Then update `CMakeLists.txt`:
```cmake
add_executable(test_<component> tests/test_<component>.cpp)
target_link_libraries(test_<component> PRIVATE llm_core)
add_test(NAME <Component> COMMAND test_<component>)
```

### Integration Tests

Test cross-component behavior:
- `MemoryManager` + `ModuleManager` → Load module, check RAM pressure
- `HardwareDetector` + `AdaptiveScheduler` → Detect low battery, check schedule changes
- `HardwareDetector` + `BackendFactory` → Detect platform, verify correct backend chosen

## Performance Considerations

### Memory

- **Weights**: Use mmap when available (especially on mobile)
- **KV cache**: Compress to 4-bit when RAM < 4GB
- **Context length**: Auto-reduce below 512 tokens on low-end devices
- **Module eviction**: Unload modules older than 5 minutes idle

### Latency

- **Prefill phase**: Process prompt in one batch (faster than token-by-token)
- **Decode phase**: Stream single tokens for low latency
- **Batching**: Respect `max_batch_size` from `InferenceSchedule`

### Battery/Thermal

- **Throttle at 80°C** — Reduce batch size by 50%
- **Pause inference** at >85°C or <5% battery
- **Log power usage** → Estimate remaining runtime

## Configuration

Model tiers and module configs live in [config/default_config.json](../config/default_config.json).

**To add a new model variant:**

```json
{
  "tiers": {
    "low_end": {
      "model_size_params": "2B",
      "quantization": "3-bit GGUF",
      "max_context_length": 256,
      "target_devices": ["Raspberry Pi Zero", "Android <2GB"]
    }
  }
}
```

**To adjust thermal thresholds:**

```json
{
  "hardware": {
    "thermal_throttle_threshold_celsius": 75,
    "battery_threshold_for_degradation_percent": 15
  }
}
```

## Debugging Tips

### Hardware Detection Not Detecting GPU?

1. Check platform implementation in `src/hardware/hardware_detector.cpp`
2. Verify `supported_backends` is populated
3. Add logging: `std::cout << "GPU detected: " << spec.has_gpu << std::endl;`
4. Platform-specific: Android → check NNAPI availability via `ANN_LOAD_LIBRARY_FAILED`

### Memory Pressure Callbacks Not Firing?

1. Verify `MemoryManager::register_pressure_callback()` is called
2. Check `calculate_pressure()` threshold logic (50%, 70%, 90%)
3. Ensure allocations are being tracked in `allocations_` vector

### Inference Backend Fails to Initialize?

1. Check model path exists and is readable
2. Verify quantization format matches backend (GGUF for llama.cpp, ONNX for ONNX Runtime)
3. Log backend initialization: `std::cerr << "Backend init failed: " << err << std::endl;`

### Modules Not Loading?

1. Check `ModuleManager::register_module()` called for module
2. Verify available RAM > `module.memory_estimate_bytes`
3. Check module priority — base model (priority 10) loads first

## Frequently Needed Changes

### Change Default Context Length

[config/default_config.json](../config/default_config.json) → `inference.default_context_length`

### Add Thermal Throttling Threshold

[config/default_config.json](../config/default_config.json) → `hardware.thermal_throttle_threshold_celsius`

### Support New Platform

1. Create `platforms/<platform_name>/hardware_<platform>.cpp`
2. Add `#ifdef` case to `CreateNativeHardwareDetector()` in `src/hardware/hardware_detector.cpp`
3. Update `CMakeLists.txt` with platform detection logic
4. Add platform flags to GitHub Actions CI

### Benchmark Inference Speed

Create `tests/benchmark_inference.cpp`:
```cpp
#include <chrono>
auto start = std::chrono::high_resolution_clock::now();
runtime->generate("Prompt", 256);
auto end = std::chrono::high_resolution_clock::now();
auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
std::cout << "Inference: " << ms << " ms" << std::endl;
```

## CI/CD

GitHub Actions (add to `.github/workflows/build.yml`):
- Build on Windows, Linux, macOS
- Run unit tests
- Check code formatting
- Build Android AAR (optional)

## Resources

- **llama.cpp**: https://github.com/ggerganov/llama.cpp
- **ONNX Runtime Mobile**: https://onnxruntime.ai/docs/mobile/
- **ExecuTorch**: https://github.com/pytorch/executorch
- **Android NNAPI**: https://developer.android.com/ndk/guides/neuralnetworks
- **CoreML**: https://developer.apple.com/coreml/
- **Metal**: https://developer.apple.com/metal/

## Questions?

Refer to the main [README.md](../README.md) for architecture overview and API examples.
