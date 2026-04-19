/// @file
/// @brief PSO + root-signature construction from RHI pipeline descriptors.

#include "D3D12Pipeline.h"

#include "D3D12Device.h"
#include "D3D12FormatTable.h"
#include "D3D12ShaderModule.h"

namespace goleta::rhi::d3d12
{
namespace
{

D3D12_FILL_MODE toFill(RhiFillMode F) noexcept
{
    return F == RhiFillMode::Wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
}

D3D12_CULL_MODE toCull(RhiCullMode C) noexcept
{
    switch (C)
    {
    case RhiCullMode::None:  return D3D12_CULL_MODE_NONE;
    case RhiCullMode::Front: return D3D12_CULL_MODE_FRONT;
    case RhiCullMode::Back:  return D3D12_CULL_MODE_BACK;
    }
    return D3D12_CULL_MODE_BACK;
}

D3D12_COMPARISON_FUNC toCmp(RhiCompareOp C) noexcept
{
    switch (C)
    {
    case RhiCompareOp::Never:        return D3D12_COMPARISON_FUNC_NEVER;
    case RhiCompareOp::Less:         return D3D12_COMPARISON_FUNC_LESS;
    case RhiCompareOp::Equal:        return D3D12_COMPARISON_FUNC_EQUAL;
    case RhiCompareOp::LessEqual:    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case RhiCompareOp::Greater:      return D3D12_COMPARISON_FUNC_GREATER;
    case RhiCompareOp::NotEqual:     return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case RhiCompareOp::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case RhiCompareOp::Always:       return D3D12_COMPARISON_FUNC_ALWAYS;
    }
    return D3D12_COMPARISON_FUNC_NEVER;
}

D3D12_BLEND toBlend(RhiBlendFactor F) noexcept
{
    switch (F)
    {
    case RhiBlendFactor::Zero:                  return D3D12_BLEND_ZERO;
    case RhiBlendFactor::One:                   return D3D12_BLEND_ONE;
    case RhiBlendFactor::SrcColor:              return D3D12_BLEND_SRC_COLOR;
    case RhiBlendFactor::OneMinusSrcColor:      return D3D12_BLEND_INV_SRC_COLOR;
    case RhiBlendFactor::DstColor:              return D3D12_BLEND_DEST_COLOR;
    case RhiBlendFactor::OneMinusDstColor:      return D3D12_BLEND_INV_DEST_COLOR;
    case RhiBlendFactor::SrcAlpha:              return D3D12_BLEND_SRC_ALPHA;
    case RhiBlendFactor::OneMinusSrcAlpha:      return D3D12_BLEND_INV_SRC_ALPHA;
    case RhiBlendFactor::DstAlpha:              return D3D12_BLEND_DEST_ALPHA;
    case RhiBlendFactor::OneMinusDstAlpha:      return D3D12_BLEND_INV_DEST_ALPHA;
    case RhiBlendFactor::ConstantColor:         return D3D12_BLEND_BLEND_FACTOR;
    case RhiBlendFactor::OneMinusConstantColor: return D3D12_BLEND_INV_BLEND_FACTOR;
    case RhiBlendFactor::ConstantAlpha:         return D3D12_BLEND_BLEND_FACTOR;
    case RhiBlendFactor::OneMinusConstantAlpha: return D3D12_BLEND_INV_BLEND_FACTOR;
    case RhiBlendFactor::SrcAlphaSaturated:     return D3D12_BLEND_SRC_ALPHA_SAT;
    }
    return D3D12_BLEND_ONE;
}

D3D12_BLEND_OP toBlendOp(RhiBlendOp O) noexcept
{
    switch (O)
    {
    case RhiBlendOp::Add:             return D3D12_BLEND_OP_ADD;
    case RhiBlendOp::Subtract:        return D3D12_BLEND_OP_SUBTRACT;
    case RhiBlendOp::ReverseSubtract: return D3D12_BLEND_OP_REV_SUBTRACT;
    case RhiBlendOp::Min:             return D3D12_BLEND_OP_MIN;
    case RhiBlendOp::Max:             return D3D12_BLEND_OP_MAX;
    }
    return D3D12_BLEND_OP_ADD;
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE toTopoType(RhiPrimitiveTopology T) noexcept
{
    switch (T)
    {
    case RhiPrimitiveTopology::PointList:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    case RhiPrimitiveTopology::LineList:
    case RhiPrimitiveTopology::LineStrip:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case RhiPrimitiveTopology::TriangleList:
    case RhiPrimitiveTopology::TriangleStrip: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case RhiPrimitiveTopology::PatchList:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    }
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

D3D12_PRIMITIVE_TOPOLOGY toTopo(RhiPrimitiveTopology T) noexcept
{
    switch (T)
    {
    case RhiPrimitiveTopology::PointList:     return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case RhiPrimitiveTopology::LineList:      return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case RhiPrimitiveTopology::LineStrip:     return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case RhiPrimitiveTopology::TriangleList:  return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case RhiPrimitiveTopology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case RhiPrimitiveTopology::PatchList:     return D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
    }
    return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

} // namespace

ComPtr<ID3D12RootSignature> buildRootSignature(ID3D12Device* Device, const RhiBindingLayout& Layout) noexcept
{
    // Root constants (one 32-bit-values block) + optional legacy descriptor-table sets. Bindless
    // SM 6.6 heap indexing comes for free via CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED flag.
    D3D12_ROOT_PARAMETER1 Params[8]{};
    uint32_t              NumParams = 0;

    if (Layout.PushConstantBytes > 0)
    {
        auto& P = Params[NumParams++];
        P.ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        P.ShaderVisibility         = D3D12_SHADER_VISIBILITY_ALL;
        P.Constants.ShaderRegister = 0;
        P.Constants.RegisterSpace  = 0;
        P.Constants.Num32BitValues = (Layout.PushConstantBytes + 3) / 4;
    }

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC VDesc{};
    VDesc.Version                    = D3D_ROOT_SIGNATURE_VERSION_1_1;
    VDesc.Desc_1_1.NumParameters     = NumParams;
    VDesc.Desc_1_1.pParameters       = Params;
    VDesc.Desc_1_1.NumStaticSamplers = 0;
    VDesc.Desc_1_1.pStaticSamplers   = nullptr;
    VDesc.Desc_1_1.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    if (Layout.UseBindlessHeap)
        VDesc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
                              | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

    ComPtr<ID3DBlob> Sig;
    ComPtr<ID3DBlob> Err;
    HRESULT Hr = D3D12SerializeVersionedRootSignature(&VDesc, &Sig, &Err);
    if (FAILED(Hr))
    {
        if (Err)
            GOLETA_LOG_ERROR(D3D12, "Root signature serialization: {}",
                             static_cast<const char*>(Err->GetBufferPointer()));
        return {};
    }
    ComPtr<ID3D12RootSignature> Out;
    Hr = Device->CreateRootSignature(0, Sig->GetBufferPointer(), Sig->GetBufferSize(), IID_PPV_ARGS(&Out));
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "CreateRootSignature: HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
        return {};
    }
    return Out;
}

// ---------- D3D12GraphicsPipeline ----------

Rc<D3D12GraphicsPipeline> D3D12GraphicsPipeline::create(D3D12Device* Device, const RhiGraphicsPipelineDesc& Desc) noexcept
{
    if (!Device)
        return {};
    auto Self = Rc<D3D12GraphicsPipeline>(new D3D12GraphicsPipeline{});
    Self->Desc_ = Desc;
    Self->RootSig = buildRootSignature(Device->raw(), Desc.Bindings);
    if (!Self->RootSig)
        return {};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC D{};
    D.pRootSignature = Self->RootSig.Get();
    auto pickBc = [](IRhiShaderModule* M) -> D3D12_SHADER_BYTECODE {
        return M ? d3d12Cast<D3D12ShaderModule>(M)->bytecode() : D3D12_SHADER_BYTECODE{};
    };
    D.VS = pickBc(Desc.VertexShader);
    D.PS = pickBc(Desc.PixelShader);
    D.GS = pickBc(Desc.GeometryShader);
    D.HS = pickBc(Desc.HullShader);
    D.DS = pickBc(Desc.DomainShader);

    D.BlendState.AlphaToCoverageEnable  = FALSE;
    D.BlendState.IndependentBlendEnable = Desc.Blend.IndependentBlend ? TRUE : FALSE;
    for (int I = 0; I < 8; ++I)
    {
        const auto& A = Desc.Blend.Attachments[I];
        auto&       T = D.BlendState.RenderTarget[I];
        T.BlendEnable           = A.BlendEnable ? TRUE : FALSE;
        T.LogicOpEnable         = FALSE;
        T.SrcBlend              = toBlend(A.SrcColor);
        T.DestBlend             = toBlend(A.DstColor);
        T.BlendOp               = toBlendOp(A.ColorOp);
        T.SrcBlendAlpha         = toBlend(A.SrcAlpha);
        T.DestBlendAlpha        = toBlend(A.DstAlpha);
        T.BlendOpAlpha          = toBlendOp(A.AlphaOp);
        T.RenderTargetWriteMask = static_cast<UINT8>(A.WriteMask);
    }

    D.SampleMask = Desc.SampleMask;

    D.RasterizerState.FillMode              = toFill(Desc.Rasterizer.FillMode);
    D.RasterizerState.CullMode              = toCull(Desc.Rasterizer.CullMode);
    D.RasterizerState.FrontCounterClockwise = Desc.Rasterizer.FrontFace == RhiFrontFace::CounterClockwise;
    D.RasterizerState.DepthBias             = static_cast<INT>(Desc.Rasterizer.DepthBias);
    D.RasterizerState.DepthBiasClamp        = Desc.Rasterizer.DepthBiasClamp;
    D.RasterizerState.SlopeScaledDepthBias  = Desc.Rasterizer.DepthBiasSlopeScale;
    D.RasterizerState.DepthClipEnable       = Desc.Rasterizer.DepthClipEnable;
    D.RasterizerState.ConservativeRaster    = Desc.Rasterizer.ConservativeRaster
                                                  ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON
                                                  : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D.DepthStencilState.DepthEnable    = Desc.DepthStencil.DepthTestEnable ? TRUE : FALSE;
    D.DepthStencilState.DepthWriteMask = Desc.DepthStencil.DepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL
                                                                            : D3D12_DEPTH_WRITE_MASK_ZERO;
    D.DepthStencilState.DepthFunc      = toCmp(Desc.DepthStencil.DepthCompareOp);
    D.DepthStencilState.StencilEnable  = Desc.DepthStencil.StencilEnable ? TRUE : FALSE;

    // Minimal vertex-input: we only fill the element array if any bindings present.
    std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;
    if (Desc.VertexInput.AttributeCount > 0 && Desc.VertexInput.Attributes)
    {
        InputElements.resize(Desc.VertexInput.AttributeCount);
        for (uint32_t I = 0; I < Desc.VertexInput.AttributeCount; ++I)
        {
            const auto& A = Desc.VertexInput.Attributes[I];
            auto&       E = InputElements[I];
            E.SemanticName         = "ATTR";
            E.SemanticIndex        = A.Location;
            E.Format               = toDxgi(A.Format);
            E.InputSlot            = A.Binding;
            E.AlignedByteOffset    = A.OffsetBytes;
            E.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            E.InstanceDataStepRate = 0;
        }
        D.InputLayout.pInputElementDescs = InputElements.data();
        D.InputLayout.NumElements        = static_cast<UINT>(InputElements.size());
    }

    D.PrimitiveTopologyType = toTopoType(Desc.Topology);
    D.NumRenderTargets      = Desc.ColorAttachmentCount;
    for (uint32_t I = 0; I < Desc.ColorAttachmentCount && I < 8; ++I)
        D.RTVFormats[I] = toDxgi(Desc.ColorAttachmentFormats[I]);
    D.DSVFormat         = toDxgiDsv(Desc.DepthStencilFormat);
    D.SampleDesc.Count  = static_cast<UINT>(Desc.SampleCount);
    D.SampleDesc.Quality = 0;
    D.NodeMask          = 0;

    const HRESULT Hr = Device->raw()->CreateGraphicsPipelineState(&D, IID_PPV_ARGS(&Self->State));
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "CreateGraphicsPipelineState: HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
        return {};
    }
    Self->Topology = toTopo(Desc.Topology);
    if (Desc.DebugName)
    {
        Self->Name = Desc.DebugName;
        setD3dObjectName(Self->State.Get(), Desc.DebugName);
    }
    return Self;
}

void D3D12GraphicsPipeline::setDebugName(const char* NewName)
{
    Name = NewName ? NewName : "";
    if (State)
        setD3dObjectName(State.Get(), Name.c_str());
}

// ---------- D3D12ComputePipeline ----------

Rc<D3D12ComputePipeline> D3D12ComputePipeline::create(D3D12Device* Device, const RhiComputePipelineDesc& Desc) noexcept
{
    if (!Device || !Desc.ComputeShader)
        return {};
    auto Self = Rc<D3D12ComputePipeline>(new D3D12ComputePipeline{});
    Self->Desc_   = Desc;
    Self->RootSig = buildRootSignature(Device->raw(), Desc.Bindings);
    if (!Self->RootSig)
        return {};
    D3D12_COMPUTE_PIPELINE_STATE_DESC D{};
    D.pRootSignature = Self->RootSig.Get();
    D.CS             = d3d12Cast<D3D12ShaderModule>(Desc.ComputeShader)->bytecode();
    const HRESULT Hr = Device->raw()->CreateComputePipelineState(&D, IID_PPV_ARGS(&Self->State));
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "CreateComputePipelineState: HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
        return {};
    }
    if (Desc.DebugName)
    {
        Self->Name = Desc.DebugName;
        setD3dObjectName(Self->State.Get(), Desc.DebugName);
    }
    return Self;
}

void D3D12ComputePipeline::setDebugName(const char* NewName)
{
    Name = NewName ? NewName : "";
    if (State)
        setD3dObjectName(State.Get(), Name.c_str());
}

} // namespace goleta::rhi::d3d12
