module ibl;
import core.math;
import lighting.material;
import lighting.brdf;

// Image based lighting from a skybox cube map.
//   plane_reflection : optional planar reflection (rgb + coverage in a).
//                      Pass float4(0) when unused.
// Returns the ambient contribution (diffuse IBL + specular IBL).
public float3 compute_ibl(Material material, float3 world_normal, float3 V,
                          TextureCube<float4> skybox, SamplerState smp,
                          float4 plane_reflection)
{
    float3 R = normalize(reflect(-V, world_normal));

    float  NdotV = dot_max(world_normal, V);
    float3 F  = fresnel_schlick(base_reflectivity(material), NdotV);
    float3 kD = (1.0 - F) * (1.0 - material.metallic);

    float3 diffuseIBL = (skybox.Sample(smp, world_normal) * material.albedo).rgb;

    uint w, h, levels;
    skybox.GetDimensions(0, w, h, levels);
    float lod = material.roughness * ((float)levels - 1.0);
    float3 specularIBL = skybox.SampleLevel(smp, R, lod).rgb;

    // fusion of skybox + planar reflection as overlay
    float3 specular_composite = lerp(specularIBL, plane_reflection.rgb, plane_reflection.a);
    return kD * diffuseIBL + F * specular_composite;
}
