module image_filter;
#include "include/bind_layouts.h"

// Bind group of every "texture in -> texture out" compute pass.
// Binding IMAGE_FILTER_PARAMS_BINDING (3) is declared by the filter that needs it.
[[vk::binding(IMAGE_FILTER_SRC_BINDING,     IMAGE_FILTER_GROUP)]] public Texture2D<float4> src_texture;
[[vk::binding(IMAGE_FILTER_SAMPLER_BINDING, IMAGE_FILTER_GROUP)]] public SamplerState      src_sampler;
[[vk::binding(IMAGE_FILTER_DST_BINDING,     IMAGE_FILTER_GROUP)]] [format("rgba8")] public WTexture2D<float4> dst_texture;

public uint2 src_size()
{
    uint w, h;
    src_texture.GetDimensions(w, h);
    return uint2(w, h);
}

// true when the invocation is outside the source image (edge workgroups)
public bool out_of_bounds(uint2 gid)
{
    uint2 size = src_size();
    return gid.x >= size.x || gid.y >= size.y;
}
