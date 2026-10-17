#include "include/bind_layouts.h"
import core.color;
import bindings.image_filter;

struct ColormapUniform
{
    float4 colors[32];
    uint   count;
};

[[vk::binding(IMAGE_FILTER_PARAMS_BINDING, IMAGE_FILTER_GROUP)]] ConstantBuffer<ColormapUniform> uMap;

[shader("compute")]
[numthreads(IMAGE_FILTER_TILE_SIZE, IMAGE_FILTER_TILE_SIZE, 1)]
void main(uint3 gid : SV_DispatchThreadID)
{
    if (out_of_bounds(gid.xy))
        return;

    float4 color = src_texture.Load(int3(gid.xy, 0));
    float lum = luminance(color.rgb);  // 0..1

    uint n = uMap.count;
    if (n == 1u)
    {
        dst_texture.Store(gid.xy, uMap.colors[0]);
        return;
    }

    float step = 1.0 / (float)(n - 1u);

    // find the gradient segment
    float idx_f = lum / step;
    uint  i = clamp((uint)idx_f, 0u, n - 2u);  // segment start index
    float t = frac(idx_f);                     // blend factor inside the segment
    float4 remap_color = lerp(uMap.colors[i], uMap.colors[i + 1u], t);

    dst_texture.Store(gid.xy, float4(remap_color.rgb, color.a));
}
