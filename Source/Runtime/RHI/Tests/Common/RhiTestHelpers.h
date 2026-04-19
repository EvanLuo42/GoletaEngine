#pragma once

/// @file
/// @brief Backend-agnostic GPU→CPU readback + HRESULT-style helpers for cross-backend tests.

#include <cstring>
#include <span>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "RHIBarrier.h"
#include "RHIBuffer.h"
#include "RHICommandList.h"
#include "RHIDevice.h"
#include "RHIQueue.h"
#include "RHISync.h"
#include "RHITexture.h"

namespace goleta::rhi::tests
{

#define REQUIRE_RHI_OK(ResultExpr)                                                                                    \
    do {                                                                                                              \
        auto&& _r = (ResultExpr);                                                                                     \
        ASSERT_TRUE(_r.isOk()) << #ResultExpr " returned Err";                                                        \
    } while (0)

/// @brief Submit a single command list and block until complete.
void submitAndWait(IRhiDevice* Device, IRhiQueue* Queue, IRhiCommandList* CmdList);

/// @brief Copy `ElemCount` elements of type `T` out of `Src` at offset `OffsetBytes` into a
///        host-visible readback buffer, then map+memcpy. One-shot helper; allocates a new
///        readback buffer + command list each call.
template <class T>
std::vector<T> readBuffer(IRhiDevice* Device, IRhiQueue* Queue, IRhiBuffer* Src, uint64_t OffsetBytes,
                          uint64_t ElemCount)
{
    std::vector<T> Out(ElemCount);
    const uint64_t Bytes = ElemCount * sizeof(T);

    RhiBufferDesc ReadDesc{};
    ReadDesc.SizeBytes = Bytes;
    ReadDesc.Usage     = RhiBufferUsage::CopyDest;
    ReadDesc.Location  = RhiMemoryLocation::Readback;
    ReadDesc.DebugName = "TestReadback";
    auto Read = Device->createBuffer(ReadDesc);
    if (!Read) return Out;

    auto Pool = Device->createCommandPool(RhiQueueKind::Graphics);
    auto Cl   = Pool->allocate();
    Cl->begin();
    Cl->copyBuffer(Read.get(), 0, Src, OffsetBytes, Bytes);
    Cl->end();
    submitAndWait(Device, Queue, Cl.get());

    void* P = Read->map(0, Bytes);
    if (P)
    {
        std::memcpy(Out.data(), P, static_cast<size_t>(Bytes));
        Read->unmap();
    }
    return Out;
}

template <class T>
void checkBufferEquals(IRhiDevice* Device, IRhiQueue* Queue, IRhiBuffer* Src, std::span<const T> Expected,
                       uint64_t OffsetBytes = 0)
{
    const auto Actual = readBuffer<T>(Device, Queue, Src, OffsetBytes, Expected.size());
    ASSERT_EQ(Actual.size(), Expected.size());
    for (size_t I = 0; I < Expected.size(); ++I)
    {
        SCOPED_TRACE("element " + std::to_string(I));
        EXPECT_EQ(Actual[I], Expected[I]);
    }
}

} // namespace goleta::rhi::tests
