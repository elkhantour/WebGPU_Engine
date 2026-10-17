import core.types;
import core.transform;
import bindings.scene;

[[vk::binding(0, 1)]] Texture2D<float4>   billboard_texture;
[[vk::binding(1, 1)]] SamplerState        texture_sampler;
[[vk::binding(2, 1)]] ConstantBuffer<uint> uScale;   // 0 = constant screen size

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

    float3 look = normalize(camera.position.xyz - mesh.position.xyz);
    float3 worldUp = float3(0.0, 1.0, 0.0);

    float3 scale = model_scale(mesh.model);
    float scale_factor = 1.0;

    if (uScale == 0u)
    {
        float target_pixel = 10.0;
        float4 view_pos = mul(camera.view, float4(mesh.position.xyz, 1.0));

        float2 pixel_size_ndc = 2.0 / float2((float)viewport.width, (float)viewport.height);
        float2 ndc_size = pixel_size_ndc * target_pixel;

        // convert ndc -> world space scale
        scale_factor = view_pos.z * -ndc_size.y;
    }

    float3 flatToCamera = normalize(float3(look.x, 0.0, look.z));
    float3 right = normalize(cross(worldUp, flatToCamera));
    float3 up = cross(look, right);

    float3 local_position = input.aPos.x * right * scale.x * scale_factor
                          + input.aPos.z * up    * scale.z * scale_factor;
    float3 world_position = mesh.position.xyz + local_position;

    VertexOut output;
    output.Position = world_to_clip(viewport, camera, float4(world_position, 1.0));
    output.vCol = input.aCol;
    output.vUv  = input.aUv;
    return output;
}

[shader("fragment")]
float4 fs_main(VertexOut input) : SV_Target
{
    return billboard_texture.Sample(texture_sampler, input.vUv);
}
