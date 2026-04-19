// Simplest non-textured triangle: a VS that emits three NDC-space vertices covering most of the
// render target, and a PS that writes a fixed red color. Root-constants plumbing is deferred to
// a follow-up test (needs per-shader-stage visibility wiring in the root signature).

struct VsOutput
{
    float4 Position : SV_Position;
};

[shader("vertex")]
VsOutput vertexMain(uint VertexId : SV_VertexID)
{
    float2 Positions[3] = {
        float2(-0.8f, -0.8f),
        float2( 0.0f,  0.8f),
        float2( 0.8f, -0.8f),
    };
    VsOutput Out;
    Out.Position = float4(Positions[VertexId], 0.0f, 1.0f);
    return Out;
}

[shader("pixel")]
float4 pixelMain(VsOutput In) : SV_Target
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
