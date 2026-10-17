#include "core.h"
#include "backend/compute/core.h"
#include "backend/compute/kawase.h"
#include "backend/context.h"
#include "backend/logger.h"
#include "backend/postfx/draw.h"
#include "backend/resource_manager.h"
#include "backend/shaders/core.h"
#include "backend/shaders/include/bloom.h"
#include "backend/shaders/include/composite.h"
#include "backend/std_texture/core.h"
#include "runtime/pipeline/render.h"
#include "runtime/texture/core.h"
#include "utils/stli.h"
#include "webgpu/webgpu.h"
#include <stdint.h>
#include <stdio.h>

// Utils
static inline int post_fx_state_disable_effect(PostFx *, const PostFxType);
static inline PostFxStatus post_fx_validate_create(PostFx *, const PostFxType);
static inline WGPUTexture post_fx_bloom_create_texture(const int, const int);

static const struct {
  post_fx_constructor constructor;
  post_fx_destructor destructor;
  post_fx_bindgroup_update bindgroup_update_callback;
  post_fx_draw_callback draw_callback;
  post_fx_uniform_update uniform_update_callback;
} post_fx_effect_config[] = {
    [PostFxType_Blit] =
        {
            post_fx_blit_create,
            post_fx_blit_destroy,
            post_fx_blit_update_bindgroup,
            post_fx_blit_draw,
            NULL,
        },
    [PostFxType_Bloom] =
        {
            post_fx_bloom_create,
            post_fx_bloom_destroy,
            post_fx_bloom_update_bindgroup,
            post_fx_bloom_draw,
            post_fx_bloom_update_uniform,
        },
    [PostFxType_Composite] =
        {
            post_fx_composite_create,
            post_fx_composite_destroy,
            post_fx_composite_update_bindgroup,
            post_fx_composite_draw,
            post_fx_composite_update_uniform,
        },
};

PostFxStatus post_fx_init(PostFx *fx, const PostFxDescriptor *desc) {

  fx->sampler = rem_new_sampler(&(WGPUSamplerDescriptor){
      .label = "PostFX Common Sampler",
      .addressModeU = WGPUAddressMode_ClampToEdge,
      .addressModeV = WGPUAddressMode_ClampToEdge,
      .addressModeW = WGPUAddressMode_ClampToEdge,
      .magFilter = WGPUFilterMode_Linear,
      .minFilter = WGPUFilterMode_Linear,
      .mipmapFilter = WGPUMipmapFilterMode_Linear,
  });

  fx->width = desc->width;
  fx->height = desc->height;
  fx->scene_view = desc->scene_view;
  fx->profiler = desc->profiler;
  fx->callbacks.count = 0;

  // define effect callbacks
  for (size_t i = 0; i < POST_FX_TYPE_COUNT; i++) {
    PostFxEffect *effect = post_fx_effect(fx, 1 << i);

    // clang-format off
    effect->constructor = post_fx_effect_config[1 << i].constructor;
    effect->destructor = post_fx_effect_config[1 << i].destructor;
    effect->bindgroup_update_callback = post_fx_effect_config[1 << i].bindgroup_update_callback;
    effect->draw_callback = post_fx_effect_config[1 << i].draw_callback;
    effect->uniform_update_callback = post_fx_effect_config[1 << i].uniform_update_callback;
    // clang-format on
  }

  return PostFxStatus_Success;
}

PostFxStatus post_fx_destroy(PostFx *fx) {

  if (fx->sampler)
    rem_destroy_sampler(&fx->sampler);

  for (size_t i = 0; i < POST_FX_TYPE_COUNT; i++)
    post_fx_effect_destroy(post_fx_effect(fx, 1 << i));

  return PostFxStatus_Success;
}

PostFxStatus post_fx_validate_create(PostFx *fx, const PostFxType type) {

  static const char *label[] = {
      [PostFxType_Blit] = "Blit",
      [PostFxType_Bloom] = "Bloom",
      [PostFxType_Composite] = "Composite",
  };

  if (fx->state & type) {
    logger_add(LoggerFlag_Error,
               "Cannot initialize post fx %s, already created. Make sure to "
               "destroy the effect beffore creating it.",
               label[type]);
    return PostFxStatus_AlreadyCreated;
  }

  // update state
  fx->state |= type;

  return PostFxStatus_Success;
}

/**
   Update the global post fx view and rebuild all active effect with the new
   scene view
 */
PostFxStatus post_fx_update_scene_view(PostFx *fx, const WGPUTextureView view) {
  fx->scene_view = view;

  for (uint8_t i = 0; i < POST_FX_TYPE_COUNT; i++) {
    PostFxEffect *effect = post_fx_effect(fx, 1 << i);

    effect->views[PostFxViewIndex_Scene] = fx->scene_view;

    if (post_fx_effect_enabled(fx, 1 << i) && effect->bindgroup_update_callback)
      effect->bindgroup_update_callback(fx);
  }

  return PostFxStatus_Success;
}

/**
   Generic function to replace the view of an effect at a certain index and
   automatically rebuild the effect after ward.
 */
PostFxStatus post_fx_update_effect_view(PostFx *fx, const PostFxType type,
                                        const PostFxViewIndex index,
                                        const WGPUTextureView view) {

  if (__builtin_ctz(type) > POST_FX_TYPE_COUNT) {
    logger_add(LoggerFlag_Error,
               "Attempting to update view of an unknown post fx type (%d)",
               type);
    return PostFxStatus_UnknownType;
  } else if (index >= POST_FX_BUFFER_CAPACITY) {
    logger_add(LoggerFlag_Error,
               "Attempting to update a post fx out of bound view (%d), "
               "maximum views allowed: %d",
               index, POST_FX_BUFFER_CAPACITY);
    return PostFxStatus_MaxCapacity;
  }

  PostFxEffect *effect = post_fx_effect(fx, type);
  effect->views[index] = view;

  if (effect->bindgroup_update_callback)
    effect->bindgroup_update_callback(fx);

  return PostFxStatus_Success;
}

PostFxStatus post_fx_update_effect_uniform(PostFx *fx, const PostFxType type,
                                           const PostFxEffectUniform uniform) {

  if (type & PostFxType_Blit) {
    logger_add(LoggerFlag_Warning,
               "Attempting to set uniform for Blit post effect. Blit post "
               "effect doesn't have any uniform.");
    return PostFxStatus_UnvalidType;
  }

  PostFxEffect *effect = post_fx_effect(fx, type);

  if (!effect->uniform_update_callback) {
    logger_add(LoggerFlag_Warning,
               "Attempting to set uniform for an effect (%d), that doesn't "
               "have the necessary callback.",
               __builtin_ctz(type));
    return PostFxStatus_MissingNecessaryResource;
  }

  effect->uniform_update_callback(fx, uniform);

  return PostFxStatus_Success;
}

PostFxStatus post_fx_bloom_update_uniform(PostFx *fx,
                                          const PostFxEffectUniform uniform) {
  PostFxEffect *effect = post_fx_effect(fx, PostFxType_Bloom);
  effect->uniform.bloom = uniform.bloom;
  rem_write_buffer(effect->buffers[0], 0, &effect->uniform.bloom,
                   sizeof(BloomUniform), REMWriteFlag_None);
  return PostFxStatus_Success;
}

PostFxStatus
post_fx_composite_update_uniform(PostFx *fx,
                                 const PostFxEffectUniform uniform) {
  PostFxEffect *effect = post_fx_effect(fx, PostFxType_Composite);
  effect->uniform.composite = uniform.composite;
  rem_write_buffer(effect->buffers[0], 0, &effect->uniform.composite,
                   sizeof(CompositeUniform), REMWriteFlag_None);
  return PostFxStatus_Success;
}

/**
   Overral effect process:
   1. validate if not already created
   2. define core attributes: pipeline, view(s), uniforms
   3. create bindgroup
   4. update fx callbacks and count (i.e. register to draw loop)
 */
PostFxStatus post_fx_blit_create(PostFx *fx) {

  const PostFxType fx_type = PostFxType_Blit;
  const RenderPipelineType pipeline_type = RenderPipelineType_Blit;

  // === Validate ===
  {
    PostFxStatus validate_status = post_fx_validate_create(fx, fx_type);
    if (validate_status != PostFxStatus_Success)
      return validate_status;
  }

  PostFxEffect *effect = post_fx_effect(fx, fx_type);

  // === Define core attributes ===
  {
    effect->pipeline = std_render_pipeline(pipeline_type);
    effect->views[PostFxViewIndex_Scene] = fx->scene_view;
    effect->texture = NULL;
  }

  effect->bindgroup_update_callback(fx);
  post_fx_add_callback(fx, effect->draw_callback);

  return PostFxStatus_Success;
}

PostFxStatus post_fx_bloom_create(PostFx *fx) {

  const PostFxType fx_type = PostFxType_Bloom;
  const RenderPipelineType pipeline_type = RenderPipelineType_Bloom;
  const BloomUniform uniform = {
      .blur = 2,
      .downscale = 2,
      .knee = 0.450f,
      .threshold = 0.3f,
  };

  // === Validate ===
  {
    PostFxStatus validate_status = post_fx_validate_create(fx, fx_type);
    if (validate_status != PostFxStatus_Success)
      return validate_status;
  }

  PostFxEffect *effect = post_fx_effect(fx, fx_type);

  // === Define core attributes ===
  {
    effect->pipeline = std_render_pipeline(pipeline_type);
    effect->views[PostFxViewIndex_Scene] = fx->scene_view;
    effect->texture =
        post_fx_bloom_create_texture((int)(fx->width / uniform.downscale),
                                     (int)(fx->height / uniform.downscale));

    effect->views[PostFxViewIndex_Bloom] = rem_new_view(effect->texture, NULL);

    effect->uniform.bloom = uniform;
  }

  effect->bindgroup_update_callback(fx);
  post_fx_add_callback(fx, effect->draw_callback);

  // === Create Kawase Compute pass ===
  {
    compute_pass_kawase_create(&effect->compute_passes[KAWASE_BUFFER_UNIFORM],
                               &(ComputePassDescriptor){
                                   .label = "Bloom FX - Kawase Compute Pass",
                                   .source_texture = effect->texture,
                               });
  }

  // if composite is created, replace the view with the bloom view
  if (post_fx_effect_enabled(fx, PostFxType_Composite)) {
    PostFxEffect *composite = post_fx_effect(fx, PostFxType_Composite);
    composite->views[PostFxViewIndex_Bloom] =
        effect->views[PostFxViewIndex_Bloom];
    composite->bindgroup_update_callback(fx);
  }

  return PostFxStatus_Success;
}

PostFxStatus post_fx_composite_create(PostFx *fx) {

  const PostFxType fx_type = PostFxType_Composite;
  const RenderPipelineType pipeline_type = RenderPipelineType_Composite;
  const CompositeUniform uniform = {
      .bloom_intensity = 1.0f,
      .exposure = 1.140f,
      .gamma = 1.460f,
      .vignette_feather = 1.0f,
      .vignette_strength = 0.0f,
  };

  // === Validate ===
  {
    PostFxStatus validate_status = post_fx_validate_create(fx, fx_type);
    if (validate_status != PostFxStatus_Success)
      return validate_status;

    // requires Post Fx Bloom to be setup (TODO: will be replace by fallback
    // texture in future)
    if (post_fx_effect(fx, PostFxType_Bloom)->views[PostFxViewIndex_Bloom] ==
        NULL) {
      logger_add(LoggerFlag_Error,
                 "Composite effect require bloom post fx to be created");
      return PostFxStatus_MissingNecessaryResource;
    }
  }

  PostFxEffect *effect = post_fx_effect(fx, fx_type);

  // === Define core attributes ===
  {
    effect->pipeline = std_render_pipeline(pipeline_type);
    effect->views[PostFxViewIndex_Scene] = fx->scene_view;
    effect->views[PostFxViewIndex_Bloom] =
        post_fx_effect(fx, PostFxType_Bloom)->views[PostFxViewIndex_Bloom];
    effect->uniform.composite = uniform;
  }

  effect->bindgroup_update_callback(fx);
  post_fx_add_callback(fx, effect->draw_callback);

  return PostFxStatus_Success;
}

PostFxStatus post_fx_blit_destroy(PostFx *fx) {
  post_fx_effect_destroy(post_fx_effect(fx, PostFxType_Blit));
  post_fx_state_disable_effect(fx, PostFxType_Blit);

  PostFxEffect *effect = post_fx_effect(fx, PostFxType_Blit);
  post_fx_remove_callback(fx, effect->draw_callback);

  return PostFxStatus_Success;
}

PostFxStatus post_fx_bloom_destroy(PostFx *fx) {

  PostFxEffect *effect = post_fx_effect(fx, PostFxType_Bloom);

  post_fx_effect_destroy(effect);
  
  rem_destroy_view(&effect->views[PostFxViewIndex_Bloom]);
  
  post_fx_state_disable_effect(fx, PostFxType_Bloom);
  post_fx_remove_callback(fx, effect->draw_callback);

  // switch composite view to fallback texture
  if (post_fx_effect_enabled(fx, PostFxType_Composite)) {
    PostFxEffect *composite = post_fx_effect(fx, PostFxType_Composite);
    composite->views[PostFxViewIndex_Bloom] =
        std_texture_view(TextureViewType_FloatBlack);
    composite->bindgroup_update_callback(fx);
  }

  return PostFxStatus_Success;
}
PostFxStatus post_fx_composite_destroy(PostFx *fx) {
  post_fx_effect_destroy(post_fx_effect(fx, PostFxType_Composite));
  post_fx_state_disable_effect(fx, PostFxType_Composite);
  
  PostFxEffect *effect = post_fx_effect(fx, PostFxType_Composite);
  post_fx_remove_callback(fx, effect->draw_callback);

  return PostFxStatus_Success;
}

/**
   Common destroyer functions
   WARNING: We do not automatically destroy views, cause views might be shared accross
   other enities, so it would be dangerous to automatically batch release them.
 */
PostFxStatus post_fx_effect_destroy(PostFxEffect *effect) {

  rem_destroy_texture(&effect->texture);

  uint8_t i;
  for (i = 0; i < POST_FX_BUFFER_CAPACITY; i++)
    if (effect->buffers[i])
      rem_destroy_buffer(&effect->buffers[i]);

  for (i = 0; i < POST_FX_COMPUTE_CAPACITY; i++)
    compute_pass_destroy(&effect->compute_passes[i]);

  if (effect->bindgroup) {
    wgpuBindGroupRelease(effect->bindgroup);
    effect->bindgroup = NULL;
  }

  return PostFxStatus_Success;
}

PostFxStatus post_fx_blit_update_bindgroup(PostFx *fx) {

  PostFxEffect *effect = post_fx_effect(fx, PostFxType_Blit);

  // clean up
  if (effect->bindgroup) {
    wgpuBindGroupRelease(effect->bindgroup);
    effect->bindgroup = NULL;
  }

  const WGPURenderPipeline pipeline = (*effect->pipeline)->handle;
  const WGPUBindGroupLayout bind_group_layout =
      wgpuRenderPipelineGetBindGroupLayout(pipeline, 0);

  WGPUBindGroupEntry entries[2] = {
      {.binding = 0, .textureView = effect->views[PostFxViewIndex_Scene]},
      {.binding = 1, .sampler = fx->sampler},
  };
  WGPUBindGroupDescriptor bg_desc = {
      .layout = bind_group_layout,
      .entryCount = 2,
      .entries = entries,
  };

  effect->bindgroup = wgpuDeviceCreateBindGroup(context_device(), &bg_desc);
  return PostFxStatus_Success;
}

PostFxStatus post_fx_bloom_update_bindgroup(PostFx *fx) {

  PostFxEffect *effect = post_fx_effect(fx, PostFxType_Bloom);

  if (effect->bindgroup)
    wgpuBindGroupRelease(effect->bindgroup);

  const WGPURenderPipeline pipeline = (*effect->pipeline)->handle;
  const WGPUBindGroupLayout bind_group_layout =
      wgpuRenderPipelineGetBindGroupLayout(pipeline, 0);

  if (effect->buffers[0] == NULL)
    effect->buffers[0] = rem_new_buffer(&(WGPUBufferDescriptor){
        .label = "Bloom buffer",
        .size = sizeof(BloomUniform),
        .usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst,
    });

  rem_write_buffer(effect->buffers[0], 0, &effect->uniform.bloom,
                   sizeof(BloomUniform), REMWriteFlag_None);

  WGPUBindGroupEntry entries[3] = {
      {.binding = 0, .textureView = effect->views[PostFxViewIndex_Scene]},
      {.binding = 1, .sampler = fx->sampler},
      {
          .binding = 2,
          .buffer = effect->buffers[0],
          .size = sizeof(BloomUniform),
      },
  };

  WGPUBindGroupDescriptor bg_desc = {
      .layout = bind_group_layout,
      .entryCount = 3,
      .entries = entries,
  };

  effect->bindgroup = wgpuDeviceCreateBindGroup(context_device(), &bg_desc);
  return PostFxStatus_Success;
}

PostFxStatus post_fx_composite_update_bindgroup(PostFx *fx) {

  PostFxEffect *effect = post_fx_effect(fx, PostFxType_Composite);

  if (effect->bindgroup) {
    wgpuBindGroupRelease(effect->bindgroup);
    effect->bindgroup = NULL;
  }

  const WGPURenderPipeline pipeline = (*effect->pipeline)->handle;
  const WGPUBindGroupLayout bind_group_layout =
      wgpuRenderPipelineGetBindGroupLayout(pipeline, 0);

  if (effect->buffers[0] == NULL)
    effect->buffers[0] = rem_new_buffer(&(WGPUBufferDescriptor){
        .label = "Composite Buffer",
        .size = sizeof(CompositeUniform),
        .usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst,
    });

  rem_write_buffer(effect->buffers[0], 0, &effect->uniform.composite,
                   sizeof(CompositeUniform), REMWriteFlag_None);

  WGPUBindGroupEntry entries[4] = {
      {.binding = 0, .textureView = effect->views[PostFxViewIndex_Scene]},
      {.binding = 1, .textureView = effect->views[PostFxViewIndex_Bloom]},
      {.binding = 2, .sampler = fx->sampler},
      {
          .binding = 3,
          .buffer = effect->buffers[0],
          .size = sizeof(CompositeUniform),
      },
  };

  WGPUBindGroupDescriptor bg_desc = {
      .layout = bind_group_layout,
      .entryCount = 4,
      .entries = entries,
  };

  effect->bindgroup = wgpuDeviceCreateBindGroup(context_device(), &bg_desc);

  return PostFxStatus_Success;
}

PostFxStatus post_fx_add_callback(PostFx *fx, post_fx_draw_callback callback) {

  if (fx->callbacks.count == POST_FX_TYPE_COUNT) {
    logger_add(LoggerFlag_Error, "Post fx callback list reached max capacity.");
    return PostFxStatus_MaxCapacity;
  }

  fx->callbacks.entries[fx->callbacks.count++] = callback;

  return PostFxStatus_Success;
}

PostFxStatus post_fx_remove_callback(PostFx *fx,
                                     post_fx_draw_callback callback) {

  StaticListStatus remove = stli_remove(
      fx->callbacks.entries, (size_t *)&fx->callbacks.count,
      sizeof(post_fx_draw_callback), &callback, "Postfx callback list");

  return PostFxStatus_Success;
}

WGPUTexture post_fx_bloom_create_texture(const int width, const int height) {

  return rem_new_texture(&(WGPUTextureDescriptor){
      .label = "Bloom texture",
      .dimension = WGPUTextureDimension_2D,
      .format = TEXTURE_FORMAT_OFFSCREEN,
      .usage = WGPUTextureUsage_TextureBinding |
               WGPUTextureUsage_RenderAttachment |
               WGPUTextureUsage_StorageBinding | WGPUTextureUsage_CopyDst,
      .sampleCount = 1,
      .mipLevelCount = 1,
      .size =
          (WGPUExtent3D){
              .height = height,
              .width = width,
              .depthOrArrayLayers = 1,
          },
  });
}

/**
   Update the bloom texture resolution and update the view from the composite
   pass (if existing)
 */
PostFxStatus post_fx_bloom_update_texture_resolution(PostFx *fx,
                                                     const int width,
                                                     const int height) {

  PostFxEffect *effect = post_fx_effect(fx, PostFxType_Bloom);

  if (effect->texture) {
    if (width == wgpuTextureGetWidth(effect->texture) *
                     effect->uniform.bloom.downscale &&
        height == wgpuTextureGetHeight(effect->texture) *
                      effect->uniform.bloom.downscale)
      return PostFxStatus_SameAttribute;
    else
      rem_destroy_texture(&effect->texture);
  }

  effect->texture = post_fx_bloom_create_texture(
      (int)(width / effect->uniform.bloom.downscale),
      (int)(height / effect->uniform.bloom.downscale));

  // TODO: destroy previous view ??
  effect->views[PostFxViewIndex_Bloom] = rem_new_view(effect->texture, NULL);
  post_fx_bloom_update_bindgroup(fx);

  compute_pass_kawase_update_source_texture(
      &effect->compute_passes[POST_FX_BLOOM_KAWASE], effect->texture);

  // update composite view as well
  if (fx->state & PostFxType_Composite) {
    post_fx_effect(fx, PostFxType_Composite)->views[PostFxViewIndex_Bloom] =
        effect->views[PostFxViewIndex_Bloom];
    post_fx_composite_update_bindgroup(fx);
  }

  return PostFxStatus_Success;
}

int post_fx_state_disable_effect(PostFx *fx, const PostFxType type) {
  fx->state &= (~type);
  return fx->state;
}

PostFxStatus post_fx_toggle_effect(PostFx *fx, const PostFxType type) {
  PostFxEffect *effect = post_fx_effect(fx, type);

  if (post_fx_effect_enabled(fx, type)) { // => disable

    if ((type & PostFxType_Blit) || (type & PostFxType_Composite)) {
      // Blit effect is by default on every pass to enable independant render
      // size from the Framebuffer
      logger_add(LoggerFlag_Warning,
                 "Cannot disable 'Blit'or 'Composite' post effect pass.");
      return PostFxStatus_UnvalidType;
    }

    if (effect->destructor) {
      effect->destructor(fx);
    } else {
      logger_add(LoggerFlag_Warning, "No destructor for effect: %d.", type);
      return PostFxStatus_MissingNecessaryResource;
    }

    // if no bloom and no composite, switch back to basic blit effect
    if (!post_fx_effect_enabled(fx, PostFxType_Bloom) &&
        !post_fx_effect_enabled(fx, PostFxType_Composite))
      post_fx_effect(fx, PostFxType_Blit)->constructor(fx);

  } else { // => enable
    if (effect->constructor) {
      effect->constructor(fx);
    } else {
      logger_add(LoggerFlag_Warning, "No constructor for effect: %d.", type);
      return PostFxStatus_MissingNecessaryResource;
    }
  }

  return PostFxStatus_Success;
}
