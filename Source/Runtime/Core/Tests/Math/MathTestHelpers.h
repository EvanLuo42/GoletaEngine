#pragma once

/// @file
/// @brief Shared helpers for math tests: float/Vec/Mat near-equality matchers.

#include "Math/Math.h"

#include <cmath>
#include <gtest/gtest.h>

namespace goleta::math::testing
{

constexpr float kEpsilon = 1e-5f;

inline ::testing::AssertionResult nearFloat(float A, float B, float Eps = kEpsilon)
{
    const float Diff = std::fabs(A - B);
    if (Diff <= Eps) return ::testing::AssertionSuccess();
    return ::testing::AssertionFailure() << "|" << A << " - " << B << "| = " << Diff << " > " << Eps;
}

inline ::testing::AssertionResult nearVec3(Vec3 A, Vec3 B, float Eps = kEpsilon)
{
    if (nearFloat(A.x(), B.x(), Eps) && nearFloat(A.y(), B.y(), Eps) && nearFloat(A.z(), B.z(), Eps))
        return ::testing::AssertionSuccess();
    return ::testing::AssertionFailure() << "(" << A.x() << "," << A.y() << "," << A.z() << ") vs ("
                                         << B.x() << "," << B.y() << "," << B.z() << ")";
}

inline ::testing::AssertionResult nearVec4(Vec4 A, Vec4 B, float Eps = kEpsilon)
{
    if (nearFloat(A.x(), B.x(), Eps) && nearFloat(A.y(), B.y(), Eps) &&
        nearFloat(A.z(), B.z(), Eps) && nearFloat(A.w(), B.w(), Eps))
        return ::testing::AssertionSuccess();
    return ::testing::AssertionFailure() << "(" << A.x() << "," << A.y() << "," << A.z() << "," << A.w()
                                         << ") vs (" << B.x() << "," << B.y() << "," << B.z() << "," << B.w() << ")";
}

inline ::testing::AssertionResult nearMat4(const Mat4& A, const Mat4& B, float Eps = kEpsilon)
{
    for (int I = 0; I < 4; ++I)
    {
        if (!nearVec4(A.row(I), B.row(I), Eps))
            return ::testing::AssertionFailure() << "row " << I << " differs";
    }
    return ::testing::AssertionSuccess();
}

} // namespace goleta::math::testing
