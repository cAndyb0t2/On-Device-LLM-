cmake_minimum_required(VERSION 3.20)
project(OnDeviceLLM CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Detect platform
if(WIN32)
    set(PLATFORM "windows")
    add_compile_definitions(PLATFORM_WINDOWS)
elseif(APPLE)
    set(PLATFORM "macos")
    add_compile_definitions(PLATFORM_MACOS)
elseif(ANDROID)
    set(PLATFORM "android")
    add_compile_definitions(PLATFORM_ANDROID)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM "linux")
    add_compile_definitions(PLATFORM_LINUX)
endif()

message(STATUS "Building for platform: ${PLATFORM}")

# Core library
add_library(llm_core STATIC
    src/hardware/hardware_detector.cpp
    src/memory/memory_manager.cpp
    src/core/runtime.cpp
    src/hal/backend.cpp
    src/hal/backends/cpu_backend.cpp
)

target_include_directories(llm_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Platform-specific sources
if(PLATFORM STREQUAL "windows")
    target_sources(llm_core PRIVATE
        platforms/windows/hardware_win.cpp
    )
elseif(PLATFORM STREQUAL "macos")
    target_sources(llm_core PRIVATE
        platforms/ios/hardware_apple.cpp
    )
    target_link_libraries(llm_core PRIVATE
        "-framework Foundation"
    )
elseif(PLATFORM STREQUAL "android")
    target_sources(llm_core PRIVATE
        platforms/android/hardware_android.cpp
    )
elseif(PLATFORM STREQUAL "linux")
    target_sources(llm_core PRIVATE
        platforms/linux/hardware_linux.cpp
    )
endif()

# Optional: Link against llama.cpp
# find_package(llama REQUIRED)
# target_link_libraries(llm_core PUBLIC llama)

# Tests
enable_testing()

add_executable(test_hardware tests/test_hardware_detection.cpp)
target_link_libraries(test_hardware PRIVATE llm_core)
add_test(NAME HardwareDetection COMMAND test_hardware)

add_executable(test_memory tests/test_memory_manager.cpp)
target_link_libraries(test_memory PRIVATE llm_core)
add_test(NAME MemoryManager COMMAND test_memory)

# Service launcher executable
add_executable(on_device_llm_http_service src/launcher/http_service.cpp)
target_link_libraries(on_device_llm_http_service PRIVATE llm_core ws2_32)

add_executable(on_device_llm_service src/launcher/runtime_service.cpp)
target_link_libraries(on_device_llm_service PRIVATE llm_core)

# Installation
install(TARGETS llm_core)
install(DIRECTORY src/ DESTINATION include/llm)
install(DIRECTORY config/ DESTINATION etc/llm)
