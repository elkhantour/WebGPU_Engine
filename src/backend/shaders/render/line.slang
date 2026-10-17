import core.types;
import bindings.scene;

[[vk::binding(0, 1)]] ConstantBuffer<float4> uColor;
[[vk::binding(1, 1)]] ConstantBuffer<float>  uThickness;

// Line segments are expanded into screen-facing quads in the vertex shader.
struct LineVertexIn
{
    [[vk::location(0)]] float3 aPosA   : POSITION0;  // anchor
    [[vk::location(1)]] float3 aPosB   : POSITION1;  // opposite endpoint
    [[vk::location(2)]] float4 aUnused : TANGENT;
    [[vk::location(3)]] float3 aCol    : COLOR;
    [[vk::location(4)]] float2 aSide   : TEXCOORD0;  // x: side (-1/+1), y: direction multiplier
};

struct VertexOut
{
    float4 Position : SV_Position;
    [[vk::location(0)]] float3 vCol : COLOR;
};

[shader("vertex")]
VertexOut vs_main(LineVertexIn input)
{
    float3 a = input.aPosA;
    float3 b = input.aPosB;
    float side    = input.aSide.x;
    float dir_mul = input.aSide.y;

    // invert direction on B endpoint
    float3 model_dir = dir_mul * normalize(a - b);

    Mesh mesh = uMesh;
    Camera camera = uCamera;
    Viewport viewport = uViewport;

    float4 world_pos = mul(mesh.model, float4(a, 1.0));
    float4 view_pos  = mul(camera.view, world_pos);

    // model space -> world space -> view space
    float3 world_dir = normalize(mul(mesh.model, float4(model_dir, 0.0)).xyz);
    float3 view_dir  = normalize(mul(camera.view, float4(world_dir, 0.0)).xyz);

    float3 view_2_cam = normalize(-view_pos.xyz);
    float3 world_view_raw = cross(view_dir, view_2_cam);

    float epsilon = 0.0001;
    float3 world_view;
    if (length(world_view_raw) < epsilon)
    {
        float3 view_fwd = float3(0.0, 0.0, -1.0);
        world_view = normalize(cross(view_dir, view_fwd));
    }
    else
    {
        world_view = normalize(world_view_raw);
    }

    float2 perp_view_2D = world_view.xy;
    float abs_view_z = abs(view_pos.z);
    float view_space_offset_x = uThickness * 0.5 * abs_view_z / viewport.projection[0][0];
    float view_space_offset_y = uThickness * 0.5 * abs_view_z / viewport.projection[1][1];

    float3 offset_view_space = float3(perp_view_2D.x * view_space_offset_x,
                                      perp_view_2D.y * view_space_offset_y, 0.0);
    float4 final_view_pos = float4(view_pos.xyz + offset_view_space * side, view_pos.w);

    VertexOut output;
    output.Position = mul(viewport.projection, final_view_pos);
    output.vCol = input.aCol;
    return output;
}

[shader("fragment")]
float4 fs_main(VertexOut input) : SV_Target
{
    return uColor;
}
