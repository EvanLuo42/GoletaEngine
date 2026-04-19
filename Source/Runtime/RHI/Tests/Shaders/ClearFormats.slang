// Parametric clear entry points, one per (type, channel-count) combination the format matrix
// exercises. Kept minimal for MVP — test-formats tests can expand by adding more entry points.

RWTexture2D<float4> Target;

[shader("compute")]
[numthreads(8, 8, 1)]
void clearFloat4(uint3 Tid : SV_DispatchThreadID)
{
    Target[Tid.xy] = float4(0.25f, 0.5f, 0.75f, 1.0f);
}
