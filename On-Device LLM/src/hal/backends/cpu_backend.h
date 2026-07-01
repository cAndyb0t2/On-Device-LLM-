#pragma once

#include "../backend.h"
#include <memory>

namespace llm::hal::backends {

/// CPU-based inference using llama.cpp or ONNX Runtime
class CPUBackend : public InferenceBackend {
public:
    explicit CPUBackend(bool use_onnx = false);
    ~CPUBackend() override;
    
    bool initialize(const std::string& model_path, uint32_t max_context_length) override;
    bool forward(const std::vector<uint32_t>& tokens, Tensor* output_logits) override;
    bool forward_with_cache(const std::vector<uint32_t>& new_tokens,
                           InferenceContext* ctx, Tensor* output_logits) override;
    bool prefill(const std::vector<uint32_t>& prompt, InferenceContext* ctx) override;
    bool decode(uint32_t token, InferenceContext* ctx, Tensor* output_logits) override;
    
    std::string get_name() const override { return use_onnx_ ? "ONNXRuntime-CPU" : "llama.cpp"; }
    uint64_t get_memory_usage() const override;
    
private:
    bool use_onnx_;
    // Actual implementation pointers omitted for brevity
};

}  // namespace llm::hal::backends
