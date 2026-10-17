[[vk::binding(0, 0)]] Texture2D<float4> myTexture;
[[vk::binding(1, 0)]] SamplerState      mySampler;

// https://stackoverflow.com/questions/2588875/whats-the-best-way-to-draw-a-fullscreen-quad-in-opengl-3-2
struct VertexOut
{
    float4 Position : SV_Position;
    [[vk::location(0)]] float2 vUv : TEXCOORD0;
};

[shader("vertex")]
VertexOut vs_main(uint vertex_index : SV_VertexID)
{
    static const float2 vertices[3] = {
        float2(-1.0, -1.0), float2(3.0, -1.0), float2(-1.0, 3.0)
    };

    VertexOut output;
    output.Position = float4(vertices[vertex_index], 0.0, 1.0);
    output.vUv = float2(0.5, -0.5) * output.Position.xy + float2(0.5, 0.5);
    return output;
}

[shader("fragment")]
float4 fs_main(VertexOut input) : SV_Target
{
    return myTexture.Sample(mySampler, input.vUv);
}
