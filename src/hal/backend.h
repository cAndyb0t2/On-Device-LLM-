#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace llm::hal {

/// Forward declarations
class Tensor {};
class InferenceContext {};

/// Inference backend abstraction
class InferenceBackend {
public:
    virtual ~InferenceBackend() = default;
    
    /// Initialize backend with model weights and config
    virtual bool initialize(const std::string& model_path, uint32_t max_context_length) = 0;
    
    /// Run forward pass on input tokens
    /// Returns logits of shape [batch_size, vocab_size]
    virtual bool forward(const std::vector<uint32_t>& tokens, Tensor* output_logits) = 0;
    
    /// Run with KV cache awareness (for streaming)
    virtual bool forward_with_cache(
        const std::vector<uint32_t>& new_tokens,
        InferenceContext* ctx,
        Tensor* output_logits) = 0;
    
    /// Get backend name
    virtual std::string get_name() const = 0;
    
    /// Get estimated memory usage in bytes
    virtual uint64_t get_memory_usage() const = 0;
    
    /// Prefill phase (for prompt processing)
    virtual bool prefill(
        const std::vector<uint32_t>& prompt,
        InferenceContext* ctx) = 0;
    
    /// Decode phase (single token generation)
    virtual bool decode(
        uint32_t token,
        InferenceContext* ctx,
        Tensor* output_logits) = 0;
};

/// Quantization format info
struct QuantizationInfo {
    std::string format;  // "GGUF", "GPTQ", "AWQ", "INT8", etc.
    uint8_t bits;        // 3, 4, 8
    std::string method;  // "static", "dynamic"
};

/// Backend factory and selector
class BackendFactory {
public:
    /// Create backend for the specified type
    static std::unique_ptr<InferenceBackend> create(
        const std::string& backend_name,
        const QuantizationInfo& quant_info);
    
    /// Auto-select best backend based on hardware
    static std::unique_ptr<InferenceBackend> create_optimal(
        const std::vector<std::string>& supported_backends);
};

}  // namespace llm::hal
