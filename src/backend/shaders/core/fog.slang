module fog;

public struct Fog
{
    public float4 color;
    public float  start_distance;
    public float  density;
};

public struct Environment
{
    public Fog fog;
};

// https://x.com/JKoukourakis/status/1958800134914949268/photo/1
public float fog_factor(float distance, float fog_start_distance, float fog_density)
{
    float fog_distance = max(distance - fog_start_distance, 0.0);
    float density      = fog_density * 0.001;
    float exp_factor   = 1.0 - exp(-density * fog_distance);
    return clamp(exp_factor, 0.0, 1.0);
}

public float4 apply_fog(float4 color, Fog fog, float distance)
{
    float k = fog_factor(distance, fog.start_distance, fog.density);
    return lerp(color, fog.color, k);
}
