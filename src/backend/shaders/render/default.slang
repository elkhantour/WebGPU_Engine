// Attribute declarations
import core.types;
import core.transform;
import bindings.scene;

struct VertexOut
{
    float4 Position : SV_Position;
    [[vk::location(0)]] float3 vCol : TEXCOORD0;
    [[vk::location(1)]] float2 vUv : TEXCOORD1;
};


// Vertex shader

[shader("vertex")]
VertexOut vs_main(VertexIn input)
{
    Mesh mesh = uMesh;
    Camera camera = uCamera;
    Viewport viewport = uViewport;

    // Final matrix: Projection * View
    float4x4 cam = mul(viewport.projection, camera.view);

    VertexOut output;
    output.Position = mul(mul(cam, mesh.model), float4(input.aPos, 1.0));
    output.vCol = input.aCol;
    output.vUv = input.aUv;

    return output;
}

// Fragment shader

[shader("fragment")]
float4 fs_main(VertexOut input) : SV_Target0
{
    return float4(input.vCol, 1.0);
}
