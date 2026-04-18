/// @file
/// @brief Inline definitions for Mat3 / Mat4; included from Matrix.h. Heavy
///        routines live in Private/Math/Matrix.cpp.

#include <cmath>

namespace goleta::math
{

GOLETA_FORCEINLINE Mat4::Mat4(Vec4 R0, Vec4 R1, Vec4 R2, Vec4 R3)
{
    Rows[0] = R0.Data;
    Rows[1] = R1.Data;
    Rows[2] = R2.Data;
    Rows[3] = R3.Data;
}

GOLETA_FORCEINLINE Mat4 Mat4::identity() { return Mat4(Vec4::unitX(), Vec4::unitY(), Vec4::unitZ(), Vec4::unitW()); }
GOLETA_FORCEINLINE Mat4 Mat4::zero()
{
    Mat4 R;
    R.Rows[0] = R.Rows[1] = R.Rows[2] = R.Rows[3] = simd::zero();
    return R;
}

GOLETA_FORCEINLINE Mat4 Mat4::translation(Vec3 T)
{
    return Mat4(Vec4(1.0f, 0.0f, 0.0f, 0.0f), Vec4(0.0f, 1.0f, 0.0f, 0.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f),
                Vec4(T.x(), T.y(), T.z(), 1.0f));
}
GOLETA_FORCEINLINE Mat4 Mat4::scale(Vec3 S)
{
    return Mat4(Vec4(S.x(), 0.0f, 0.0f, 0.0f), Vec4(0.0f, S.y(), 0.0f, 0.0f), Vec4(0.0f, 0.0f, S.z(), 0.0f),
                Vec4(0.0f, 0.0f, 0.0f, 1.0f));
}
GOLETA_FORCEINLINE Mat4 Mat4::scale(float S) { return Mat4::scale(Vec3(S, S, S)); }

GOLETA_FORCEINLINE Mat4 Mat4::rotationX(float Radians)
{
    float C = std::cos(Radians), S = std::sin(Radians);
    return Mat4(Vec4(1.0f, 0.0f, 0.0f, 0.0f), Vec4(0.0f, C, S, 0.0f), Vec4(0.0f, -S, C, 0.0f),
                Vec4(0.0f, 0.0f, 0.0f, 1.0f));
}
GOLETA_FORCEINLINE Mat4 Mat4::rotationY(float Radians)
{
    float C = std::cos(Radians), S = std::sin(Radians);
    return Mat4(Vec4(C, 0.0f, -S, 0.0f), Vec4(0.0f, 1.0f, 0.0f, 0.0f), Vec4(S, 0.0f, C, 0.0f),
                Vec4(0.0f, 0.0f, 0.0f, 1.0f));
}
GOLETA_FORCEINLINE Mat4 Mat4::rotationZ(float Radians)
{
    float C = std::cos(Radians), S = std::sin(Radians);
    return Mat4(Vec4(C, S, 0.0f, 0.0f), Vec4(-S, C, 0.0f, 0.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f),
                Vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

GOLETA_FORCEINLINE Vec4 Mat4::row(int I) const { return Vec4(Rows[I]); }
GOLETA_FORCEINLINE Vec4 Mat4::column(int I) const
{
    alignas(16) float Buf[16];
    simd::store(Buf + 0, Rows[0]);
    simd::store(Buf + 4, Rows[1]);
    simd::store(Buf + 8, Rows[2]);
    simd::store(Buf + 12, Rows[3]);
    return Vec4(Buf[I], Buf[I + 4], Buf[I + 8], Buf[I + 12]);
}

GOLETA_FORCEINLINE Mat4 operator+(const Mat4& A, const Mat4& B)
{
    Mat4 R;
    R.Rows[0] = simd::add(A.Rows[0], B.Rows[0]);
    R.Rows[1] = simd::add(A.Rows[1], B.Rows[1]);
    R.Rows[2] = simd::add(A.Rows[2], B.Rows[2]);
    R.Rows[3] = simd::add(A.Rows[3], B.Rows[3]);
    return R;
}
GOLETA_FORCEINLINE Mat4 operator-(const Mat4& A, const Mat4& B)
{
    Mat4 R;
    R.Rows[0] = simd::sub(A.Rows[0], B.Rows[0]);
    R.Rows[1] = simd::sub(A.Rows[1], B.Rows[1]);
    R.Rows[2] = simd::sub(A.Rows[2], B.Rows[2]);
    R.Rows[3] = simd::sub(A.Rows[3], B.Rows[3]);
    return R;
}

// Row-vector convention: (A * B).Row[i] = A.Row[i] as a row-vector times B.
GOLETA_FORCEINLINE Mat4 operator*(const Mat4& A, const Mat4& B)
{
    Mat4 R;
    for (int I = 0; I < 4; ++I)
    {
        simd::Vec4f Ai = A.Rows[I];
        simd::Vec4f Row = simd::mul(simd::splatX(Ai), B.Rows[0]);
        Row = simd::fma(simd::splatY(Ai), B.Rows[1], Row);
        Row = simd::fma(simd::splatZ(Ai), B.Rows[2], Row);
        Row = simd::fma(simd::splatW(Ai), B.Rows[3], Row);
        R.Rows[I] = Row;
    }
    return R;
}

GOLETA_FORCEINLINE Vec4 operator*(Vec4 V, const Mat4& M)
{
    simd::Vec4f R = simd::mul(simd::splatX(V.Data), M.Rows[0]);
    R = simd::fma(simd::splatY(V.Data), M.Rows[1], R);
    R = simd::fma(simd::splatZ(V.Data), M.Rows[2], R);
    R = simd::fma(simd::splatW(V.Data), M.Rows[3], R);
    return Vec4(R);
}

// Column-vector form. Slower than V * M because it packs four dot products;
// prefer V * M when you can.
GOLETA_FORCEINLINE Vec4 operator*(const Mat4& M, Vec4 V)
{
    simd::Vec4f D0 = simd::dot4(M.Rows[0], V.Data);
    simd::Vec4f D1 = simd::dot4(M.Rows[1], V.Data);
    simd::Vec4f D2 = simd::dot4(M.Rows[2], V.Data);
    simd::Vec4f D3 = simd::dot4(M.Rows[3], V.Data);
    simd::Vec4f Xy = simd::shuffle2<0, 1, 0, 1>(D0, D1); // (x,x,y,y)
    simd::Vec4f Zw = simd::shuffle2<0, 1, 0, 1>(D2, D3); // (z,z,w,w)
    return Vec4(simd::shuffle2<0, 2, 0, 2>(Xy, Zw));
}

GOLETA_FORCEINLINE Mat4 transpose(const Mat4& M)
{
    simd::Vec4f T0 = simd::shuffle2<0, 1, 0, 1>(M.Rows[0], M.Rows[1]); // a b e f
    simd::Vec4f T1 = simd::shuffle2<2, 3, 2, 3>(M.Rows[0], M.Rows[1]); // c d g h
    simd::Vec4f T2 = simd::shuffle2<0, 1, 0, 1>(M.Rows[2], M.Rows[3]); // i j m n
    simd::Vec4f T3 = simd::shuffle2<2, 3, 2, 3>(M.Rows[2], M.Rows[3]); // k l o p
    Mat4 R;
    R.Rows[0] = simd::shuffle2<0, 2, 0, 2>(T0, T2); // a e i m
    R.Rows[1] = simd::shuffle2<1, 3, 1, 3>(T0, T2); // b f j n
    R.Rows[2] = simd::shuffle2<0, 2, 0, 2>(T1, T3); // c g k o
    R.Rows[3] = simd::shuffle2<1, 3, 1, 3>(T1, T3); // d h l p
    return R;
}

GOLETA_FORCEINLINE Vec3 transformPoint(const Mat4& M, Vec3 P)
{
    simd::Vec4f R = simd::mul(simd::splatX(P.Data), M.Rows[0]);
    R = simd::fma(simd::splatY(P.Data), M.Rows[1], R);
    R = simd::fma(simd::splatZ(P.Data), M.Rows[2], R);
    R = simd::add(M.Rows[3], R); // implicit W = 1
    return Vec3(simd::set(simd::getX(R), simd::getY(R), simd::getZ(R), 0.0f));
}
GOLETA_FORCEINLINE Vec3 transformDirection(const Mat4& M, Vec3 D)
{
    simd::Vec4f R = simd::mul(simd::splatX(D.Data), M.Rows[0]);
    R = simd::fma(simd::splatY(D.Data), M.Rows[1], R);
    R = simd::fma(simd::splatZ(D.Data), M.Rows[2], R);
    return Vec3(simd::set(simd::getX(R), simd::getY(R), simd::getZ(R), 0.0f));
}

GOLETA_FORCEINLINE Mat3::Mat3(Vec3 R0, Vec3 R1, Vec3 R2)
{
    Rows[0] = R0.Data;
    Rows[1] = R1.Data;
    Rows[2] = R2.Data;
}

GOLETA_FORCEINLINE Mat3 Mat3::identity() { return Mat3(Vec3::unitX(), Vec3::unitY(), Vec3::unitZ()); }
GOLETA_FORCEINLINE Mat3 Mat3::zero()
{
    Mat3 R;
    R.Rows[0] = R.Rows[1] = R.Rows[2] = simd::zero();
    return R;
}

GOLETA_FORCEINLINE Mat3 Mat3::scale(Vec3 S)
{
    return Mat3(Vec3(S.x(), 0.0f, 0.0f), Vec3(0.0f, S.y(), 0.0f), Vec3(0.0f, 0.0f, S.z()));
}
GOLETA_FORCEINLINE Mat3 Mat3::rotationX(float Radians)
{
    float C = std::cos(Radians), S = std::sin(Radians);
    return Mat3(Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, C, S), Vec3(0.0f, -S, C));
}
GOLETA_FORCEINLINE Mat3 Mat3::rotationY(float Radians)
{
    float C = std::cos(Radians), S = std::sin(Radians);
    return Mat3(Vec3(C, 0.0f, -S), Vec3(0.0f, 1.0f, 0.0f), Vec3(S, 0.0f, C));
}
GOLETA_FORCEINLINE Mat3 Mat3::rotationZ(float Radians)
{
    float C = std::cos(Radians), S = std::sin(Radians);
    return Mat3(Vec3(C, S, 0.0f), Vec3(-S, C, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
}

GOLETA_FORCEINLINE Vec3 Mat3::row(int I) const { return Vec3(Rows[I]); }
GOLETA_FORCEINLINE Vec3 Mat3::column(int I) const
{
    alignas(16) float Buf[12];
    simd::store(Buf + 0, Rows[0]);
    simd::store(Buf + 4, Rows[1]);
    simd::store(Buf + 8, Rows[2]);
    return Vec3(Buf[I], Buf[I + 4], Buf[I + 8]);
}

GOLETA_FORCEINLINE Mat3 operator+(const Mat3& A, const Mat3& B)
{
    Mat3 R;
    R.Rows[0] = simd::add(A.Rows[0], B.Rows[0]);
    R.Rows[1] = simd::add(A.Rows[1], B.Rows[1]);
    R.Rows[2] = simd::add(A.Rows[2], B.Rows[2]);
    return R;
}
GOLETA_FORCEINLINE Mat3 operator-(const Mat3& A, const Mat3& B)
{
    Mat3 R;
    R.Rows[0] = simd::sub(A.Rows[0], B.Rows[0]);
    R.Rows[1] = simd::sub(A.Rows[1], B.Rows[1]);
    R.Rows[2] = simd::sub(A.Rows[2], B.Rows[2]);
    return R;
}
GOLETA_FORCEINLINE Mat3 operator*(const Mat3& A, const Mat3& B)
{
    Mat3 R;
    for (int I = 0; I < 3; ++I)
    {
        simd::Vec4f Ai = A.Rows[I];
        simd::Vec4f Row = simd::mul(simd::splatX(Ai), B.Rows[0]);
        Row = simd::fma(simd::splatY(Ai), B.Rows[1], Row);
        Row = simd::fma(simd::splatZ(Ai), B.Rows[2], Row);
        R.Rows[I] = Row;
    }
    return R;
}
GOLETA_FORCEINLINE Vec3 operator*(const Mat3& M, Vec3 V)
{
    simd::Vec4f Dx = simd::dot3(M.Rows[0], V.Data);
    simd::Vec4f Dy = simd::dot3(M.Rows[1], V.Data);
    simd::Vec4f Dz = simd::dot3(M.Rows[2], V.Data);
    return Vec3(simd::set(simd::getX(Dx), simd::getX(Dy), simd::getX(Dz), 0.0f));
}
GOLETA_FORCEINLINE Vec3 operator*(Vec3 V, const Mat3& M)
{
    simd::Vec4f R = simd::mul(simd::splatX(V.Data), M.Rows[0]);
    R = simd::fma(simd::splatY(V.Data), M.Rows[1], R);
    R = simd::fma(simd::splatZ(V.Data), M.Rows[2], R);
    return Vec3(simd::set(simd::getX(R), simd::getY(R), simd::getZ(R), 0.0f));
}

GOLETA_FORCEINLINE Mat3 transpose(const Mat3& M)
{
    return Mat3(Vec3(simd::getX(M.Rows[0]), simd::getX(M.Rows[1]), simd::getX(M.Rows[2])),
                Vec3(simd::getY(M.Rows[0]), simd::getY(M.Rows[1]), simd::getY(M.Rows[2])),
                Vec3(simd::getZ(M.Rows[0]), simd::getZ(M.Rows[1]), simd::getZ(M.Rows[2])));
}

} // namespace goleta::math
