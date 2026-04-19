/// @file
/// @brief Instance-level cross-backend tests. Not parameterized — runs once per registered
///        backend via the enumerate-and-assert pattern.

#include <gtest/gtest.h>

#include "Common/RhiBackendMask.h"
#include "Common/RhiTestFixture.h"
#include "RHIBackend.h"

using namespace goleta::rhi::tests;

namespace
{
void exerciseInstance(BackendKind Kind)
{
    if (findRhiBackend(Kind) == nullptr)
        GTEST_SKIP() << "backend " << backendName(Kind) << " not registered";
    RhiInstanceCreateInfo II{};
    II.Backend = Kind;
    auto Instance = createInstance(II);
    ASSERT_TRUE(Instance);
    EXPECT_EQ(Instance->backend(), Kind);

    const auto Adapters = Instance->enumerateAdapters();
    if (Adapters.empty())
        GTEST_SKIP() << backendName(Kind) << " enumeration returned empty";
    for (const auto& A : Adapters)
    {
        EXPECT_STRNE(A.Name, "");
        EXPECT_EQ(A.Backend, Kind);
    }
}
} // namespace

TEST(Instance, NullAdapterNamedAndEnumerable)   { exerciseInstance(BackendKind::Null); }
TEST(Instance, D3D12AdapterNamedAndEnumerable)  { exerciseInstance(BackendKind::D3D12); }
// TODO(rhi): add one line per new backend as they land (Vulkan, etc.).
