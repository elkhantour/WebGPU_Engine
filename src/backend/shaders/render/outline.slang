import core.types;
import bindings.scene;

struct VertexOut
{
    float4 Position : SV_Position;
    [[vk::location(0)]] float3 vCol : COLOR;
};

[shader("vertex")]
VertexOut vs_main(VertexIn input)
{
    Mesh mesh = uMesh;
    Camera camera = uCamera;
    Viewport viewport = uViewport;

    float thickness = 0.0015;

    // world space
    float4 worldPos  = mul(mesh.model, float4(input.aPos, 1.0));
    float3 worldNorm = normalize(mul(mesh.model, float4(input.aNorm, 0.0)).xyz);

    // view space
    float3 viewPos  = mul(camera.view, worldPos).xyz;
    float3 viewNorm = normalize(mul(camera.view, float4(worldNorm, 0.0)).xyz);

    // inflate along the view-space normal, scaled by distance so the outline
    // keeps a constant on-screen thickness
    float dist = length(viewPos);
    float3 inflatedViewPos = viewPos + viewNorm * (thickness * dist);

    VertexOut output;
    output.Position = mul(viewport.projection, float4(inflatedViewPos, 1.0));
    output.vCol = input.aCol;
    return output;
}

[shader("fragment")]
float4 fs_main(VertexOut input) : SV_Target
{
    return float4(1.0, 0.0, 0.0, 1.0);
}
