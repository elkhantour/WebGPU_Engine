module glass;
import core.types;
import core.transform;
import core.noise;
import lighting.brdf;

public struct Glass
{
    public float  roughness;
    public float  frost_scale;
    public float  frost_strength;
    public float  _pad;
    public float4 color;
};

public struct GlassVertexOut
{
    public float4 Position : SV_Position;
    [[vk::location(0)]] public float3 vCol  : COLOR;
    [[vk::location(1)]] public float3 vNorm : NORMAL;
    [[vk::location(2)]] public float3 vFrag : TEXCOORD1;
    [[vk::location(3)]] public float2 vUv   : TEXCOORD0;
};

public GlassVertexOut glass_vertex(VertexIn input, Mesh mesh, Camera camera, Viewport viewport)
{
    float4 world = local_to_world(mesh, input.aPos);

    GlassVertexOut output;
    output.Position = world_to_clip(viewport, camera, world);
    output.vCol  = input.aCol;
    output.vNorm = input.aNorm;
    output.vFrag = world.xyz;
    output.vUv   = input.aUv;
    return output;
}

// Frost effect: perturb the normal with perlin noise
public float3 frost_normal(float3 N, float2 uv, Glass glass)
{
    float n = perlin_noise(uv, glass.frost_scale);
    return normalize(N + n * glass.frost_strength);
}

// Dielectric Fresnel term
public float3 glass_fresnel(float3 N, float3 V)
{
    float NdotV = max(dot(N, V), 0.0);
    return fresnel_schlick(float3(0.04), NdotV);
}

// Tint the reflection/transmission by the glass colour, less so at grazing angles
public float4 glass_tint(Glass glass, float4 composite, float3 fresnel)
{
    return lerp(glass.color * composite, composite, fresnel.r);
}
