module shadows;
import core.math;
import lighting.lights;

// ---------------------------------------------------------------------------
// Shadow maps come in two flavours, selected at compile time:
//   default                      : depth textures + comparison sampler
//   -DRENDER_SHADOW_AS_COLOR     : shadow depth stored in a colour channel
// Passes should declare their resources with these aliases.
// ---------------------------------------------------------------------------
#ifdef RENDER_SHADOW_AS_COLOR
public typealias PointShadowMaps       = TextureCubeArray<float4>;
public typealias DirectionalShadowMaps = Texture2DArray<float4>;
public typealias ShadowSampler         = SamplerState;
#else
public typealias PointShadowMaps       = DepthTextureCubeArray;
public typealias DirectionalShadowMaps = DepthTexture2DArray;
public typealias ShadowSampler         = SamplerComparisonState;
#endif

static const uint  PCF_KERNEL_MAX   = 0u;
public static const float PCF_MAX_DISTANCE = 5.0;

public uint kernel_from_distance(float3 frag_pos, float3 cam_pos)
{
    float d = distance(frag_pos, cam_pos);
    return d > 2.0 ? 0u : 3u;
}

public float compute_point_depth(float position, float near, float far)
{
    float norm_z_comp = (-position * -1.0 * (far + near)) / (far - near) + ((-2.0 * far * near) / (far - near));
    float depth = (far + near) / (far - near) - 2.0 * far * near / (norm_z_comp * (far - near));
    return (depth + 1.0) * 0.5;
}

// World position -> (shadow uv, depth to compare)
public float3 directional_shadow_position(float3 world_position, float4x4 light_view)
{
    float4 shadow_clip = mul(light_view, float4(world_position, 1.0));
    float3 shadow_ndc  = shadow_clip.xyz / shadow_clip.w;
    return float3(ndc_to_uv(shadow_ndc.xy), shadow_ndc.z);
}

float shadow_map_width(DirectionalShadowMaps maps)
{
    uint w, h, layers;
    maps.GetDimensions(w, h, layers);
    return (float)w;
}

public float point_shadow_factor(ConstantBuffer<LightList> lights, PointShadowMaps maps,
                                 ShadowSampler smp, float3 frag_position)
{
    float factor = max((float)lights.point_count, 1.0); // visible by default
    float bias   = 0.002;

    for (uint l = 0u; l < lights.point_count; l++)
    {
        PointLight light = lights.point_entries[l];
        float3 frag_to_light = frag_position - light.position;
#ifdef RENDER_SHADOW_AS_COLOR
        float far = 100.0;
        float current_depth = length(frag_to_light);
        float closest_depth = maps.Sample(smp, float4(normalize(frag_to_light), (float)l)).r * far;
        factor = (current_depth - bias > closest_depth) ? 0.0 : 1.0;
#else
        float3 abs_vec = abs(frag_to_light);
        float local_z_comp = max(abs_vec.x, max(abs_vec.y, abs_vec.z));
        float3 shadow_direction = normalize(frag_to_light);
        float depth = compute_point_depth(local_z_comp, light.near, light.far);
        float dist  = length(frag_to_light) / light.far;

        factor = smoothstep(light.cutoff, light.inner_cutoff, dist);
        factor = pow(factor, light.intensity);
        factor = dist > light.cutoff ? 0.0 : factor;

        // PCF
        float pcf = 0.0;
        uint  pcf_count = 0u;
        float half_kernel = (float)(PCF_KERNEL_MAX / 2u);
        float spread = 0.08;
        for (float x = -half_kernel; x <= half_kernel; x += 1.0)
        for (float y = -half_kernel; y <= half_kernel; y += 1.0)
        for (float z = -half_kernel; z <= half_kernel; z += 1.0)
        {
            float3 offset = float3(x, y, z) / light.far * spread;
            pcf += maps.SampleCmp(smp, float4(shadow_direction + offset, (float)l), depth - bias);
            pcf_count += 1u;
        }
        pcf /= (float)pcf_count;
        factor *= pcf;
#endif
    }
    return factor;
}

public float spot_shadow_factor(ConstantBuffer<LightList> lights, DirectionalShadowMaps maps,
                                ShadowSampler smp, float3 frag_position)
{
    float factor = max((float)lights.spot_count, 1.0); // visible by default
    float bias   = 0.001;

    for (uint l = 0u; l < lights.spot_count; l++)
    {
        SpotLight light = lights.spot_entries[l];
        float3 shadow_position = directional_shadow_position(frag_position, light.view);
#ifdef RENDER_SHADOW_AS_COLOR
        factor *= maps.Sample(smp, float3(shadow_position.xy, (float)l)).r;
#else
        float3 light_direction = normalize(light.position - frag_position);
        float spot_dot = dot(-light_direction, normalize(light.lookat - light.position));
        float shadow_depth = shadow_position.z - bias;

        factor = smoothstep(light.cutoff, light.inner_cutoff, spot_dot);
        factor = pow(factor, light.intensity);
        factor = spot_dot < light.cutoff ? 0.0 : factor;

        // PCF
        float pcf = 0.0;
        uint  pcf_count = 0u;
        float half_kernel = (float)(PCF_KERNEL_MAX / 2u);
        float spread = 1.0;
        for (float x = -half_kernel; x <= half_kernel; x += 1.0)
        for (float y = -half_kernel; y <= half_kernel; y += 1.0)
        {
            float2 offset_uv = float2(x, y) / shadow_map_width(maps) * spread;
            pcf += maps.SampleCmp(smp, float3(shadow_position.xy + offset_uv, (float)l), shadow_depth);
            pcf_count += 1u;
        }
        pcf /= (float)pcf_count;
        factor *= pcf;
#endif
    }
    return factor;
}

public float sun_shadow_factor(ConstantBuffer<LightList> lights, DirectionalShadowMaps maps,
                               ShadowSampler smp, float3 frag_position)
{
    float factor = max((float)lights.sun_count, 1.0); // visible by default
    float bias   = 0.001;

    for (uint l = 0u; l < lights.sun_count; l++)
    {
        // sun shadow layers come after the spot light layers
        uint layer = lights.spot_count + l;
        SunLight light = lights.sun_entries[l];
        float3 shadow_position = directional_shadow_position(frag_position, light.view);
#ifdef RENDER_SHADOW_AS_COLOR
        factor *= maps.Sample(smp, float3(shadow_position.xy, (float)layer)).r;
#else
        float shadow_depth = shadow_position.z - bias;

        // PCF
        float pcf = 0.0;
        uint  pcf_count = 0u;
        float half_kernel = (float)(PCF_KERNEL_MAX / 2u);
        float spread = 1.0;
        for (float x = -half_kernel; x <= half_kernel; x += 1.0)
        for (float y = -half_kernel; y <= half_kernel; y += 1.0)
        {
            float2 offset_uv = float2(x, y) / shadow_map_width(maps) * spread;
            pcf += maps.SampleCmp(smp, float3(shadow_position.xy + offset_uv, (float)layer), shadow_depth);
            pcf_count += 1u;
        }
        pcf /= (float)pcf_count;
        factor *= pcf;
#endif
    }
    return factor;
}
