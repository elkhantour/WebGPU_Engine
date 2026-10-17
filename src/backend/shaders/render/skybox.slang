import core.types;
import core.transform;
import bindings.scene;

[[vk::binding(0, 1)]] TextureCube<float4>    skybox_texture;
[[vk::binding(1, 1)]] SamplerState           skybox_sampler;
[[vk::binding(2, 1)]] ConstantBuffer<float>  skybox_blur;   // reserved (blur path disabled)

struct VertexOut
{
    float4 vPos : SV_Position;
    [[vk::location(0)]] float3 vDir : TEXCOORD0;
};

[shader("vertex")]
VertexOut vs_main([[vk::location(0)]] float3 position : POSITION)
{
    // follow the camera but infinitely far (rotation only, no translation)
    float4x4 rot_only_view = remove_translation(uCamera.view);

    VertexOut output;
    output.vDir = position;
    output.vPos = mul(uViewport.projection, mul(rot_only_view, float4(position, 1.0)));
    return output;
}

[shader("fragment")]
float4 fs_main(VertexOut input) : SV_Target
{
    return skybox_texture.Sample(skybox_sampler, normalize(input.vDir));
}
