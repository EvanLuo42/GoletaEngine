/// @file
/// @brief ID3D12Heap wrapper for placed-resource creation.

#include "D3D12MemoryHeap.h"

#include "D3D12Device.h"

namespace goleta::rhi::d3d12
{

Rc<D3D12MemoryHeap> D3D12MemoryHeap::create(D3D12Device* Device, const RhiHeapDesc& Desc) noexcept
{
    if (!Device || Desc.SizeBytes == 0)
        return {};

    D3D12_HEAP_DESC D{};
    D.SizeInBytes = Desc.SizeBytes;
    D.Alignment   = Desc.Alignment == 0 ? D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT : Desc.Alignment;
    switch (Desc.Location)
    {
    case RhiMemoryLocation::DeviceLocal: D.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;  break;
    case RhiMemoryLocation::Upload:      D.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;   break;
    case RhiMemoryLocation::Readback:    D.Properties.Type = D3D12_HEAP_TYPE_READBACK; break;
    }
    D.Flags = Desc.BufferOnly ? D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS : D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

    auto Self = Rc<D3D12MemoryHeap>(new D3D12MemoryHeap{});
    const HRESULT Hr = Device->raw()->CreateHeap(&D, IID_PPV_ARGS(&Self->Heap));
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "CreateHeap: HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
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

void D3D12MemoryHeap::setDebugName(const char* NewName)
{
    Name = NewName ? NewName : "";
    if (Heap)
        setD3dObjectName(Heap.Get(), Name.c_str());
}

} // namespace goleta::rhi::d3d12
