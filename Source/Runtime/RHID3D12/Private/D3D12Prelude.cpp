/// @file
/// @brief Out-of-line helpers referenced from D3D12Prelude.h.

#include "D3D12Prelude.h"

#include <string>

namespace goleta::rhi::d3d12
{

void setD3dObjectName(ID3D12Object* Object, const char* Utf8Name) noexcept
{
    if (!Object || !Utf8Name || !*Utf8Name)
        return;
    const int WideLen = ::MultiByteToWideChar(CP_UTF8, 0, Utf8Name, -1, nullptr, 0);
    if (WideLen <= 0)
        return;
    std::wstring Wide(static_cast<size_t>(WideLen - 1), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, Utf8Name, -1, Wide.data(), WideLen);
    Object->SetName(Wide.c_str());
}

DWORD nanosecondsToWaitMillis(uint64_t Nanos) noexcept
{
    if (Nanos == ~uint64_t{0})
        return INFINITE;
    const uint64_t Millis = (Nanos + 999'999ull) / 1'000'000ull;
    return Millis > 0xFFFFFFFEull ? 0xFFFFFFFEu : static_cast<DWORD>(Millis);
}

} // namespace goleta::rhi::d3d12
