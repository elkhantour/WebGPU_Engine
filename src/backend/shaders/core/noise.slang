module noise;

float2 n22(float2 p)
{
    float3 a = frac(float3(p.x, p.y, p.x) * float3(123.34, 234.34, 345.65));
    a = a + float3(dot(a, a + 34.45));
    return frac(float2(a.x * a.y, a.y * a.z));
}

float2 get_gradient(float2 pos)
{
    float two_pi = 6.283185;
    float angle  = n22(pos).x * two_pi;
    return float2(cos(angle), sin(angle));
}

public float perlin_noise(float2 uv, float cells_count)
{
    float2 pos_in_grid        = uv * cells_count;
    float2 cell_pos_in_grid   = floor(pos_in_grid);
    float2 local_pos_in_cell  = pos_in_grid - cell_pos_in_grid;
    float2 blend = local_pos_in_cell * local_pos_in_cell * (3.0 - 2.0 * local_pos_in_cell);

    float2 left_top     = cell_pos_in_grid + float2(0.0, 1.0);
    float2 right_top    = cell_pos_in_grid + float2(1.0, 1.0);
    float2 left_bottom  = cell_pos_in_grid + float2(0.0, 0.0);
    float2 right_bottom = cell_pos_in_grid + float2(1.0, 0.0);

    float left_top_dot     = dot(pos_in_grid - left_top,     get_gradient(left_top));
    float right_top_dot    = dot(pos_in_grid - right_top,    get_gradient(right_top));
    float left_bottom_dot  = dot(pos_in_grid - left_bottom,  get_gradient(left_bottom));
    float right_bottom_dot = dot(pos_in_grid - right_bottom, get_gradient(right_bottom));

    float noise_value = lerp(lerp(left_bottom_dot, right_bottom_dot, blend.x),
                             lerp(left_top_dot,    right_top_dot,    blend.x), blend.y);
    return 0.5 + 0.5 * (noise_value / 0.7);
}
