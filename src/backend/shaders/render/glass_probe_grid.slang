import core.types;
import core.fog;
import bindings.scene;
import lighting.probes;
import materials.glass;

// ---- glass (group 1) -------------------------------------------------------
[[vk::binding(0, 1)]] ConstantBuffer<Glass>       uGlass;
[[vk::binding(1, 1)]] ConstantBuffer<Environment> uEnvironment;
[[vk::binding(2, 1)]] ConstantBuffer<ProbeList>   uProbes;
[[vk::binding(3, 1)]] Texture2DArray<float4>      probe_reflection_maps;
[[vk::binding(4, 1)]] TextureCube<float4>         skybox_map;
[[vk::binding(5, 1)]] SamplerState                linear_sampler;

[shader("vertex")]
GlassVertexOut vs_main(VertexIn input)
{
    return glass_vertex(input, uMesh, uCamera, uViewport);
}

[shader("fragment")]
float4 fs_main(GlassVertexOut input) : SV_Target
{
    Camera camera = uCamera;
    float3 N = normalize(input.vNorm);

    // frost effect
    float3 perturbed_N = frost_normal(N, input.vUv, uGlass);
    float3 V = normalize(camera.position.xyz - input.vFrag);

    // Fresnel + reflection direction (both from the frosted normal)
    float3 f = glass_fresnel(perturbed_N, V);
    float3 R = normalize(reflect(-V, perturbed_N));

    float4 skybox = skybox_map.Sample(linear_sampler, R);

    // TODO: probe reflection (proximity / weighted blending) is not wired yet.
    // alpha = 1 means "fully covered by the (black) probe", as in the original.
    float4 reflection = float4(0.0, 0.0, 0.0, 1.0);

    float4 composite = lerp(skybox, reflection, reflection.a);
    float4 combined  = glass_tint(uGlass, composite, f);
    return apply_fog(combined, uEnvironment.fog, length(input.vFrag - camera.position.xyz));
}
