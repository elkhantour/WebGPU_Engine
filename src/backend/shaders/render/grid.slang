import core.types;
import core.math;
import core.transform;
import bindings.scene;

// NOTE: WebGPU aligns uniform members on 16 bytes. Keep this struct in sync
// with the C side (the padding matters when it is not the last binding).
struct GridData
{
    float4 color;
    float  division;
    float  scale;
    float  thickness;
    float  _padding;
};

[[vk::binding(0, 1)]] ConstantBuffer<GridData> uGrid;

struct VertexOut
{
    float4 Position : SV_Position;
    [[vk::location(0)]] float3 vCol : COLOR;
    [[vk::location(1)]] float2 vUv  : TEXCOORD0;
};

[shader("vertex")]
VertexOut vs_main(VertexIn input)
{
    Mesh mesh = uMesh;
    Camera camera = uCamera;
    Viewport viewport = uViewport;

    // Keep the grid below the camera's look-at point
    float2 offset = float2(camera.lookat.x, camera.lookat.z);
    float4x4 translate = translation_matrix(float3(offset.x, 0.0, offset.y));

    VertexOut output;
    output.Position = mul(viewport.projection,
                      mul(camera.view,
                      mul(translate,
                      mul(mesh.model, float4(input.aPos, 1.0)))));
    output.vCol = input.aCol;
    output.vUv  = input.aUv;
    return output;
}

[shader("fragment")]
float4 fs_main(VertexOut input) : SV_Target
{
    Camera camera = uCamera;
    float2 vUv = input.vUv;
    float2 offset = float2(camera.lookat.x, camera.lookat.z);

    // Setup grid
    float patternSize = 1.0 / uGrid.division;       // size of the tile
    float edge        = patternSize / uGrid.thickness; // size of the edge
    float face_tone   = 0.0;                        // 0.9 for the face of the tile
    float edge_tone   = 1.0;                        // 0.5 for the edge

    // Move UV opposite to the camera direction to compensate the grid translation
    float2 compensUv = float2(vUv.x + offset.x * patternSize,
                              vUv.y + offset.y * patternSize);
    float2 gridUv = fsign(float2(edge) - frac(compensUv / patternSize) * patternSize);
    float4 pattern = float4(face_tone - fsign(gridUv.x + gridUv.y + 1.0) * (face_tone - edge_tone));

    float2 center = float2(0.5, 0.5);
    float3 white = float3(1.0);
    float3 black = float3(0.0);
    float3 x_color = float3(0.91, 0.29, 0.23);
    float3 z_color = float3(0.23, 0.40, 0.91);

    // Add X & Y axis
    float axisThickness = 0.0005;
    float yAxis = step(abs(vUv.x - 0.5 + offset.x / uGrid.scale), axisThickness);
    float xAxis = step(abs(vUv.y - 0.5 + offset.y / uGrid.scale), axisThickness);
    float3 yAxisColor = lerp(black, z_color, float3(yAxis));
    float3 xAxisColor = lerp(black, x_color, float3(xAxis));
    float3 axis = max(xAxisColor, yAxisColor);
    float4 axisMask = float4(min(1.0 - xAxis, 1.0 - yAxis));

    // Radial fade
    float fade = pow(distance(vUv, center), 3.0) * 20.0;
    float ray  = max(min(fade, 1.0), 0.0);
    float3 grad = lerp(white, black, ray);
    float avg = (grad.r + grad.g + grad.b) / 3.0;

    return (uGrid.color * pattern * axisMask + float4(axis, 1.0)) * float4(grad, avg);
}
