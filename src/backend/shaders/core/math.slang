module math;

public float dot_max(float3 a, float3 b) { return max(dot(a, b), 0.0); }

// WGSL sign() returns a float; Slang/HLSL sign() returns an int.
public float  fsign(float x)   { return x > 0.0 ? 1.0 : (x < 0.0 ? -1.0 : 0.0); }
public float2 fsign(float2 v)  { return float2(fsign(v.x), fsign(v.y)); }

// NDC [-1,1] (y up) -> texture UV [0,1] (y down)
public float2 ndc_to_uv(float2 ndc) { return ndc * float2(0.5, -0.5) + float2(0.5, 0.5); }

// Perspective depth -> [0,1] linear depth
// see: https://learnopengl.com/Advanced-OpenGL/Depth-testing
public float linearize_depth(float z, float near, float far)
{
    return (2.0 * near) / (far + near - z * (far - near));
}
