// SM 6.6 bindless compute test shader. Root constants carry the bindless UAV index via Slang's
// DescriptorHandle<T>. On DXIL this lowers to ResourceDescriptorHeap[idx][i]; on Vulkan it
// becomes descriptor-indexing with nonuniform qualifier automatically.

struct Push
{
    DescriptorHandle<RWStructuredBuffer<uint>> Output;
};
[[vk::push_constant]] ConstantBuffer<Push> P;

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 Tid : SV_DispatchThreadID)
{
    P.Output[Tid.x] = Tid.x * 2u;
}
