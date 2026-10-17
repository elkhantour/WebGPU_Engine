#include "mipmap.h"
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

static inline void compute_pass_mipmap_dispatch(WGPUComputePassEncoder,
                                                const TextureResolution,
                                                const TextureResolution,
                                                const mip_t);

static inline void compute_pass_mipmap_cache_resources(ComputePass *);

MipmapStatus compute_pass_mipmap_create(ComputePass *pass,
                                        const ComputePassDescriptor *desc) {

  uint32_t layer_count = wgpuTextureGetDepthOrArrayLayers(desc->source_texture);

  // For each layer we need to cache two views. So need to check
  // if capacity is right.
  if (layer_count > 2 * COMPUTE_PASS_VIEW_CAPACITY) {
    logger_add(
        LoggerFlag_Error,
        "Layer count for compute pass '%s' is too high (%u), layer count "
        "should be smaller than %u.",
        desc->label, layer_count, COMPUTE_PASS_VIEW_CAPACITY / 2);
    return MipmapStatus_OutOfBound;
  }

  compute_pass_create(pass, desc);

  compute_pass_mipmap_update_source_texture(pass, pass->source_texture);

  return MipmapStatus_Success;
}

MipmapStatus
compute_pass_mipmap_update_source_texture(ComputePass *pass,
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
  compute_pass_mipmap_cache_resources(pass);

  return MipmapStatus_Success;
}

void compute_pass_mipmap_cache_resources(ComputePass *pass) {

  const mip_t mip_count = wgpuTextureGetMipLevelCount(pass->source_texture);

  const WGPUComputePipeline mipmap_pipeline =
      std_compute_pipeline(ComputePipelineType_Mipmap)->handle;

  for (uint32_t i = 0; i < pass->layer_count; i++) {
    for (mip_t j = 1; j < mip_count; j++) {

      /* === BIND TARGET VIEWS === */
      WGPUTextureViewDescriptor src_view_desc = {
          .format = wgpuTextureGetFormat(pass->source_texture),
          .dimension = WGPUTextureViewDimension_2D,
          .baseMipLevel = j - 1,
          .mipLevelCount = 1,
          .baseArrayLayer = i,
          .arrayLayerCount = 1,
      };

      WGPUTextureViewDescriptor dst_view_desc = src_view_desc;
      dst_view_desc.baseMipLevel = j;

      const uint32_t view_index = i * (mip_count - 1) * 2 + (j - 1) * 2;

      pass->views[view_index] =
          rem_new_view(pass->source_texture, &src_view_desc);

      pass->views[view_index + 1] =
          rem_new_view(pass->source_texture, &dst_view_desc);

      WGPUBindGroupEntry entries[] = {
          {.binding = 0, .textureView = pass->views[view_index]},
          {.binding = 1, .sampler = pass->sampler},
          {.binding = 2, .textureView = pass->views[view_index + 1]},
      };

      const int entries_count = sizeof(entries) / sizeof(entries[0]);

      const uint32_t group_index = i * (mip_count - 1) + (j - 1);

      pass->bindgroups[group_index] = wgpuDeviceCreateBindGroup(
          context_device(), &(WGPUBindGroupDescriptor){
                                .layout = wgpuComputePipelineGetBindGroupLayout(
                                    mipmap_pipeline, 0),
                                .entryCount = entries_count,
                                .entries = entries,
                            });
    }
  }
}

void compute_pass_mipmap_draw(ComputePass *pass) {

  WGPUCommandEncoder command_encoder =
      wgpuDeviceCreateCommandEncoder(context_device(), NULL);

  const mip_t mip_count = wgpuTextureGetMipLevelCount(pass->source_texture);

  const WGPUComputePipeline mipmap_pipeline =
      std_compute_pipeline(ComputePipelineType_Mipmap)->handle;

  for (uint32_t i = 0; i < pass->layer_count; i++) {

    for (mip_t j = 1; j < mip_count; j++) {

      const uint32_t group_index = i * (mip_count - 1) + (j - 1);
      WGPUBindGroup bind_group = pass->bindgroups[group_index];

      /* === PASS  BEGIN === */
      WGPUComputePassEncoder compute_pass = wgpuCommandEncoderBeginComputePass(
          command_encoder,
          &(WGPUComputePassDescriptor){.label = "Mipmap Compute Pass"});

      wgpuComputePassEncoderSetPipeline(compute_pass, mipmap_pipeline);

      wgpuComputePassEncoderSetBindGroup(compute_pass, 0, bind_group, 0, NULL);

      compute_pass_mipmap_dispatch(
          compute_pass, wgpuTextureGetWidth(pass->source_texture),
          wgpuTextureGetHeight(pass->source_texture), j);

      /* === PASS END === */
      wgpuComputePassEncoderEnd(compute_pass);
      wgpuComputePassEncoderRelease(compute_pass);
    }
  }
  WGPUCommandBuffer compute_buffer =
      wgpuCommandEncoderFinish(command_encoder, NULL);

  wgpuQueueSubmit(context_queue(), 1, &compute_buffer);

  wgpuCommandEncoderRelease(command_encoder);
  wgpuCommandBufferRelease(compute_buffer);
}

void compute_pass_mipmap_dispatch(WGPUComputePassEncoder pass,
                                  const TextureResolution width,
                                  const TextureResolution height,
                                  const mip_t mip) {

  mip_t mip_width = glm_max(1, width >> mip);
  mip_t mip_height = glm_max(1, height >> mip);

  uint32_t dispatch_x = (mip_width + MIPMAP_WORKGROUP - 1) / MIPMAP_WORKGROUP;
  uint32_t dispatch_y = (mip_height + MIPMAP_WORKGROUP - 1) / MIPMAP_WORKGROUP;

  wgpuComputePassEncoderDispatchWorkgroups(pass, dispatch_x, dispatch_y, 1);
}
