#ifndef _POST_FX_CORE_H_
#define _POST_FX_CORE_H_

#include "backend/compute/core.h"
#include "backend/compute/kawase.h"
#include "backend/profiler.h"
#include "backend/shaders/core.h"
#include "backend/shaders/include/bloom.h"
#include "backend/shaders/include/composite.h"
#include "runtime/pipeline/render.h"
#include "runtime/texture/core.h"
#include <stdint.h>
#include <webgpu/webgpu.h>

typedef struct PostFx PostFx;
typedef struct PostFxEffect PostFxEffect;
typedef union PostFxEffectUniform PostFxEffectUniform;

typedef enum {
  PostFxStatus_Success,
  PostFxStatus_MaxCapacity,
  PostFxStatus_AlreadyCreated,
  PostFxStatus_Uncreated,
  PostFxStatus_MissingNecessaryResource,
  PostFxStatus_UnknownType,
  PostFxStatus_UnvalidType,
  PostFxStatus_SameAttribute,
  PostFxStatus_UndefError,
} PostFxStatus;

#define POST_FX_TYPE_COUNT 3
typedef enum {
  PostFxType_Blit = 1 << 0,
  PostFxType_Bloom = 1 << 1,
  PostFxType_Composite = 1 << 2,
} PostFxType;

typedef enum {
  PostFxViewIndex_Scene,
  PostFxViewIndex_Bloom,
} PostFxViewIndex;

/**
   Overall flow:

     Create phase            Optimized Cache              Draw
      (Storage)              (DOD friendly)          (linearly read)

   .-------------.
   | Bloom       |
   | + texture   |
   | + view      |----.   .- array --------.
   | + uniform   |    |   |  Bind group    |       .----------------.
   '-------------'    |   |  View          |      |                  ▼
   .-------------.    '-->|  Pipeline      |         set pipeline
   | Composite   |        |----------------| ---->   set bind group
   | + texture   |        | Bind group     |         pass draw
   | + view      |------->| View           |      ▲                 |
   | + uniform   |        | Pipeline       |      '-----------------'
   '-------------'        |----------------|
   .-------------.        | Bind group     |
   | Blit        |   .--->| View           |
   | + texture   |---'    | Pipeline       |
   | + view      |        '----------------'
   '-------------'

   Post effects follow a certain order and are co-dependent, as instance, there
   would be no interest ot have first call the composite effect and then de
   bloom effect.

 */

typedef PostFxStatus (*post_fx_constructor)(PostFx *);
typedef PostFxStatus (*post_fx_destructor)(PostFx *);
typedef PostFxStatus (*post_fx_bindgroup_update)(PostFx *);
typedef PostFxStatus (*post_fx_uniform_update)(PostFx *,
                                               const PostFxEffectUniform);
typedef void (*post_fx_draw_callback)(PostFx *, WGPUCommandEncoder);

static const int POST_FX_BUFFER_CAPACITY = 6;
static const int POST_FX_VIEW_CAPACITY = 6;
static const int POST_FX_COMPUTE_CAPACITY = 3;

static const int POST_FX_BLOOM_KAWASE = 0;

union PostFxEffectUniform {
  CompositeUniform composite;
  BloomUniform bloom;
};

struct PostFxEffect {

  // cached bindgroup created with the effect parameter (view, unfiroms...)
  WGPUBindGroup bindgroup;
  RenderPipeline *const *pipeline;

  // effect view are either created from texture above of directly shared from
  // an external source
  WGPUTextureView views[POST_FX_VIEW_CAPACITY];
  WGPUBuffer buffers[POST_FX_BUFFER_CAPACITY];
  // compute passes for various effects (used to blur the bloom)
  ComputePass compute_passes[POST_FX_COMPUTE_CAPACITY];
  // optional texture if we want the post fx to use a independent texture
  WGPUTexture texture;

  // Mutators:
  post_fx_constructor constructor;
  post_fx_destructor destructor;
  post_fx_draw_callback draw_callback;
  post_fx_uniform_update uniform_update_callback;
  post_fx_bindgroup_update bindgroup_update_callback;

  PostFxEffectUniform uniform;
};

struct PostFx {

  PostFxType state;
  WGPUSampler sampler; // common sampler used in each effect
  PostFxEffect effects[POST_FX_TYPE_COUNT];
  WGPUTextureView scene_view; // view from which the effect will be applied on
  TextureResolution width, height;
  Profiler *profiler;

  struct {
    post_fx_draw_callback entries[POST_FX_TYPE_COUNT];
    uint8_t count;
  } callbacks;
};

typedef struct {
  Profiler *profiler;
  WGPUTextureView scene_view;
  const TextureResolution width, height;
} PostFxDescriptor;

EXTERN_C_BEGIN

PostFxStatus post_fx_init(PostFx *, const PostFxDescriptor *);
PostFxStatus post_fx_destroy(PostFx *);

// Creators
PostFxStatus post_fx_blit_create(PostFx *);
PostFxStatus post_fx_bloom_create(PostFx *);
PostFxStatus post_fx_composite_create(PostFx *);

// Destructors
PostFxStatus post_fx_blit_destroy(PostFx *);
PostFxStatus post_fx_bloom_destroy(PostFx *);
PostFxStatus post_fx_composite_destroy(PostFx *);
PostFxStatus post_fx_effect_destroy(PostFxEffect *);

PostFxStatus post_fx_add_callback(PostFx *, post_fx_draw_callback);
PostFxStatus post_fx_remove_callback(PostFx *, post_fx_draw_callback);

// Mutators
PostFxStatus post_fx_blit_update_bindgroup(PostFx *);
PostFxStatus post_fx_bloom_update_bindgroup(PostFx *);
PostFxStatus post_fx_composite_update_bindgroup(PostFx *);

PostFxStatus post_fx_bloom_update_uniform(PostFx *, const PostFxEffectUniform);
PostFxStatus post_fx_composite_update_uniform(PostFx *,
                                              const PostFxEffectUniform);

PostFxStatus post_fx_update_scene_view(PostFx *, const WGPUTextureView);
PostFxStatus post_fx_update_effect_view(PostFx *, const PostFxType,
                                        const PostFxViewIndex,
                                        const WGPUTextureView);

PostFxStatus post_fx_update_effect_uniform(PostFx *, const PostFxType,
                                           const PostFxEffectUniform);

PostFxStatus post_fx_bloom_update_texture_resolution(PostFx *, const int,
                                                     const int);

// Accessor
static inline PostFxEffect *post_fx_effect(PostFx *fx, const PostFxType type) {
  return &fx->effects[__builtin_ctz(type)];
}

static inline bool post_fx_effect_enabled(PostFx *fx, const PostFxType type) {
  return (fx->state & type);
}

static inline void post_fx_draw(PostFx *fx,
                                WGPUCommandEncoder command_encoder) {
  // loop through registerd effects
  for (uint8_t i = 0; i < fx->callbacks.count; i++)
    fx->callbacks.entries[i](fx, command_encoder);
}

PostFxStatus post_fx_toggle_effect(PostFx *, const PostFxType);

EXTERN_C_END

#endif
