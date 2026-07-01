#include <iostream>
#include <thread>
#include <chrono>
#include "runtime.h"

int main(int argc, char* argv[]) {
    // Expected usage: runtime_service.exe <model_path>
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <model_path>" << std::endl;
        return 1;
    }
    std::string model_path = argv[1];

    // Create runtime instance
    auto runtime = llm::core::LLMRuntime::create();
    if (!runtime) {
        std::cerr << "Failed to create LLM runtime" << std::endl;
        return 1;
    }
    if (!runtime->initialize(model_path)) {
        std::cerr << "Runtime initialization failed" << std::endl;
        return 1;
    }

    // Perform a single dummy generation to verify everything works
    std::string response = runtime->generate("Hello, world!", 128);
    std::cout << "Service generation result: " << response << std::endl;

    // Keep the service alive until stopped. In a real service, you'd implement
    // a proper command loop or IPC mechanism. Here we just sleep.
    std::cout << "Service is now running (press Ctrl+C to exit)" << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
    return 0;
}
