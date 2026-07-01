#include "backend.h"
#include "backends/cpu_backend.h"
#include <algorithm>
#include <iostream>

namespace llm::hal {

std::unique_ptr<InferenceBackend> BackendFactory::create(
    const std::string& backend_name,
    const QuantizationInfo& quant_info) {
    
    std::cout << "Creating backend: " << backend_name 
              << " with quant: " << quant_info.format 
              << " (" << (int)quant_info.bits << "-bit)" << std::endl;
              
    bool use_onnx = (backend_name.find("ONNX") != std::string::npos);
    return std::make_unique<backends::CPUBackend>(use_onnx);
}

std::unique_ptr<InferenceBackend> BackendFactory::create_optimal(
    const std::vector<std::string>& supported_backends) {
    
    // In a full implementation, we would inspect supported_backends 
    // (e.g. CUDA, DirectML) and select the hardware-accelerated one.
    // For now, we fallback to CPU (llama.cpp).
    std::cout << "Selecting optimal backend. Supported options: ";
    for (const auto& b : supported_backends) {
        std::cout << b << " ";
    }
    std::cout << std::endl;
    
    return std::make_unique<backends::CPUBackend>(false);
}

}  // namespace llm::hal
