/// @file
/// @brief Descriptor heap implementation. Free-list over an ID3D12DescriptorHeap.

#include "D3D12DescriptorHeap.h"

namespace goleta::rhi::d3d12
{
namespace
{
inline constexpr uint32_t kCpuHeapInvalid = 0xFFFFFFFFu;
}

bool ShaderVisibleHeap::initialize(ID3D12Device* Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t Capacity,
                                   const char* DebugName) noexcept
{
    if (!Device || Capacity == 0)
        return false;

    D3D12_DESCRIPTOR_HEAP_DESC Desc{};
    Desc.Type           = Type;
    Desc.NumDescriptors = Capacity;
    Desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    const HRESULT Hr    = Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&Heap));
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "CreateDescriptorHeap(shader-visible, type={}): HRESULT 0x{:08x}",
                         static_cast<int>(Type), static_cast<unsigned>(Hr));
        return false;
    }
    if (DebugName)
        setD3dObjectName(Heap.Get(), DebugName);
    CpuStart  = Heap->GetCPUDescriptorHandleForHeapStart();
    GpuStart  = Heap->GetGPUDescriptorHandleForHeapStart();
    Increment = Device->GetDescriptorHandleIncrementSize(Type);
    Capacity_ = Capacity;
    NextFresh = 1u; // Reserve slot 0 so InvalidBindlessIndex (0) remains an unambiguous sentinel.
    FreeList.clear();
    return true;
}

uint32_t ShaderVisibleHeap::allocate() noexcept
{
    std::scoped_lock Lock(Mutex);
    if (!FreeList.empty())
    {
        const uint32_t Idx = FreeList.back();
        FreeList.pop_back();
        return Idx;
    }
    if (NextFresh >= Capacity_)
        return InvalidBindlessIndex;
    return NextFresh++;
}

uint32_t ShaderVisibleHeap::allocateRangeFresh(uint32_t Count) noexcept
{
    if (Count == 0)
        return InvalidBindlessIndex;
    std::scoped_lock Lock(Mutex);
    if (NextFresh + Count > Capacity_)
        return InvalidBindlessIndex;
    const uint32_t Base = NextFresh;
    NextFresh += Count;
    return Base;
}

void ShaderVisibleHeap::free(uint32_t Index) noexcept
{
    if (Index == InvalidBindlessIndex || Index >= Capacity_)
        return;
    std::scoped_lock Lock(Mutex);
    FreeList.push_back(Index);
}

D3D12_CPU_DESCRIPTOR_HANDLE ShaderVisibleHeap::cpuHandle(uint32_t Index) const noexcept
{
    D3D12_CPU_DESCRIPTOR_HANDLE H = CpuStart;
    H.ptr += static_cast<SIZE_T>(Index) * Increment;
    return H;
}

D3D12_GPU_DESCRIPTOR_HANDLE ShaderVisibleHeap::gpuHandle(uint32_t Index) const noexcept
{
    D3D12_GPU_DESCRIPTOR_HANDLE H = GpuStart;
    H.ptr += static_cast<UINT64>(Index) * Increment;
    return H;
}

bool D3D12DescriptorHeap::initialize(ID3D12Device* Device, uint32_t ResourceCapacity, uint32_t SamplerCapacity) noexcept
{
    if (!Resource.initialize(Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, ResourceCapacity, "Goleta CBV/SRV/UAV Heap"))
        return false;
    if (!Sampler.initialize(Device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, SamplerCapacity, "Goleta Sampler Heap"))
        return false;
    return true;
}

bool CpuDescriptorHeap::initialize(ID3D12Device* Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t Capacity) noexcept
{
    if (!Device || Capacity == 0)
        return false;
    D3D12_DESCRIPTOR_HEAP_DESC Desc{};
    Desc.Type           = Type;
    Desc.NumDescriptors = Capacity;
    Desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    const HRESULT Hr    = Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&Heap));
    if (FAILED(Hr))
    {
        GOLETA_LOG_ERROR(D3D12, "CreateDescriptorHeap(cpu, type={}): HRESULT 0x{:08x}",
                         static_cast<int>(Type), static_cast<unsigned>(Hr));
        return false;
    }
    CpuStart  = Heap->GetCPUDescriptorHandleForHeapStart();
    Increment = Device->GetDescriptorHandleIncrementSize(Type);
    Capacity_ = Capacity;
    NextFresh = 0;
    FreeList.clear();
    return true;
}

uint32_t CpuDescriptorHeap::allocate() noexcept
{
    std::scoped_lock Lock(Mutex);
    if (!FreeList.empty())
    {
        const uint32_t Idx = FreeList.back();
        FreeList.pop_back();
        return Idx;
    }
    if (NextFresh >= Capacity_)
        return kCpuHeapInvalid;
    return NextFresh++;
}

void CpuDescriptorHeap::free(uint32_t Index) noexcept
{
    if (Index >= Capacity_)
        return;
    std::scoped_lock Lock(Mutex);
    FreeList.push_back(Index);
}

D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHeap::cpuHandle(uint32_t Index) const noexcept
{
    D3D12_CPU_DESCRIPTOR_HANDLE H = CpuStart;
    H.ptr += static_cast<SIZE_T>(Index) * Increment;
    return H;
}

} // namespace goleta::rhi::d3d12
