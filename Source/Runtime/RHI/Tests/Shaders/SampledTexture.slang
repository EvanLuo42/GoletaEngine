// Samples a 2D texture using a static point sampler and writes its center texel into a UAV.

Texture2D<float4>     InputTex;
SamplerState          InputSampler;
RWStructuredBuffer<float4> Output;

[shader("compute")]
[numthreads(1, 1, 1)]
void computeMain()
{
    Output[0] = InputTex.SampleLevel(InputSampler, float2(0.5f, 0.5f), 0.0f);
}
