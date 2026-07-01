#include "cpu_backend.h"
#include <iostream>

namespace llm::hal::backends {

CPUBackend::CPUBackend(bool use_onnx)
    : use_onnx_(use_onnx) {}

CPUBackend::~CPUBackend() {}

bool CPUBackend::initialize(const std::string& model_path, uint32_t max_context_length) {
    std::cout << "Initializing CPU Backend (" << get_name() << ") with model: " 
              << model_path << " and context limit: " << max_context_length << std::endl;
    return true;
}

bool CPUBackend::forward(const std::vector<uint32_t>& tokens, Tensor* output_logits) {
    // Stub forward implementation
    return true;
}

bool CPUBackend::forward_with_cache(
    const std::vector<uint32_t>& new_tokens,
    InferenceContext* ctx,
    Tensor* output_logits) {
    return true;
}

bool CPUBackend::prefill(
    const std::vector<uint32_t>& prompt,
    InferenceContext* ctx) {
    return true;
}

bool CPUBackend::decode(
    uint32_t token,
    InferenceContext* ctx,
    Tensor* output_logits) {
    return true;
}

uint64_t CPUBackend::get_memory_usage() const {
    // Return typical memory usage depending on the backend name
    return use_onnx_ ? 256 * 1024 * 1024ULL : 512 * 1024 * 1024ULL;
}

}  // namespace llm::hal::backends
