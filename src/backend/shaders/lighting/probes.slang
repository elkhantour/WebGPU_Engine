module probes;
import core.constants;
import core.math;

public struct PlaneReflection
{
    public float3 position;
    public float  near;
    public float3 normal;
    public float  far;
    public float3 scale;
    public float  distance;
    public float3 tangent;
    public float  signed_distance;
    public float3 bitangent;
    public uint   texture_layer;
    public column_major float4x4 view;
};

public struct ProbeReflection
{
    public float3 position;
    public float  radius;
};

public struct ProbeList
{
    public PlaneReflection reflection_plane[PROBE_ARRAY_CAPACITY];
    public PlaneReflection reflection_probe[PROBE_ARRAY_CAPACITY];
    public uint probe_count;
    public uint plane_count;
    public uint irradiance_count;
};

// World position -> UV in a planar reflection texture
public float2 compute_reflection_uv(float4x4 projection, float4x4 r_view, float3 frag_pos)
{
    float4 clip = mul(projection, mul(r_view, float4(frag_pos, 1.0)));
    float3 ndc  = clip.xyz / clip.w;   // [-1, 1] space
    return ndc_to_uv(ndc.xy);          // [0, 1] space
}
