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

    // frost effect (used for the reflection tint only, Fresnel uses the clean normal)
    float3 V = normalize(camera.position.xyz - input.vFrag);
    float3 f = glass_fresnel(N, V);

    float3 view_dir = normalize(input.vFrag - camera.position.xyz);
    float3 R = reflect(view_dir, N);

    float4 skybox = skybox_map.Sample(linear_sampler, R);

    // TODO: planar reflection loop (disabled). Per plane i < uMesh.probe_reflection_plane_count:
    //   uv = compute_reflection_uv(uViewport.projection, uProbes.reflection_plane[i].view, vFrag)
    //   reflection = probe_reflection_maps.SampleLevel(linear_sampler,
    //                    float3(uv, texture_layer), 2.0);  reflection.a /= 1.3;
    float4 reflection = float4(0.0);

    float4 composite = lerp(skybox, reflection, reflection.a);
    float4 combined  = glass_tint(uGlass, composite, f);
    return apply_fog(combined, uEnvironment.fog, length(input.vFrag - camera.position.xyz));
}
