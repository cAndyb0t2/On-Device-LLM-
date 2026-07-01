# On-Device LLM Runtime

A universal, privacy-first AI assistant that runs fully on-device across Android phones, iPhones, Windows laptops, macOS, Linux, and edge hardware like Raspberry Pi — with adaptive inference scheduling and no mandatory cloud dependency.

## Project Goal

Build an adaptive AI runtime that intelligently scales from constrained edge devices (1B models, <4GB RAM) to high-end workstations (13B+ models, 16GB+ RAM), optimized for inference quality, memory efficiency, latency, and battery life.

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│  Application Layer (Platform UIs)                   │
│  Swift/Kotlin native + cross-platform options       │
└──────────────────┬──────────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────────┐
│  Core LLM Runtime (C++)                             │
├─────────────────────────────────────────────────────┤
│  Hardware Detection   │ Module Manager              │
│  Adaptive Scheduler   │ Privacy Manager             │
│  Memory Manager       │ HAL Backend Selector        │
└──────────────────┬──────────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────────┐
│  Hardware Abstraction Layer (HAL)                   │
├─────────────────────────────────────────────────────┤
│  CPU (llama.cpp)  │ CUDA    │ Metal   │ NNAPI       │
│  DirectML         │ VULKAN  │ CoreML  │ ONNX        │
└─────────────────────────────────────────────────────┘
```

## Key Features

### 1. Three-Tier Hardware Classification

- **Low-end** (1B–3B params): <4GB RAM, Raspberry Pi, older phones
  - Context: 512 tokens
  - Format: 4-bit GGUF
  - Backends: CPU (llama.cpp)

- **Mid-range** (3B–7B params): 4–16GB RAM, standard smartphones/laptops
  - Context: 2048 tokens
  - Format: 4-bit GGUF
  - Backends: CPU, NNAPI (Android), COREML (iOS), Metal (Apple Silicon)

- **High-end** (7B–13B+ params): 16GB+ RAM, gaming laptops, workstations
  - Context: 4096 tokens
  - Format: 8-bit GGUF, FP16
  - Backends: CUDA, Metal, Vulkan, DirectML

### 2. Modular Architecture

Models are not monolithic—compose capabilities at runtime:

```cpp
ModuleType type;
Module* base_model;         // Always loaded (required)
Module* reasoning;          // Optional, ~512 MB
Module* retrieval;          // Optional, ~256 MB
Module* multimodal;         // Optional, ~1 GB
```

**Load/unload decisions** based on:
- Available RAM
- Task requirements
- User preferences

### 3. Memory Management

- **Memory-mapped weight loading** (mmap) reduces peak RAM at startup
- **Layer streaming** for phones: load layers on-demand, not all at once
- **KV cache compression**: quantize to 4-bit or evict oldest tokens
- **Graceful degradation**: reduce context length before OOM

### 4. Adaptive Inference Scheduling

Monitors and responds to:

| Signal | Action |
|--------|--------|
| Thermal throttling (>80°C) | Reduce batch size, increase latency |
| Low battery (<20%) | Reduce context length, unload optional modules |
| Available RAM drops | Compress KV cache, pause heavy ops |
| Backgrounding | Pause inference, save state |

### 5. Privacy-First Architecture

- **All inference local** – no data sent to cloud
- **Encrypted user memory** – SQLite + SQLCipher
- **Offline mode** – block all network calls
- **Default settings** – privacy enforced, not opt-in
- **Honest capability reporting** – timestamp knowledge cutoff, distinguish "unknown" from "requires internet"

### 6. Multi-Platform Support

```
Android:  NNAPI, Vulkan, CPU (llama.cpp)
iOS:      CoreML, Metal, CPU
macOS:    Metal, CPU, CUDA (if NVIDIA)
Windows:  DirectML, CUDA, CPU
Linux:    CUDA, Vulkan, CPU
```

## Project Structure

```
On-Device LLM/
├── src/
│   ├── core/                 # Runtime orchestrator
│   ├── hardware/             # Hardware detection
│   ├── hal/                  # Hardware abstraction layer
│   │   └── backends/         # Backend implementations
│   ├── memory/               # Memory manager
│   ├── modules/              # Module loading system
│   ├── scheduler/            # Adaptive inference scheduler
│   └── privacy/              # Privacy & encryption
├── platforms/
│   ├── android/              # NNAPI, JNI bindings
│   ├── ios/                  # CoreML, Swift interface
│   ├── windows/              # DirectML, Windows APIs
│   └── linux/                # Vulkan, system libraries
├── tools/
│   ├── quantization/         # GGUF, GPTQ, AWQ export
│   └── model_export/         # HuggingFace → ONNX/CoreML
├── config/
│   └── default_config.json   # Model tiers, module configs
├── tests/                    # Unit & integration tests
├── CMakeLists.txt            # Build configuration
└── README.md                 # This file
```

## Building

### Prerequisites

- C++17 compatible compiler
- CMake 3.20+
- llama.cpp (for CPU backend)
- Optional: CUDA toolkit, Metal SDK, Android NDK

### Compile

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
make test
```

### For specific platform:

```bash
# Android
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.cmake ..

# iOS
cmake -GXcode -DCMAKE_SYSTEM_NAME=iOS ..

# Windows with CUDA
cmake -DCMAKE_CUDA_COMPILER=C:/Program\ Files/NVIDIA\ GPU\ Computing\ Toolkit/CUDA/bin/nvcc.exe ..
```

## Usage

### Basic Inference

```cpp
#include "llm/core/runtime.h"

using namespace llm::core;

int main() {
    auto runtime = LLMRuntime::create();
    runtime->initialize("models/base_7b.gguf");
    
    std::string response = runtime->generate(
        "What is the capital of France?",
        256  // max tokens
    );
    
    std::cout << response << std::endl;
    return 0;
}
```

### Streaming Generation

```cpp
runtime->generate_streaming(
    "Explain quantum computing",
    [](const std::string& token) {
        std::cout << token << std::flush;
    }
);
```

### Check Device Capabilities

```cpp
auto spec = runtime->get_device_spec();
std::cout << "Tier: " << static_cast<int>(spec.tier) << std::endl;
std::cout << "Available RAM: " << spec.available_ram_bytes << " bytes" << std::endl;

auto scheduler = runtime->get_scheduler();
auto schedule = scheduler->get_recommended_schedule();
std::cout << "Max context: " << schedule.max_context_length << std::endl;
```

### Load Optional Modules

```cpp
auto modules = runtime->get_module_manager();
modules->load_module(llm::modules::ModuleType::REASONING);
modules->load_module(llm::modules::ModuleType::RETRIEVAL);
```

## Model Quantization Pipeline

### Export from HuggingFace

```bash
python3 tools/model_export/export.py meta-llama/Llama-2-7b \
  --format gguf \
  --output models/
```

### Quantize for Specific Tier

```bash
python3 tools/quantization/quantize.py Llama-2-7b \
  --tier mid_range \
  --format gguf \
  --bits 4 \
  --prune 0.2 \
  --output quantized_models/
```

This produces:
- `llama-2-7b-mid.gguf` (~4 GB, 4-bit quantized)
- `llama-2-7b-mid.onnx` (cross-platform fallback)
- Metadata with tier config, context length, memory estimates

## Configuration

See [config/default_config.json](config/default_config.json) for:
- Model variants per tier
- Module memory estimates
- Thermal/battery thresholds
- Privacy policies

Example: Change knowledge cutoff (for honest capability reporting)

```json
{
  "model_metadata": {
    "knowledge_cutoff": "2024-06"
  }
}
```

## Testing

```bash
# Hardware detection
./test_hardware_detection

# Memory management
./test_memory_manager

# All tests
make test
```

## Privacy & Security

### Offline-Only Mode

```cpp
auto privacy = runtime->get_privacy_manager();
llm::privacy::PrivacyPolicy policy;
policy.disable_network_access = true;
policy.allow_cloud_sync = false;
privacy->set_policy(policy);
```

### User Memory

```cpp
auto memory_store = privacy->get_memory_store();
memory_store->initialize("my-password");
memory_store->store_message("conv_123", "user", "What is 2+2?");
memory_store->store_message("conv_123", "assistant", "The answer is 4.");
```

### Audit Log

```cpp
auto stats = privacy->get_audit_log();
for (const auto& entry : stats) {
    std::cout << entry.timestamp << " | " << entry.operation 
              << " | " << (entry.allowed ? "ALLOWED" : "BLOCKED") << std::endl;
}
```

## Roadmap

### Phase 1 (MVP)
- [x] Hardware detection across platforms
- [x] HAL abstraction layer
- [x] Memory management with KV cache
- [x] Module loading skeleton
- [ ] CPU backend (llama.cpp integration)
- [ ] Android NNAPI support
- [ ] iOS CoreML support

### Phase 2
- [ ] Adaptive thermal/battery scheduling
- [ ] CUDA backend
- [ ] Quantization pipeline (GGUF, GPTQ)
- [ ] Privacy: encrypted user memory store
- [ ] Model streaming for low-RAM devices

### Phase 3
- [ ] Retrieval-augmented generation (RAG)
- [ ] On-device fine-tuning (LoRA)
- [ ] Federated learning support
- [ ] Speech I/O (Whisper, TTS)
- [ ] Vision capabilities (multimodal)

## Contributing

Please follow these guidelines:
1. Code in C++17, Python 3.10+
2. Add tests for new components
3. Document public APIs with Doxygen comments
4. Platform-specific code in `platforms/` directory

## License

[Specify your license — MIT, Apache 2.0, etc.]

## References

- [llama.cpp](https://github.com/ggerganov/llama.cpp) – CPU inference
- [ONNX Runtime Mobile](https://onnxruntime.ai/docs/mobile/)
- [ExecuTorch](https://github.com/pytorch/executorch) – PyTorch mobile
- [Android NNAPI](https://developer.android.com/ndk/guides/neuralnetworks)
- [CoreML](https://developer.apple.com/coreml/)
- [Vulkan](https://www.khronos.org/vulkan/)

## Contact & Support

For questions or issues, open a GitHub issue or discussion.
