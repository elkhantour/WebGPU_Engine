#include "colormap.h"
#include "backend/compute/core.h"
#include "backend/context.h"
#include "backend/logger.h"
#include "backend/resource_manager.h"
#include "backend/shaders/core.h"
#include "runtime/shader/core.h"
#include "runtime/texture/core.h"
#include "webgpu/webgpu.h"
#include <cglm/cglm.h>
#include <math.h>
#include <stdint.h>

static inline void compute_pass_colormap_cache_resources(ComputePass *);

ColormapStatus compute_pass_colormap_create(ComputePass *pass,
                                            const ComputePassDescriptor *desc) {

  uint32_t layer_count = wgpuTextureGetDepthOrArrayLayers(desc->source_texture);

  if (layer_count > COMPUTE_PASS_BINDGROUP_CAPACITY) {
    logger_add(
        LoggerFlag_Error,
        "Layer count for compute pass '%s' is too high (%u), layer count "
        "should be smaller than %u.",
        desc->label, layer_count, COMPUTE_PASS_VIEW_CAPACITY / 2);
    return ColormapStatus_OutOfBound;
  }

  compute_pass_create(pass, desc);

  pass->buffers[COLORMAP_BUFFER_UNIFORM] =
      rem_new_buffer(&(WGPUBufferDescriptor){
          .label = "Colormap Pass Buffer",
          .usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst,
          .mappedAtCreation = false,
          .size = sizeof(ColormapUniform),
      });

  compute_pass_colormap_update_source_texture(pass, desc->source_texture);

  return ColormapStatus_Success;
}

ColormapStatus compute_pass_colormap_draw(ComputePass *pass) {

  WGPUCommandEncoder command_encoder =
      wgpuDeviceCreateCommandEncoder(context_device(), NULL);

  compute_pass_colormap_draw_inline(pass, command_encoder);

  WGPUCommandBuffer compute_buffer =
      wgpuCommandEncoderFinish(command_encoder, NULL);

  wgpuQueueSubmit(context_queue(), 1, &compute_buffer);

  wgpuCommandEncoderRelease(command_encoder);
  wgpuCommandBufferRelease(compute_buffer);

  return ColormapStatus_Success;
}

ColormapStatus
compute_pass_colormap_update_source_texture(ComputePass *pass,
                                            const WGPUTexture texture) {

  pass->source_texture = texture;

  // free resources with old texture
  uint8_t i;
  for (i = 0; i < COMPUTE_PASS_VIEW_CAPACITY; i++)
    rem_destroy_view(&pass->views[i]);

  for (i = 0; i < COMPUTE_PASS_BINDGROUP_CAPACITY; i++)
    if (pass->bindgroups[i]) {
      wgpuBindGroupRelease(pass->bindgroups[i]);
      pass->bindgroups[i] = NULL;
    }

  // rebuild bindgroups
  compute_pass_colormap_cache_resources(pass);

  return ColormapStatus_Success;
}

void compute_pass_colormap_cache_resources(ComputePass *pass) {

  const ComputePipeline *compute_pipeline =
      std_compute_pipeline(ComputePipelineType_Colormap);

  const WGPUComputePipeline pipeline = compute_pipeline->handle;
  const WGPUBindGroupLayout bind_group_layout =
      wgpuComputePipelineGetBindGroupLayout(pipeline, 0);

  const WGPUTexture a_tex = pass->source_texture;
  const WGPUTexture b_tex = pass->buffer_texture;

  for (uint32_t i = 0; i < pass->layer_count; i++) {

    WGPUTextureViewDescriptor a_desc = {
        .format = wgpuTextureGetFormat(pass->source_texture),
        .dimension = WGPUTextureViewDimension_2D,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = i,
        .arrayLayerCount = 1,
    };

    WGPUTextureViewDescriptor b_desc = a_desc;
    b_desc.baseArrayLayer = 0; // reset array

    const uint32_t index = i * 2;

    pass->views[index] = rem_new_view(a_tex, &a_desc);
    pass->views[index + 1] = rem_new_view(b_tex, &b_desc);

    WGPUBindGroupEntry entries[] = {
        {.binding = 0, .textureView = pass->views[index]},
        {.binding = 1, .sampler = pass->sampler},
        {.binding = 2, .textureView = pass->views[index + 1]},
        {.binding = 3,
         .buffer = pass->buffers[COLORMAP_BUFFER_UNIFORM],
         .size = sizeof(ColormapUniform)},
    };

    const int entries_count = sizeof(entries) / sizeof(entries[0]);

    pass->bindgroups[i] = wgpuDeviceCreateBindGroup(
        context_device(), &(WGPUBindGroupDescriptor){
                              .layout = bind_group_layout,
                              .entryCount = entries_count,
                              .entries = entries,
                          });
  }
}

ColormapStatus compute_pass_colormap_set_colors(ComputePass *pass,
                                                const ColormapUniform *data) {

  rem_write_buffer(pass->buffers[KAWASE_BUFFER_UNIFORM], 0, (void *)data,
                   sizeof(ColormapUniform), REMWriteFlag_None);
  return ColormapStatus_Success;
}

ColormapStatus
compute_pass_colormap_draw_inline(ComputePass *pass,
                                  const WGPUCommandEncoder command_encoder) {

  const ComputePipeline *compute_pipeline =
      std_compute_pipeline(ComputePipelineType_Colormap);

  const WGPUComputePipeline pipeline = compute_pipeline->handle;

  WGPUTexture a_tex = pass->source_texture;
  WGPUTexture b_tex = pass->buffer_texture;

  for (uint32_t i = 0; i < pass->layer_count; i++) {

    WGPUBindGroup bind_group = pass->bindgroups[i];

    /* === PASS  BEGIN === */
    WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(
        command_encoder,
        &(WGPUComputePassDescriptor){.label = "Colormap Compute Pass"});

    wgpuComputePassEncoderSetPipeline(compute_pass, pipeline);
    wgpuComputePassEncoderSetBindGroup(compute_pass, 0, bind_group, 0, NULL);

    compute_pass_dispatch(compute_pass, COLORMAP_WORKGROUP, pass->width,
                          pass->height);

    /* === PASS END === */
    wgpuComputePassEncoderEnd(compute_pass);
    wgpuComputePassEncoderRelease(compute_pass);

    // copy the buffer to source texture
    wgpuCommandEncoderCopyTextureToTexture(
        command_encoder,
        &(WGPUImageCopyTexture){
            .texture = b_tex,
            .mipLevel = 0,
            .aspect = WGPUTextureAspect_All,
        },
        &(WGPUImageCopyTexture){.texture = a_tex,
                                .mipLevel = 0,
                                .origin = {0, 0, i},
                                .aspect = WGPUTextureAspect_All},
        &(WGPUExtent3D){pass->width, pass->height, 1});
  }

  return ColormapStatus_Success;
}
