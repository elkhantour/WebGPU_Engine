import core.fullscreen;
import core.color;

struct BloomUniform
{
    float threshold;
    float knee;
    uint  blur;
    uint  downscale;
};

[[vk::binding(0, 0)]] Texture2D<float4>            bloom_texture;
[[vk::binding(1, 0)]] SamplerState                 bloom_sampler;
[[vk::binding(2, 0)]] ConstantBuffer<BloomUniform> uBloom;

[shader("vertex")]
FullscreenOut vs_main(uint vertex_index : SV_VertexID)
{
    return fullscreen_triangle(vertex_index);
}

[shader("fragment")]
float4 fs_main(FullscreenOut input) : SV_Target
{
    float3 color = bloom_texture.Sample(bloom_sampler, input.vUv).rgb;
    float lum = luminance(color);

    float threshold = uBloom.threshold;
    float knee      = uBloom.knee;

    // soft knee: guard against knee == 0
    float soft   = clamp((lum - threshold + knee) / max(2.0 * knee, 1e-5), 0.0, 1.0);
    float bright = max(lum - threshold, 0.0) * soft;

    return float4(color * bright, 1.0);
}
