#include "include/bind_layouts.h"
import bindings.image_filter;

struct KawaseUniform
{
    float2 texel_size;  // (1.0/width, 1.0/height)
    float  offset;      // current Kawase offset (1,2,3...)
    float  _pad;
    float4 _pad2;
};

[[vk::binding(IMAGE_FILTER_PARAMS_BINDING, IMAGE_FILTER_GROUP)]] ConstantBuffer<KawaseUniform> uKawase;

// Unity shader:
// https://github.com/tomc128/urp-kawase-blur/blob/master/Assets/KawaseBlur/KawaseBlur.shader
// Other resources:
//   https://www.intel.com/content/www/us/en/developer/articles/technical/an-investigation-of-fast-real-time-gpu-based-image-blur-algorithms.html
//   https://github.com/butterw/bShaders/blob/master/blurKawase_15.hlsl
//   https://community.arm.com/cfs-file/__key/communityserver-blogs-components-weblogfiles/00-00-00-20-66/siggraph2015_2D00_mmg_2D00_marius_2D00_notes.pdf

float4 tap(float2 uv, float2 offset)
{
    return src_texture.SampleLevel(src_sampler, uv + offset, 0.0);
}

[shader("compute")]
[numthreads(IMAGE_FILTER_TILE_SIZE, IMAGE_FILTER_TILE_SIZE, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    float2 dims = (float2)src_size();
    float2 uv = ((float2)id.xy + 0.5) / dims;

    float2 o = float2(uKawase.offset) * uKawase.texel_size;

    float4 col = tap(uv, float2( o.x, 0.0));
    col += tap(uv, float2(-o.x, 0.0));
    col += tap(uv, float2(0.0,  o.y));
    col += tap(uv, float2(0.0, -o.y));
    col += tap(uv, float2( o.x,  o.y));
    col += tap(uv, float2( o.x, -o.y));
    col += tap(uv, float2(-o.x,  o.y));
    col += tap(uv, float2(-o.x, -o.y));
    col = col * 0.125;

    dst_texture.Store(id.xy, col);
}
