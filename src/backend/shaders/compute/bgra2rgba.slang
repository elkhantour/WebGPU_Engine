#include "include/bind_layouts.h"
import bindings.image_filter;

[shader("compute")]
[numthreads(IMAGE_FILTER_TILE_SIZE, IMAGE_FILTER_TILE_SIZE, 1)]
void main(uint3 gid : SV_DispatchThreadID)
{
    if (out_of_bounds(gid.xy))
        return;

    // load BGRA, store RGBA
    float4 bgra = src_texture.Load(int3(gid.xy, 0));
    dst_texture.Store(gid.xy, float4(bgra.z, bgra.y, bgra.x, bgra.w));
}
