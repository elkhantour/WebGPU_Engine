import core.types;
import core.transform;
import bindings.scene;

struct VertexOut
{
    float4 Position : SV_Position;
    [[vk::location(0)]] float3 vCol : COLOR;
};

[shader("vertex")]
VertexOut vs_main(VertexIn input)
{
    VertexOut output;
    output.Position = local_to_clip(uViewport, uCamera, uMesh, input.aPos);
    output.vCol = input.aCol;
    return output;
}

[shader("fragment")]
float4 fs_main(VertexOut input) : SV_Target
{
    return float4(0.0);
}
