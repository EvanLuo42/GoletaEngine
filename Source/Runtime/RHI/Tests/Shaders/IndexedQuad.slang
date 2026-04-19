// Full-RT quad via indexed draw. Consumes positions from a vertex buffer (layout location 0).

struct VsInput
{
    float2 Position : ATTR0;
};

struct VsOutput
{
    float4 Position : SV_Position;
};

[shader("vertex")]
VsOutput vertexMain(VsInput In)
{
    VsOutput Out;
    Out.Position = float4(In.Position, 0.0f, 1.0f);
    return Out;
}

[shader("pixel")]
float4 pixelMain(VsOutput In) : SV_Target
{
    return float4(0.2f, 0.6f, 0.9f, 1.0f);
}
