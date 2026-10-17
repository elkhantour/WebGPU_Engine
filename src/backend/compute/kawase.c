#include "kawase.h"
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

static inline void compute_pass_kawase_cache_resources(ComputePass *);

KawaseStatus compute_pass_kawase_create(ComputePass *pass,
                                        const ComputePassDescriptor *desc) {

  uint32_t layer_count = wgpuTextureGetDepthOrArrayLayers(desc->source_texture);

  // For each layer we need to cache two bindgroups and views. So need to check
  // if capacity is right.
  if (layer_count > 2 * COMPUTE_PASS_VIEW_CAPACITY ||
      layer_count > 2 * COMPUTE_PASS_BINDGROUP_CAPACITY) {
    logger_add(
        LoggerFlag_Error,
        "Layer count for compute pass '%s' is too high (%u), layer count "
        "should be smaller than %u.",
        desc->label, layer_count, COMPUTE_PASS_VIEW_CAPACITY / 2);
    return KawaseStatus_OutOfBound;
  }

  compute_pass_create(pass, desc);

  pass->buffers[KAWASE_BUFFER_UNIFORM] = rem_new_buffer(&(WGPUBufferDescriptor){
      .label = "Kawase Pass Buffer",
      .usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst,
      .mappedAtCreation = false,
      .size = sizeof(KawaseUniform),
  });

  compute_pass_kawase_update_source_texture(pass, desc->source_texture);

  return KawaseStatus_Success;
}

KawaseStatus compute_pass_kawase_draw(ComputePass *pass,
                                      const uint32_t pass_count) {

  WGPUCommandEncoder command_encoder =
      wgpuDeviceCreateCommandEncoder(context_device(), NULL);

  compute_pass_kawase_draw_inline(pass, pass_count, command_encoder);

  WGPUCommandBuffer compute_buffer =
      wgpuCommandEncoderFinish(command_encoder, NULL);

  wgpuQueueSubmit(context_queue(), 1, &compute_buffer);

  wgpuCommandEncoderRelease(command_encoder);
  wgpuCommandBufferRelease(compute_buffer);

  return KawaseStatus_Success;
}

KawaseStatus
compute_pass_kawase_update_source_texture(ComputePass *pass,
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
  compute_pass_kawase_cache_resources(pass);

  return KawaseStatus_Success;
}

void compute_pass_kawase_cache_resources(ComputePass *pass) {

  const ComputePipeline *compute_pipeline =
      std_compute_pipeline(ComputePipelineType_Kawase);

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
         .buffer = pass->buffers[KAWASE_BUFFER_UNIFORM],
         .size = sizeof(KawaseUniform)},
    };

    const int entries_count = sizeof(entries) / sizeof(entries[0]);

    pass->bindgroups[index] = wgpuDeviceCreateBindGroup(
        context_device(), &(WGPUBindGroupDescriptor){
                              .layout = bind_group_layout,
                              .entryCount = entries_count,
                              .entries = entries,
                          });

    // swap views
    entries[0].textureView = pass->views[index + 1];
    entries[2].textureView = pass->views[index];
    pass->bindgroups[index + 1] = wgpuDeviceCreateBindGroup(
        context_device(), &(WGPUBindGroupDescriptor){
                              .layout = bind_group_layout,
                              .entryCount = entries_count,
                              .entries = entries,
                          });
  }
}

KawaseStatus
compute_pass_kawase_draw_inline(ComputePass *pass, const uint32_t pass_count,
                                const WGPUCommandEncoder command_encoder) {

  const ComputePipeline *compute_pipeline =
      std_compute_pipeline(ComputePipelineType_Kawase);

  const WGPUComputePipeline pipeline = compute_pipeline->handle;

  // Using a predefined set of offset seems to give smoother result than
  // increment offset by K (1.0f) each pass count.
  // static const int offset_count = 5;
  // static const float offset[] = {0.5f, 1.5f, 2.5f, 2.5f, 3.0f};

  static const float offset[] = {
      0.5f, 1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f, 8.5f, 9.5f, 10.5f, 11.5f,
      12.5f, 13.5f, 14.5f, 15.5f,

      // softening plateau (more Kawase-like)
      16.0f, 16.5f, 17.0f, 17.5f, 18.0f, 18.5f, 19.0f, 19.5f,

      // tail falloff (slower growth)
      20.0f, 20.5f, 21.0f, 21.5f, 22.0f, 22.5f, 23.0f, 23.5f};

  static const int offset_count = sizeof(offset) / sizeof(offset[0]);

  KawaseUniform uniform = {
      .texel_size =
          {
              texture_size_texel(pass->width),
              texture_size_texel(pass->height),
          },
      .offset = offset[0],
  };

  WGPUTexture a_tex = pass->source_texture;
  WGPUTexture b_tex = pass->buffer_texture;

  for (uint32_t i = 0; i < pass->layer_count; i++) {

    const uint32_t index = i * 2;
    WGPUBindGroup bind_group_a = pass->bindgroups[index];
    WGPUBindGroup bind_group_b = pass->bindgroups[index + 1];

    for (uint32_t j = 0; j < pass_count; j++) {

      /* === PASS  BEGIN === */
      WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(
          command_encoder,
          &(WGPUComputePassDescriptor){.label = "Kawase Compute Pass"});

      wgpuComputePassEncoderSetPipeline(compute_pass, pipeline);

      // update offset uniform
      rem_write_buffer(pass->buffers[KAWASE_BUFFER_UNIFORM], 0, &uniform,
                       sizeof(KawaseUniform), REMWriteFlag_None);

      wgpuComputePassEncoderSetBindGroup(
          compute_pass, 0, (j % 2 == 0) ? bind_group_a : bind_group_b, 0, NULL);

      compute_pass_dispatch(compute_pass, KAWASE_WORKGROUP, pass->width,
                            pass->height);

      /* === PASS END === */
      wgpuComputePassEncoderEnd(compute_pass);
      wgpuComputePassEncoderRelease(compute_pass);

      if (j < offset_count)
        uniform.offset = offset[j];
    }

    // copy dest to src if count pass is odd (since result end up in dest)
    if (pass_count % 2 == 1) {
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
  }

  return KawaseStatus_Success;
}
