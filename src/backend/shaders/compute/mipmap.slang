#include "include/bind_layouts.h"
import bindings.image_filter;

// 2x2 box filter: each invocation writes one texel of the next mip level.
// https://eliemichel.github.io/LearnWebGPU/basic-compute/image-processing/mipmap-generation.html
[shader("compute")]
[numthreads(IMAGE_FILTER_TILE_SIZE, IMAGE_FILTER_TILE_SIZE, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint2 base = 2u * id.xy;

    float4 color = (src_texture.Load(int3(base + uint2(0u, 0u), 0))
                  + src_texture.Load(int3(base + uint2(0u, 1u), 0))
                  + src_texture.Load(int3(base + uint2(1u, 0u), 0))
                  + src_texture.Load(int3(base + uint2(1u, 1u), 0))) * 0.25;

    dst_texture.Store(id.xy, color);
}
