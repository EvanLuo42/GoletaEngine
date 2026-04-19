// Reads from a structured input buffer (SRV) and writes doubled values to a UAV output.

StructuredBuffer<uint>   Input;
RWStructuredBuffer<uint> Output;

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 Tid : SV_DispatchThreadID)
{
    Output[Tid.x] = Input[Tid.x] * 2u;
}
