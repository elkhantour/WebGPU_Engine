import core.types;

// Screen-space quad: this pass does NOT use the scene bind group,
// only the mesh at @group(0) @binding(0).
[[vk::binding(0, 0)]] ConstantBuffer<Mesh> uMesh;

struct TextureCoordinates
{
    float2 offset;
    float2 scale;
    float4 color;
};

[[vk::binding(0, 1)]] Texture2D<float4>                    screen_texture;
[[vk::binding(1, 1)]] SamplerState                         texture_sampler;
[[vk::binding(2, 1)]] ConstantBuffer<TextureCoordinates>   uTexture;

struct VertexOut
{
    float4 Position : SV_Position;
    [[vk::location(0)]] float3 vCol : COLOR;
    [[vk::location(1)]] float2 vUv  : TEXCOORD0;
};

[shader("vertex")]
VertexOut vs_main(VertexIn input)
{
    float4 model_pos = mul(uMesh.model, float4(input.aPos, 1.0));

    VertexOut output;
    output.Position = float4(model_pos.xz, 0.0, 1.0);
    output.vCol = input.aCol;
    output.vUv  = input.aUv;
    return output;
}

[shader("fragment")]
float4 fs_main(VertexOut input) : SV_Target
{
    float4 tex = screen_texture.Sample(texture_sampler, input.vUv * uTexture.scale + uTexture.offset);
    return lerp(uTexture.color, tex, tex.a);
}
