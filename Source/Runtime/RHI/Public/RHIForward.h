#pragma once

/// @file
/// @brief Forward declarations for every RHI interface. Include when only pointers are needed.

#include <cstdint>

namespace goleta::rhi
{

class IRhiResource;
class IRhiBuffer;
class IRhiTexture;
class IRhiTextureView;
class IRhiSampler;
class IRhiShaderModule;
class IRhiGraphicsPipeline;
class IRhiComputePipeline;
class IRhiRayTracingPipeline;
class IRhiAccelStructure;
class IRhiMemoryHeap;
class IRhiDescriptorHeap;
class IRhiDescriptorSet;
class IRhiDescriptorSetLayout;
class IRhiQueryHeap;

class IRhiInstance;
class IRhiDevice;
class IRhiQueue;
class IRhiFence;
class IRhiSwapChain;
class IRhiCommandList;
class IRhiCommandPool;
class IRhiDebug;

struct RhiStructHeader;
struct RhiAdapterInfo;
struct RhiDeviceFeatures;

enum class RhiFormat : uint32_t;
enum class RhiQueueKind : uint8_t;
enum class RhiResourceKind : uint8_t;
enum class RhiDebugLevel : uint8_t;
enum class RhiStructType : uint32_t;
enum class RhiError : uint32_t;
enum class RhiWaitStatus : uint8_t;
enum class RhiNativeWindowKind : uint8_t;
enum class BackendKind : uint8_t;

struct RhiNativeWindow;

} // namespace goleta::rhi
