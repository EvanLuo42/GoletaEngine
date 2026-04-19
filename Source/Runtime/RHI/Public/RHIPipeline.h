#pragma once

/// @file
/// @brief Graphics and compute pipeline-state objects. Immutable once created.

#include <cstdint>

#include "RHIEnums.h"
#include "RHIExport.h"
#include "RHIFormat.h"
#include "RHIResource.h"
#include "RHIStructChain.h"

namespace goleta::rhi
{

class IRhiShaderModule;
class IRhiDescriptorSetLayout;

enum class RhiFillMode : uint8_t
{
    Solid = 0,
    Wireframe
};
enum class RhiCullMode : uint8_t
{
    None = 0,
    Front,
    Back
};
enum class RhiFrontFace : uint8_t
{
    CounterClockwise = 0,
    Clockwise
};

struct RhiRasterizerState
{
    RhiFillMode  FillMode            = RhiFillMode::Solid;
    RhiCullMode  CullMode            = RhiCullMode::Back;
    RhiFrontFace FrontFace           = RhiFrontFace::CounterClockwise;
    float        DepthBias           = 0.0f;
    float        DepthBiasClamp      = 0.0f;
    float        DepthBiasSlopeScale = 0.0f;
    bool         DepthClipEnable     = true;
    bool         ConservativeRaster  = false;
};

enum class RhiStencilOp : uint8_t
{
    Keep = 0,
    Zero,
    Replace,
    IncrementClamp,
    DecrementClamp,
    Invert,
    IncrementWrap,
    DecrementWrap,
};

struct RhiStencilOpState
{
    RhiStencilOp FailOp      = RhiStencilOp::Keep;
    RhiStencilOp PassOp      = RhiStencilOp::Keep;
    RhiStencilOp DepthFailOp = RhiStencilOp::Keep;
    RhiCompareOp CompareOp   = RhiCompareOp::Always;
};

struct RhiDepthStencilState
{
    bool              DepthTestEnable  = false;
    bool              DepthWriteEnable = false;
    RhiCompareOp      DepthCompareOp   = RhiCompareOp::Less;
    bool              StencilEnable    = false;
    uint8_t           StencilReadMask  = 0xFF;
    uint8_t           StencilWriteMask = 0xFF;
    RhiStencilOpState FrontFace{};
    RhiStencilOpState BackFace{};
};

enum class RhiBlendFactor : uint8_t
{
    Zero = 0,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SrcAlphaSaturated,
};

enum class RhiBlendOp : uint8_t
{
    Add = 0,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
};

enum class RhiColorWriteMask : uint8_t
{
    None = 0,
    R    = 1u << 0,
    G    = 1u << 1,
    B    = 1u << 2,
    A    = 1u << 3,
    All  = R | G | B | A,
};

struct RhiBlendAttachmentState
{
    bool              BlendEnable = false;
    RhiBlendFactor    SrcColor    = RhiBlendFactor::One;
    RhiBlendFactor    DstColor    = RhiBlendFactor::Zero;
    RhiBlendOp        ColorOp     = RhiBlendOp::Add;
    RhiBlendFactor    SrcAlpha    = RhiBlendFactor::One;
    RhiBlendFactor    DstAlpha    = RhiBlendFactor::Zero;
    RhiBlendOp        AlphaOp     = RhiBlendOp::Add;
    RhiColorWriteMask WriteMask   = RhiColorWriteMask::All;
};

struct RhiBlendState
{
    bool                    IndependentBlend = false;
    RhiBlendAttachmentState Attachments[8]{};
    float                   BlendConstants[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

enum class RhiVertexInputRate : uint8_t
{
    Vertex = 0,
    Instance
};

struct RhiVertexAttribute
{
    uint32_t  Location    = 0;
    uint32_t  Binding     = 0;
    RhiFormat Format      = RhiFormat::Rgba32Float;
    uint32_t  OffsetBytes = 0;
};

struct RhiVertexBinding
{
    uint32_t           Binding     = 0;
    uint32_t           StrideBytes = 0;
    RhiVertexInputRate InputRate   = RhiVertexInputRate::Vertex;
};

struct RhiVertexInputState
{
    const RhiVertexBinding*   Bindings       = nullptr;
    uint32_t                  BindingCount   = 0;
    const RhiVertexAttribute* Attributes     = nullptr;
    uint32_t                  AttributeCount = 0;
};

/// @brief Root-signature-style layout shared by all pipeline kinds.
/// @note  The renderer is expected to use push constants ("root constants") for indices into
///        the global bindless heap. Descriptor-set layouts are optional for traditional binding.
struct RhiBindingLayout
{
    uint32_t           PushConstantBytes      = 128; // 32 DWORDS.
    RhiShaderStageMask PushConstantVisibility = RhiShaderStageMask::All;

    IRhiDescriptorSetLayout* const* Sets     = nullptr;
    uint32_t                        SetCount = 0;

    /// @brief Whether the layout opts into the device's global bindless heap. Default true.
    bool UseBindlessHeap = true;
};

struct RhiGraphicsPipelineDesc
{
    static constexpr RhiStructType kStructType = RhiStructType::GraphicsPipelineDesc;
    RhiStructHeader                Header{kStructType, nullptr};

    IRhiShaderModule* VertexShader   = nullptr;
    IRhiShaderModule* PixelShader    = nullptr;
    IRhiShaderModule* GeometryShader = nullptr;
    IRhiShaderModule* HullShader     = nullptr;
    IRhiShaderModule* DomainShader   = nullptr;

    // Mesh-shading pipelines set these two and leave vertex/geom/hull/domain null.
    IRhiShaderModule* MeshShader          = nullptr;
    IRhiShaderModule* AmplificationShader = nullptr;

    RhiBindingLayout     Bindings{};
    RhiVertexInputState  VertexInput{};
    RhiPrimitiveTopology Topology = RhiPrimitiveTopology::TriangleList;
    RhiRasterizerState   Rasterizer{};
    RhiDepthStencilState DepthStencil{};
    RhiBlendState        Blend{};
    RhiSampleCount       SampleCount = RhiSampleCount::X1;
    uint32_t             SampleMask  = 0xFFFFFFFF;

    RhiFormat ColorAttachmentFormats[8] = {};
    uint32_t  ColorAttachmentCount      = 0;
    RhiFormat DepthStencilFormat        = RhiFormat::Unknown;

    const char* DebugName = nullptr;
};

struct RhiComputePipelineDesc
{
    static constexpr RhiStructType kStructType = RhiStructType::ComputePipelineDesc;
    RhiStructHeader                Header{kStructType, nullptr};

    IRhiShaderModule* ComputeShader = nullptr;
    RhiBindingLayout  Bindings{};
    const char*       DebugName = nullptr;
};

class RHI_API IRhiGraphicsPipeline : public IRhiResource
{
public:
    virtual const RhiGraphicsPipelineDesc& desc() const noexcept = 0;
};

class RHI_API IRhiComputePipeline : public IRhiResource
{
public:
    virtual const RhiComputePipelineDesc& desc() const noexcept = 0;
};

} // namespace goleta::rhi
