/// @file
/// @brief ID3D12Resource buffers. D3D12MA path when available, CreateCommittedResource fallback.

#include "D3D12Buffer.h"

#include "D3D12Device.h"

namespace goleta::rhi::d3d12
{
namespace
{

D3D12_RESOURCE_FLAGS toResourceFlags(RhiBufferUsage Usage) noexcept
{
    D3D12_RESOURCE_FLAGS F = D3D12_RESOURCE_FLAG_NONE;
    if (anyOf(Usage, RhiBufferUsage::StorageBuffer))
        F |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    return F;
}

D3D12_HEAP_TYPE toHeapType(RhiMemoryLocation L) noexcept
{
    switch (L)
    {
    case RhiMemoryLocation::DeviceLocal: return D3D12_HEAP_TYPE_DEFAULT;
    case RhiMemoryLocation::Upload:      return D3D12_HEAP_TYPE_UPLOAD;
    case RhiMemoryLocation::Readback:    return D3D12_HEAP_TYPE_READBACK;
    }
    return D3D12_HEAP_TYPE_DEFAULT;
}

D3D12_RESOURCE_DESC makeBufferDesc(const RhiBufferDesc& Desc) noexcept
{
    D3D12_RESOURCE_DESC D{};
    D.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    D.Alignment        = 0;
    D.Width            = Desc.SizeBytes;
    D.Height           = 1;
    D.DepthOrArraySize = 1;
    D.MipLevels        = 1;
    D.Format           = DXGI_FORMAT_UNKNOWN;
    D.SampleDesc.Count = 1;
    D.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    D.Flags            = toResourceFlags(Desc.Usage);
    return D;
}

} // namespace

Rc<D3D12Buffer> D3D12Buffer::create(D3D12Device* Device, const RhiBufferDesc& Desc) noexcept
{
    if (!Device || Desc.SizeBytes == 0)
        return {};

    auto Self = Rc<D3D12Buffer>(new D3D12Buffer{});
    Self->OwnerDevice = Device;
    Self->Desc_       = Desc;

    const D3D12_RESOURCE_DESC  D   = makeBufferDesc(Desc);
    const D3D12_BARRIER_LAYOUT InitialLayout = D3D12_BARRIER_LAYOUT_UNDEFINED; // Unused for buffers.
    (void)InitialLayout;
    const D3D12_RESOURCE_STATES InitialStates =
        Desc.Location == RhiMemoryLocation::Upload   ? D3D12_RESOURCE_STATE_GENERIC_READ
      : Desc.Location == RhiMemoryLocation::Readback ? D3D12_RESOURCE_STATE_COPY_DEST
                                                     : D3D12_RESOURCE_STATE_COMMON;

#if defined(GOLETA_RHID3D12_HAS_D3D12MA) && GOLETA_RHID3D12_HAS_D3D12MA
    if (auto* Alloc = Device->allocator())
    {
        D3D12MA::ALLOCATION_DESC AllocDesc{};
        AllocDesc.HeapType = toHeapType(Desc.Location);

        D3D12MA::Allocation* RawAlloc = nullptr;
        ID3D12Resource*      RawRes   = nullptr;
        const HRESULT        Hr       = Alloc->CreateResource(&AllocDesc, &D, InitialStates, nullptr,
                                                              &RawAlloc, IID_PPV_ARGS(&RawRes));
        if (FAILED(Hr))
        {
            GOLETA_LOG_ERROR(D3D12, "D3D12MA::CreateResource(buffer): HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
            return {};
        }
        Self->Allocation.Attach(RawAlloc);
        Self->Resource.Attach(RawRes);
    }
    else
#endif
    {
        D3D12_HEAP_PROPERTIES Props{};
        Props.Type = toHeapType(Desc.Location);
        const HRESULT Hr = Device->raw()->CreateCommittedResource(&Props, D3D12_HEAP_FLAG_NONE, &D, InitialStates,
                                                                   nullptr, IID_PPV_ARGS(&Self->Resource));
        if (FAILED(Hr))
        {
            GOLETA_LOG_ERROR(D3D12, "CreateCommittedResource(buffer): HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
            return {};
        }
    }

    if (Desc.DebugName)
    {
        Self->Name = Desc.DebugName;
        setD3dObjectName(Self->Resource.Get(), Desc.DebugName);
    }
    Self->allocateBindlessViews();
    return Self;
}

Rc<D3D12Buffer> D3D12Buffer::createPlaced(D3D12Device* Device, ID3D12Heap* Heap, uint64_t Offset,
                                          const RhiBufferDesc& Desc) noexcept
{
    if (!Device || !Heap || Desc.SizeBytes == 0)
        return {};
    auto Self = Rc<D3D12Buffer>(new D3D12Buffer{});
    Self->OwnerDevice = Device;
    Self->Desc_       = Desc;

    const D3D12_RESOURCE_DESC D = makeBufferDesc(Desc);
    const D3D12_RESOURCE_STATES InitialStates =
        Desc.Location == RhiMemoryLocation::Upload   ? D3D12_RESOURCE_STATE_GENERIC_READ
      : Desc.Location == RhiMemoryLocation::Readback ? D3D12_RESOURCE_STATE_COPY_DEST
                                                     : D3D12_RESOURCE_STATE_COMMON;

    const HRESULT Hr = Device->raw()->CreatePlacedResource(Heap, Offset, &D, InitialStates, nullptr,
                                                            IID_PPV_ARGS(&Self->Resource));
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "CreatePlacedResource(buffer): HRESULT 0x{:08x}", static_cast<unsigned>(Hr));
        return {};
    }
    if (Desc.DebugName)
    {
        Self->Name = Desc.DebugName;
        setD3dObjectName(Self->Resource.Get(), Desc.DebugName);
    }
    Self->allocateBindlessViews();
    return Self;
}

Rc<D3D12Buffer> D3D12Buffer::wrapRaw(D3D12Device* Device, ComPtr<ID3D12Resource> Resource,
                                     const RhiBufferDesc& Desc) noexcept
{
    auto Self = Rc<D3D12Buffer>(new D3D12Buffer{});
    Self->OwnerDevice = Device;
    Self->Resource    = std::move(Resource);
    Self->Desc_       = Desc;
    Self->allocateBindlessViews();
    return Self;
}

D3D12Buffer::~D3D12Buffer()
{
    if (MappedPtr && Resource)
    {
        Resource->Unmap(0, nullptr);
        MappedPtr = nullptr;
    }
    if (OwnerDevice)
    {
        if (SrvIndex != InvalidBindlessIndex)
            OwnerDevice->bindless().resourceHeap().free(SrvIndex);
        if (UavIndex != InvalidBindlessIndex)
            OwnerDevice->bindless().resourceHeap().free(UavIndex);
    }
}

void D3D12Buffer::setDebugName(const char* NewName)
{
    Name = NewName ? NewName : "";
    if (Resource)
        setD3dObjectName(Resource.Get(), Name.c_str());
}

void* D3D12Buffer::map(uint64_t Offset, uint64_t /*Size*/)
{
    if (!Resource || Desc_.Location == RhiMemoryLocation::DeviceLocal)
        return nullptr;
    if (MappedPtr)
        return static_cast<std::byte*>(MappedPtr) + Offset;
    void* Ptr = nullptr;
    const D3D12_RANGE Range{0, 0}; // No CPU read range hint; covers Upload path.
    const HRESULT Hr = Resource->Map(0, Desc_.Location == RhiMemoryLocation::Readback ? nullptr : &Range, &Ptr);
    if (FAILED(Hr))
        return nullptr;
    MappedPtr = Ptr;
    return static_cast<std::byte*>(Ptr) + Offset;
}

void D3D12Buffer::unmap()
{
    if (!Resource || !MappedPtr)
        return;
    Resource->Unmap(0, nullptr);
    MappedPtr = nullptr;
}

void D3D12Buffer::allocateBindlessViews() noexcept
{
    if (!OwnerDevice || !Resource)
        return;
    auto* Dev = OwnerDevice->raw();
    auto& Heap = OwnerDevice->bindless().resourceHeap();

    const bool IsStructured = Desc_.StructureStride > 0;
    const bool IsRaw        = !IsStructured && anyOf(Desc_.Usage, RhiBufferUsage::StorageBuffer);

    // Only storage / structured buffers receive a bindless SRV. Constant buffers are viewed as
    // CBVs through IRhiDescriptorSet::writeConstantBuffer(); creating an untyped buffer SRV for
    // them trips the debug layer with UNKNOWN format and removes the device.
    if (anyOf(Desc_.Usage, RhiBufferUsage::StorageBuffer) || IsStructured)
    {
        const uint32_t Idx = Heap.allocate();
        if (Idx != InvalidBindlessIndex)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC V{};
            V.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            V.ViewDimension           = D3D12_SRV_DIMENSION_BUFFER;
            V.Format                  = IsRaw ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN;
            if (IsStructured)
            {
                V.Buffer.NumElements         = static_cast<UINT>(Desc_.SizeBytes / Desc_.StructureStride);
                V.Buffer.StructureByteStride = Desc_.StructureStride;
            }
            else
            {
                V.Buffer.NumElements         = static_cast<UINT>(Desc_.SizeBytes / 4);
                V.Buffer.Flags               = IsRaw ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;
            }
            Dev->CreateShaderResourceView(Resource.Get(), &V, Heap.cpuHandle(Idx));
            SrvIndex = Idx;
        }
    }

    if (anyOf(Desc_.Usage, RhiBufferUsage::StorageBuffer))
    {
        const uint32_t Idx = Heap.allocate();
        if (Idx != InvalidBindlessIndex)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC V{};
            V.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            V.Format        = IsRaw ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN;
            if (IsStructured)
            {
                V.Buffer.NumElements         = static_cast<UINT>(Desc_.SizeBytes / Desc_.StructureStride);
                V.Buffer.StructureByteStride = Desc_.StructureStride;
            }
            else
            {
                V.Buffer.NumElements = static_cast<UINT>(Desc_.SizeBytes / 4);
                V.Buffer.Flags       = IsRaw ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;
            }
            Dev->CreateUnorderedAccessView(Resource.Get(), nullptr, &V, Heap.cpuHandle(Idx));
            UavIndex = Idx;
        }
    }
}

} // namespace goleta::rhi::d3d12
