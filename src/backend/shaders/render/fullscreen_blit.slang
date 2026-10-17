import core.fullscreen;

[[vk::binding(0, 0)]] Texture2D<float4> myTexture;
[[vk::binding(1, 0)]] SamplerState      mySampler;

[shader("vertex")]
FullscreenOut vs_main(uint vertex_index : SV_VertexID)
{
    return fullscreen_triangle(vertex_index);
}

[shader("fragment")]
float4 fs_main(FullscreenOut input) : SV_Target
{
    return myTexture.Sample(mySampler, input.vUv);
}
