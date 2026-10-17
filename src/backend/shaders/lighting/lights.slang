module lights;
import core.constants;

public struct PointLight
{
    public float3 position;
    public float  cutoff;
    public float4 color;
    public column_major float4x4 views[POINT_LIGHT_VIEWS];
    public float  intensity;
    public float  inner_cutoff;
    public float  near;
    public float  far;
};

public struct AmbientLight
{
    public float4 color;
    public float  intensity;
};

public struct SpotLight
{
    public float3 position;
    public float  cutoff;
    public float3 lookat;
    public float  inner_cutoff;
    public float4 color;
    public column_major float4x4 view;
    public float  intensity;
};

public struct SunLight
{
    public float3 position;
    public float  intensity;
    public float4 color;
    public column_major float4x4 view;
};

public struct LightList
{
    public AmbientLight ambient_entries[LIGHT_ARRAY_CAPACITY];
    public PointLight   point_entries[LIGHT_ARRAY_CAPACITY];
    public SpotLight    spot_entries[LIGHT_ARRAY_CAPACITY];
    public SunLight     sun_entries[LIGHT_ARRAY_CAPACITY];
    public uint ambient_count;
    public uint point_count;
    public uint spot_count;
    public uint sun_count;
};
