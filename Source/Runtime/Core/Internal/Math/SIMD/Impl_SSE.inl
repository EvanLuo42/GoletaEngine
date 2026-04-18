/// @file
/// @brief SSE4.1 / AVX2 SIMD backend. SSE4.1 is the baseline; GOLETA_SIMD_AVX2
///        upgrades fma/fms to hardware VFMADD/VFMSUB.

#pragma once

namespace goleta::simd
{

GOLETA_FORCEINLINE Vec4f load(const float* P) { return _mm_load_ps(P); }
GOLETA_FORCEINLINE Vec4f loadU(const float* P) { return _mm_loadu_ps(P); }
GOLETA_FORCEINLINE void store(float* P, const Vec4f V) { _mm_store_ps(P, V); }
GOLETA_FORCEINLINE void storeU(float* P, const Vec4f V) { _mm_storeu_ps(P, V); }

GOLETA_FORCEINLINE Vec4f set(const float X, const float Y, const float Z, const float W)
{
    return _mm_setr_ps(X, Y, Z, W);
}
GOLETA_FORCEINLINE Vec4f splat(const float V) { return _mm_set1_ps(V); }
GOLETA_FORCEINLINE Vec4f zero() { return _mm_setzero_ps(); }
GOLETA_FORCEINLINE Vec4f one() { return _mm_set1_ps(1.0f); }

GOLETA_FORCEINLINE Vec4f add(const Vec4f A, const Vec4f B) { return _mm_add_ps(A, B); }
GOLETA_FORCEINLINE Vec4f sub(const Vec4f A, const Vec4f B) { return _mm_sub_ps(A, B); }
GOLETA_FORCEINLINE Vec4f mul(const Vec4f A, const Vec4f B) { return _mm_mul_ps(A, B); }
GOLETA_FORCEINLINE Vec4f div(const Vec4f A, const Vec4f B) { return _mm_div_ps(A, B); }

GOLETA_FORCEINLINE Vec4f fma(const Vec4f A, const Vec4f B, const Vec4f C)
{
#if GOLETA_SIMD_AVX2
    return _mm_fmadd_ps(A, B, C);
#else
    return _mm_add_ps(_mm_mul_ps(A, B), C);
#endif
}
GOLETA_FORCEINLINE Vec4f fms(const Vec4f A, const Vec4f B, const Vec4f C)
{
#if GOLETA_SIMD_AVX2
    return _mm_fmsub_ps(A, B, C);
#else
    return _mm_sub_ps(_mm_mul_ps(A, B), C);
#endif
}

GOLETA_FORCEINLINE Vec4f neg(const Vec4f A) { return _mm_sub_ps(_mm_setzero_ps(), A); }
GOLETA_FORCEINLINE Vec4f abs(const Vec4f A) { return _mm_and_ps(A, _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF))); }
GOLETA_FORCEINLINE Vec4f min(const Vec4f A, const Vec4f B) { return _mm_min_ps(A, B); }
GOLETA_FORCEINLINE Vec4f max(const Vec4f A, const Vec4f B) { return _mm_max_ps(A, B); }

GOLETA_FORCEINLINE Vec4f sqrt(const Vec4f A) { return _mm_sqrt_ps(A); }
GOLETA_FORCEINLINE Vec4f rsqrt(const Vec4f A) { return _mm_rsqrt_ps(A); } // ~12-bit approx
GOLETA_FORCEINLINE Vec4f rcp(const Vec4f A) { return _mm_rcp_ps(A); }     // ~12-bit approx

GOLETA_FORCEINLINE Vec4f dot3(const Vec4f A, const Vec4f B) { return _mm_dp_ps(A, B, 0x7F); }
GOLETA_FORCEINLINE Vec4f dot4(const Vec4f A, const Vec4f B) { return _mm_dp_ps(A, B, 0xFF); }
GOLETA_FORCEINLINE Vec4f cross3(const Vec4f A, const Vec4f B)
{
    const __m128 Ayzx = _mm_shuffle_ps(A, A, _MM_SHUFFLE(3, 0, 2, 1));
    const __m128 Byzx = _mm_shuffle_ps(B, B, _MM_SHUFFLE(3, 0, 2, 1));
    const __m128 T = _mm_sub_ps(_mm_mul_ps(A, Byzx), _mm_mul_ps(Ayzx, B));
    return _mm_shuffle_ps(T, T, _MM_SHUFFLE(3, 0, 2, 1));
}

GOLETA_FORCEINLINE Vec4f cmpEq(const Vec4f A, const Vec4f B) { return _mm_cmpeq_ps(A, B); }
GOLETA_FORCEINLINE Vec4f cmpNe(const Vec4f A, const Vec4f B) { return _mm_cmpneq_ps(A, B); }
GOLETA_FORCEINLINE Vec4f cmpLt(const Vec4f A, const Vec4f B) { return _mm_cmplt_ps(A, B); }
GOLETA_FORCEINLINE Vec4f cmpLe(const Vec4f A, const Vec4f B) { return _mm_cmple_ps(A, B); }
GOLETA_FORCEINLINE Vec4f cmpGt(const Vec4f A, const Vec4f B) { return _mm_cmpgt_ps(A, B); }
GOLETA_FORCEINLINE Vec4f cmpGe(const Vec4f A, const Vec4f B) { return _mm_cmpge_ps(A, B); }

GOLETA_FORCEINLINE Vec4f andMask(const Vec4f A, const Vec4f B) { return _mm_and_ps(A, B); }
GOLETA_FORCEINLINE Vec4f orMask(const Vec4f A, const Vec4f B) { return _mm_or_ps(A, B); }
GOLETA_FORCEINLINE Vec4f xorMask(const Vec4f A, const Vec4f B) { return _mm_xor_ps(A, B); }
GOLETA_FORCEINLINE Vec4f andNot(const Vec4f A, const Vec4f B) { return _mm_andnot_ps(A, B); }

// blendv picks the second operand when the mask lane's MSB is set, so the
// argument order is (B, A, Mask) -- not the (A, B, Mask) you'd guess.
GOLETA_FORCEINLINE Vec4f select(const Vec4f Mask, const Vec4f A, const Vec4f B) { return _mm_blendv_ps(B, A, Mask); }

template <int X, int Y, int Z, int W>
GOLETA_FORCEINLINE Vec4f shuffle(const Vec4f A)
{
    return _mm_shuffle_ps(A, A, _MM_SHUFFLE(W, Z, Y, X));
}
template <int X, int Y, int Z, int W>
GOLETA_FORCEINLINE Vec4f shuffle2(const Vec4f A, const Vec4f B)
{
    return _mm_shuffle_ps(A, B, _MM_SHUFFLE(W, Z, Y, X));
}

GOLETA_FORCEINLINE float getX(const Vec4f A) { return _mm_cvtss_f32(A); }
GOLETA_FORCEINLINE float getY(const Vec4f A) { return _mm_cvtss_f32(_mm_shuffle_ps(A, A, _MM_SHUFFLE(1, 1, 1, 1))); }
GOLETA_FORCEINLINE float getZ(const Vec4f A) { return _mm_cvtss_f32(_mm_shuffle_ps(A, A, _MM_SHUFFLE(2, 2, 2, 2))); }
GOLETA_FORCEINLINE float getW(const Vec4f A) { return _mm_cvtss_f32(_mm_shuffle_ps(A, A, _MM_SHUFFLE(3, 3, 3, 3))); }

GOLETA_FORCEINLINE Vec4f splatX(const Vec4f A) { return _mm_shuffle_ps(A, A, _MM_SHUFFLE(0, 0, 0, 0)); }
GOLETA_FORCEINLINE Vec4f splatY(const Vec4f A) { return _mm_shuffle_ps(A, A, _MM_SHUFFLE(1, 1, 1, 1)); }
GOLETA_FORCEINLINE Vec4f splatZ(const Vec4f A) { return _mm_shuffle_ps(A, A, _MM_SHUFFLE(2, 2, 2, 2)); }
GOLETA_FORCEINLINE Vec4f splatW(const Vec4f A) { return _mm_shuffle_ps(A, A, _MM_SHUFFLE(3, 3, 3, 3)); }

} // namespace goleta::simd
