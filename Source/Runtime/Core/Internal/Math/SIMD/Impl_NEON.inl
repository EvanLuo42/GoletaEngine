/// @file
/// @brief ARMv8-A NEON SIMD backend. AArch64-only (relies on vaddvq_f32).

namespace goleta::simd
{

GOLETA_FORCEINLINE Vec4f load(const float* P) { return vld1q_f32(P); }
GOLETA_FORCEINLINE Vec4f loadU(const float* P) { return vld1q_f32(P); }
GOLETA_FORCEINLINE void store(float* P, Vec4f V) { vst1q_f32(P, V); }
GOLETA_FORCEINLINE void storeU(float* P, Vec4f V) { vst1q_f32(P, V); }

GOLETA_FORCEINLINE Vec4f set(float X, float Y, float Z, float W)
{
    alignas(16) const float Buf[4] = {X, Y, Z, W};
    return vld1q_f32(Buf);
}
GOLETA_FORCEINLINE Vec4f splat(float V) { return vdupq_n_f32(V); }
GOLETA_FORCEINLINE Vec4f zero() { return vdupq_n_f32(0.0f); }
GOLETA_FORCEINLINE Vec4f one() { return vdupq_n_f32(1.0f); }

GOLETA_FORCEINLINE Vec4f add(Vec4f A, Vec4f B) { return vaddq_f32(A, B); }
GOLETA_FORCEINLINE Vec4f sub(Vec4f A, Vec4f B) { return vsubq_f32(A, B); }
GOLETA_FORCEINLINE Vec4f mul(Vec4f A, Vec4f B) { return vmulq_f32(A, B); }
GOLETA_FORCEINLINE Vec4f div(Vec4f A, Vec4f B) { return vdivq_f32(A, B); }

// vfmaq_f32(ACC, A, B) is ACC + A * B -- note the argument order.
GOLETA_FORCEINLINE Vec4f fma(Vec4f A, Vec4f B, Vec4f C) { return vfmaq_f32(C, A, B); }
GOLETA_FORCEINLINE Vec4f fms(Vec4f A, Vec4f B, Vec4f C) { return vfmsq_f32(C, A, B); }

GOLETA_FORCEINLINE Vec4f neg(Vec4f A) { return vnegq_f32(A); }
GOLETA_FORCEINLINE Vec4f abs(Vec4f A) { return vabsq_f32(A); }
GOLETA_FORCEINLINE Vec4f min(Vec4f A, Vec4f B) { return vminq_f32(A, B); }
GOLETA_FORCEINLINE Vec4f max(Vec4f A, Vec4f B) { return vmaxq_f32(A, B); }

GOLETA_FORCEINLINE Vec4f sqrt(Vec4f A) { return vsqrtq_f32(A); }
GOLETA_FORCEINLINE Vec4f rsqrt(Vec4f A) { return vrsqrteq_f32(A); } // ~12-bit approx
GOLETA_FORCEINLINE Vec4f rcp(Vec4f A) { return vrecpeq_f32(A); }    // ~12-bit approx

GOLETA_FORCEINLINE Vec4f dot3(Vec4f A, Vec4f B)
{
    float32x4_t M = vmulq_f32(A, B);
    M = vsetq_lane_f32(0.0f, M, 3);
    return vdupq_n_f32(vaddvq_f32(M));
}
GOLETA_FORCEINLINE Vec4f dot4(Vec4f A, Vec4f B) { return vdupq_n_f32(vaddvq_f32(vmulq_f32(A, B))); }

template <int X, int Y, int Z, int W>
GOLETA_FORCEINLINE Vec4f shuffle(Vec4f A)
{
    Vec4f R = vdupq_n_f32(0.0f);
    R = vsetq_lane_f32(vgetq_lane_f32(A, X), R, 0);
    R = vsetq_lane_f32(vgetq_lane_f32(A, Y), R, 1);
    R = vsetq_lane_f32(vgetq_lane_f32(A, Z), R, 2);
    R = vsetq_lane_f32(vgetq_lane_f32(A, W), R, 3);
    return R;
}
template <int X, int Y, int Z, int W>
GOLETA_FORCEINLINE Vec4f shuffle2(Vec4f A, Vec4f B)
{
    Vec4f R = vdupq_n_f32(0.0f);
    R = vsetq_lane_f32(vgetq_lane_f32(A, X), R, 0);
    R = vsetq_lane_f32(vgetq_lane_f32(A, Y), R, 1);
    R = vsetq_lane_f32(vgetq_lane_f32(B, Z), R, 2);
    R = vsetq_lane_f32(vgetq_lane_f32(B, W), R, 3);
    return R;
}

GOLETA_FORCEINLINE Vec4f cross3(Vec4f A, Vec4f B)
{
    Vec4f Ayzx = shuffle<1, 2, 0, 3>(A);
    Vec4f Byzx = shuffle<1, 2, 0, 3>(B);
    Vec4f T = vsubq_f32(vmulq_f32(A, Byzx), vmulq_f32(Ayzx, B));
    return shuffle<1, 2, 0, 3>(T);
}

GOLETA_FORCEINLINE Vec4f cmpEq(Vec4f A, Vec4f B) { return vreinterpretq_f32_u32(vceqq_f32(A, B)); }
GOLETA_FORCEINLINE Vec4f cmpNe(Vec4f A, Vec4f B) { return vreinterpretq_f32_u32(vmvnq_u32(vceqq_f32(A, B))); }
GOLETA_FORCEINLINE Vec4f cmpLt(Vec4f A, Vec4f B) { return vreinterpretq_f32_u32(vcltq_f32(A, B)); }
GOLETA_FORCEINLINE Vec4f cmpLe(Vec4f A, Vec4f B) { return vreinterpretq_f32_u32(vcleq_f32(A, B)); }
GOLETA_FORCEINLINE Vec4f cmpGt(Vec4f A, Vec4f B) { return vreinterpretq_f32_u32(vcgtq_f32(A, B)); }
GOLETA_FORCEINLINE Vec4f cmpGe(Vec4f A, Vec4f B) { return vreinterpretq_f32_u32(vcgeq_f32(A, B)); }

GOLETA_FORCEINLINE Vec4f andMask(Vec4f A, Vec4f B)
{
    return vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(A), vreinterpretq_u32_f32(B)));
}
GOLETA_FORCEINLINE Vec4f orMask(Vec4f A, Vec4f B)
{
    return vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(A), vreinterpretq_u32_f32(B)));
}
GOLETA_FORCEINLINE Vec4f xorMask(Vec4f A, Vec4f B)
{
    return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(A), vreinterpretq_u32_f32(B)));
}
// andNot(A, B) = ~A & B. vbicq is "bit-clear": vbicq_u32(B, A) = B & ~A.
GOLETA_FORCEINLINE Vec4f andNot(Vec4f A, Vec4f B)
{
    return vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(B), vreinterpretq_u32_f32(A)));
}
GOLETA_FORCEINLINE Vec4f select(Vec4f Mask, Vec4f A, Vec4f B) { return vbslq_f32(vreinterpretq_u32_f32(Mask), A, B); }

GOLETA_FORCEINLINE float getX(Vec4f A) { return vgetq_lane_f32(A, 0); }
GOLETA_FORCEINLINE float getY(Vec4f A) { return vgetq_lane_f32(A, 1); }
GOLETA_FORCEINLINE float getZ(Vec4f A) { return vgetq_lane_f32(A, 2); }
GOLETA_FORCEINLINE float getW(Vec4f A) { return vgetq_lane_f32(A, 3); }

GOLETA_FORCEINLINE Vec4f splatX(Vec4f A) { return vdupq_laneq_f32(A, 0); }
GOLETA_FORCEINLINE Vec4f splatY(Vec4f A) { return vdupq_laneq_f32(A, 1); }
GOLETA_FORCEINLINE Vec4f splatZ(Vec4f A) { return vdupq_laneq_f32(A, 2); }
GOLETA_FORCEINLINE Vec4f splatW(Vec4f A) { return vdupq_laneq_f32(A, 3); }

} // namespace goleta::simd
