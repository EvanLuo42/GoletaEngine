/// @file
/// @brief ID3D12QueryHeap wrapper (timestamp only in MVP).

#include "D3D12QueryHeap.h"

#include "D3D12Device.h"

namespace goleta::rhi::d3d12
{

Rc<D3D12QueryHeap> D3D12QueryHeap::create(D3D12Device* Device, const RhiQueryHeapDesc& Desc) noexcept
{
    if (!Device || Desc.Count == 0)
        return {};

    D3D12_QUERY_HEAP_DESC D{};
    D.Count = Desc.Count;
    switch (Desc.Kind)
    {
    case RhiQueryKind::Timestamp:          D.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP; break;
    case RhiQueryKind::Occlusion:          D.Type = D3D12_QUERY_HEAP_TYPE_OCCLUSION; break;
    case RhiQueryKind::PipelineStatistics: D.Type = D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS; break;
    }

    auto Self = Rc<D3D12QueryHeap>(new D3D12QueryHeap{});
    const HRESULT Hr = Device->raw()->CreateQueryHeap(&D, IID_PPV_ARGS(&Self->Heap));
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "CreateQueryHeap: HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
        return {};
    }
    Self->Desc_ = Desc;
    if (Desc.DebugName)
    {
        Self->Name = Desc.DebugName;
        setD3dObjectName(Self->Heap.Get(), Desc.DebugName);
    }
    return Self;
}

void D3D12QueryHeap::setDebugName(const char* NewName)
{
    Name = NewName ? NewName : "";
    if (Heap)
        setD3dObjectName(Heap.Get(), Name.c_str());
}

D3D12_QUERY_TYPE D3D12QueryHeap::d3dType() const noexcept
{
    switch (Desc_.Kind)
    {
    case RhiQueryKind::Timestamp:          return D3D12_QUERY_TYPE_TIMESTAMP;
    case RhiQueryKind::Occlusion:          return D3D12_QUERY_TYPE_OCCLUSION;
    case RhiQueryKind::PipelineStatistics: return D3D12_QUERY_TYPE_PIPELINE_STATISTICS;
    }
    return D3D12_QUERY_TYPE_TIMESTAMP;
}

} // namespace goleta::rhi::d3d12
