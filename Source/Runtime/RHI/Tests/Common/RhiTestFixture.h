#pragma once

/// @file
/// @brief Cross-backend GoogleTest fixture + GPU_TEST macro. One test body, one declaration,
///        instantiated per backend. Each test carries a BackendMask declaring which backends
///        it actually runs on; unavailable or masked-out backends produce GTEST_SKIP.

#include <gtest/gtest.h>

#include <string>

#include "Memory/Rc.h"
#include "RHIBuffer.h"
#include "RHICommandList.h"
#include "RHIDebug.h"
#include "RHIDescriptor.h"
#include "RHIDevice.h"
#include "RHIEnums.h"
#include "RHIInstance.h"
#include "RHIMemory.h"
#include "RHIPipeline.h"
#include "RHIQuery.h"
#include "RHIQueue.h"
#include "RHISampler.h"
#include "RHIShader.h"
#include "RHISwapChain.h"
#include "RHISync.h"
#include "RHITexture.h"
#include "RhiBackendMask.h"

// This header is test-only. Pull RHI names into scope so GPU_TEST bodies (which live at
// global scope) can reference RhiBufferDesc, InvalidBindlessIndex, etc. without qualification.
using namespace goleta;        // NOLINT(google-build-using-namespace)
using namespace goleta::rhi;   // NOLINT(google-build-using-namespace)

namespace goleta::rhi::tests
{

/// @brief Lazy, per-process backend state. Device created on first access; nullptr when the
///        backend is not registered, or device creation fails.
struct BackendState
{
    Rc<IRhiInstance> Instance;
    Rc<IRhiDevice>   Device;
    Rc<IRhiQueue>    Gfx;
    bool             Probed = false;
    bool             Available = false;
};

BackendState& backendState(BackendKind Kind) noexcept;

/// @brief Cross-backend test fixture. Parameterized by BackendKind. Access the per-backend
///        device through Device / Gfx (nullptr when SKIPped).
class RhiTest : public ::testing::TestWithParam<BackendKind>
{
public:
    // Public so the free-function test body emitted by GPU_TEST can reach them via `F.`.
    IRhiDevice* Device = nullptr;
    IRhiQueue*  Gfx    = nullptr;

protected:
    void SetUp() override;
};

/// @brief Emit a single parameterized test that runs on the intersection of (registered
///        backends) × (RunMask bits). The body runs with `this->Device` and `this->Gfx`
///        pointing at the current backend's device/queue.
#define GPU_TEST(SuiteName, TestName, RunMask)                                                       \
    static void gpuTestBody_##SuiteName##_##TestName(::goleta::rhi::tests::RhiTest& F);              \
    TEST_P(RhiTest, SuiteName##_##TestName)                                                          \
    {                                                                                                \
        if (!::goleta::rhi::tests::contains((RunMask), GetParam()))                                  \
        {                                                                                            \
            GTEST_SKIP() << "test opted out of backend "                                             \
                         << ::goleta::rhi::tests::backendName(GetParam());                           \
        }                                                                                            \
        gpuTestBody_##SuiteName##_##TestName(*this);                                                 \
    }                                                                                                \
    static void gpuTestBody_##SuiteName##_##TestName(::goleta::rhi::tests::RhiTest& F)

} // namespace goleta::rhi::tests
