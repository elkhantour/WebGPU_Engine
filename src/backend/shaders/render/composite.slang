import core.fullscreen;

struct CompositeUniform
{
    float bloom_intensity;
    float vignette_strength;
    float vignette_radius;
    float exposure;
    float gamma;
};

[[vk::binding(0, 0)]] Texture2D<float4>                scene_texture;
[[vk::binding(1, 0)]] Texture2D<float4>                bloom_texture;
[[vk::binding(2, 0)]] SamplerState                     linear_sampler;
[[vk::binding(3, 0)]] ConstantBuffer<CompositeUniform> uComposite;

[shader("vertex")]
FullscreenOut vs_main(uint vertex_index : SV_VertexID)
{
    return fullscreen_triangle(vertex_index);
}

[shader("fragment")]
float4 fs_main(FullscreenOut input) : SV_Target
{
    float2 uv = input.vUv;

    float3 scene = scene_texture.Sample(linear_sampler, uv).rgb;
    float3 bloom = bloom_texture.Sample(linear_sampler, uv).rgb;
    float3 color = scene + bloom * uComposite.bloom_intensity; // additive bloom

    // vignette
    float2 pos = uv * 2.0 - float2(1.0);
    float dist = length(pos);
    float vig = smoothstep(uComposite.vignette_radius, 1.0, dist);
    float vignette = lerp(1.0, 1.0 - uComposite.vignette_strength, clamp(vig, 0.0, 1.0));
    color = color * vignette;

    // simple tone mapping + exposure
    color = float3(1.0) - exp(-color * uComposite.exposure);

    // gamma
    color = pow(color, float3(1.0 / uComposite.gamma));

    return float4(color, 1.0);
}
