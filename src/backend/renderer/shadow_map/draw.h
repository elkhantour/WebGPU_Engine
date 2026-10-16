#ifndef _SHADOW_MAP_DRAW_H_
#define _SHADOW_MAP_DRAW_H_

#include <stddef.h>
#include <stdint.h>
#include <webgpu/webgpu.h>

// #include "runtime/light/light.h"
#include "backend/profiler.h"
#include "backend/renderer/render_pass/core.h"
#include "backend/resource_manager.h"
#include "backend/ubo.h"
#include "runtime/light/core.h"
#include "runtime/light/list.h"
#include "runtime/mesh/core.h"

#include "runtime/mesh/mesh.h"
#include "runtime/pipeline/render.h"
#include "runtime/scene/debug/core.h"
#include "runtime/scene/debug/debug.h"
#include "utils/defines.h"
#include "utils/projection.h"

#include "backend/renderer/render_pass/core.h"
#include "backend/renderer/render_pass/draw.h"
#include "backend/std_pipeline/core.h"
#include "core.h"
#include "runtime/light/core.h"
#include "runtime/light/list.h"
#include "runtime/mesh/core.h"
#include "runtime/pipeline/render.h"
#include "runtime/scene/debug/view.h"
#include "webgpu/webgpu.h"

typedef struct {
  const MeshRefList *mesh_list;
  LightList *lights;
  Profiler *profiler;
} ShadowMapDrawAllDescriptor;

typedef struct {
  const WGPUCommandEncoder command_encoder;
  PointLight *light;
  const size_t texture_layer;
  RenderPass *pass;
  Profiler *profiler;
} ShadowMapDrawPointLightDescriptor;

typedef struct {
  SceneDebug *scene_debug;
  const uint16_t max_views;
} ShadowMapDebug;

typedef struct {
  const WGPUCommandEncoder command_encoder;
  SunLight *light;
  const size_t texture_layer;
  RenderPass *pass;
  Profiler *profiler;
} ShadowMapDrawSunLightDescriptor;

typedef struct {
  const WGPUCommandEncoder command_encoder;
  SpotLight *light;
  const size_t texture_layer;
  RenderPass *pass;
  Profiler *profiler;
} ShadowMapDrawSpotLightDescriptor;

typedef struct {
  const WGPUCommandEncoder command_encoder;
  Projection *views;
  const size_t texture_layer;
  const ubo_id_t ubo_offset;
  RenderPass *pass;
  Profiler *profiler;
} ShadowMapDrawDirLightDescriptor;

typedef struct {
  RenderPass *pass;
  const uint32_t texture_layer;
  const ubo_id_t ubo_offset;
  WGPUCommandEncoder command_encoder;
  Profiler *profiler;
} ShadowMapDrawDescriptor;

EXTERN_C_BEGIN

static inline void
renderer_draw_shadow_map_spot_light(const ShadowMapDrawSpotLightDescriptor *,
                                    const ShadowMapDebug *);
static inline void
renderer_draw_shadow_map_sun_light(const ShadowMapDrawSunLightDescriptor *,
                                   const ShadowMapDebug *);
static inline void
renderer_draw_shadow_map_dir_light(const ShadowMapDrawDirLightDescriptor *,
                                   const ShadowMapDebug *);

static inline void
renderer_draw_shadow_map_point_light(const ShadowMapDrawPointLightDescriptor *,
                                     const ShadowMapDebug *);

static inline void
renderer_draw_shadow_map_all(const ShadowMapDrawAllDescriptor *,
                             const ShadowMapDebug *);

static inline void renderer_draw_shadow_map(const ShadowMapDrawDescriptor *,
                                            const ShadowMapDebug *);

static int debug_view_count = 0;

/**
   The building shadow phase is segmented in numerous steps:

   1. BINDING:
   For each scene point and spot lights, we bind the relative views to
   the mesh shadow shader view matrix uniform

   2. DRAWING:
   Once the view matrix is bound, the mesh is ready to drawn with its shadow
  shader. For each light views, each mesh will be renderder (under a certain
  point of view)

   3. STORING:
  The rendered mesh will be stored in a dedicated Texture that will be held in a
  arrayed texture in the Light list

  4. TRANSFERT
  Once the rendered textures are stored, they will be uploaded in the mesh
  "default shader" as a texture and sampler to be read and reused for calculate
  the shadow

 */
void renderer_draw_shadow_map(const ShadowMapDrawDescriptor *desc,
                              const ShadowMapDebug *debug) {

  /*  Create a new "nested" texture view for each layer that points back to the
     texture。Both "global Texture view" and "indexed Texture view" point toward
                               the same texture

                           .----------------------.
                           |      WGPUTexture     |
                           '----------------------'
                   .-------------------'-------------------.
                   |                                       |
   .----------------------------------.   .-------------------------------.
   |    WGPUTextureView<all layers>   |   |    WGPUTextureView<layer N>   |
   '----------------------------------'   '-------------------------------'
     Final Texture with all layers         Target a specific texture layer

                   |                                      |
                   |                                      |
                   |		 	 .-------------------------------.
                   |			 |          1.Renderpass         |
                   |			 '-------------------------------'
                   |                         Render to specific layer
                   |
                   |
    .-------------------------------.
    |  2. Shader<texture 2D array>  |
    '-------------------------------'
        Bind the Array to shader

        In our case we create 2 layer views: one for color, and another for
      depth. The color one will mostly be used for debugging purpose, whereas
           the depth one will be used for comparison in texture shader

   */

  profiler_latency_start(desc->profiler, ProfilerLatencyType_ShadowPass);
  {
    // TODO: cache those views in the renderpass views
    // create per layer texture views (depth + color)
    WGPUTextureViewDescriptor temp_layer_texture_descriptor_depth = {
        .label = "Shadow per layer texture view - Depth",
        .format = SHADOW_DEPTH_FORMAT,
        .dimension = WGPUTextureViewDimension_2D,
        .baseArrayLayer = desc->texture_layer,
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .baseMipLevel = 0,
    };

    WGPUTextureViewDescriptor temp_layer_texture_descriptor_color = {
        .label = "Shadow per layer texture view - Color",
        .format = SHADOW_COLOR_FORMAT,
        .dimension = WGPUTextureViewDimension_2D,
        .baseArrayLayer = desc->texture_layer,
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .baseMipLevel = 0,
    };

    WGPUTextureView temp_layer_texture_view_depth = rem_new_view(
        desc->pass->depth.texture, &temp_layer_texture_descriptor_depth);

    WGPUTextureView temp_layer_texture_view_color = rem_new_view(
        desc->pass->color.texture, &temp_layer_texture_descriptor_color);

    WGPUTextureView cached_view_color = desc->pass->color.attachment.view;
    WGPUTextureView cached_view_depth = desc->pass->depth.attachment.view;

    // sometimes pass encoder may be set if we batch update all lights
    if (desc->command_encoder == NULL)
      render_pass_im_begin(desc->pass);
    {

      LightShadowData light_data = {.view_offset = desc->ubo_offset};
      render_pass_update_preprocessor_data(desc->pass, 0, &light_data);

      RenderPassDrawOptions layer_views = {
          .color = temp_layer_texture_view_color,
          .depth = temp_layer_texture_view_depth,
      };

      render_pass_im_set_views(desc->pass, &layer_views);

      render_pass_im_draw(desc->pass);

      // set back original view
      RenderPassDrawOptions source_views = {
          .color = cached_view_color,
          .depth = cached_view_depth,
      };
      render_pass_im_set_views(desc->pass, &source_views);
    }
    if (desc->command_encoder == NULL)
      render_pass_im_end(desc->pass);

    rem_destroy_view(&temp_layer_texture_view_depth);

    if (debug && debug->scene_debug && debug_view_count++ < debug->max_views)
      scene_debug_view_create(debug->scene_debug,
                              temp_layer_texture_view_color);
    else
      rem_destroy_view(&temp_layer_texture_view_color);
  }
  profiler_latency_end(desc->profiler, ProfilerLatencyType_ShadowPass);
}

/**
   ▗▄▄▄ ▗▄▄▖  ▗▄▖ ▗▖ ▗▖     ▗▄▖ ▗▖   ▗▖
   ▐▌  █▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌    ▐▌ ▐▌▐▌   ▐▌
   ▐▌  █▐▛▀▚▖▐▛▀▜▌▐▌ ▐▌    ▐▛▀▜▌▐▌   ▐▌
   ▐▙▄▄▀▐▌ ▐▌▐▌ ▐▌▐▙█▟▌    ▐▌ ▐▌▐▙▄▄▖▐▙▄▄▖

    Shadow map array Generation:
    - bind light view projection
    - render each mesh under lights POV
    - render to the shadow array
 */

void renderer_draw_shadow_map_all(const ShadowMapDrawAllDescriptor *desc,
                                  const ShadowMapDebug *debug) {

  // logger_add(LoggerFlag_Process, "Computing all shadow maps...");

  /*
     When using "global" shadow encoder and passing it through all the passes
     create an async conflict issue: the point light view probably takes longer
     to be calculated and gets overriden by the the sun point of view.

     A solution to this is to add an offset or use a command encoder per light

     The overall flow would be:

     1. set the shadow pipeline minBindSize to:
     .------------------------------------------------------------------------.
     |                      Mp * Nvp + Ms + Mn * sizeof(mat4)                 |
     |	               ex:  16 *  6  + 16 + 16 *    64                        |
     |                                                                        |
     |  - Mp: max point light                                                 |
     |  - Nvp: number of point light view (6)                                 |
     |  - Ms: max spot light                                                  |
     |  - Mn: max sun light                                                   |
     '------------------------------------------------------------------------'

     2. For each light: push the view to the ViewArray at correct index (cache?)
     .------------------------------------------------------------------------.
     |  Offset 0 × sizeof(mat4)       → point light view matrix face 0        |
     |  Offset 1 × sizeof(mat4)       → point light view matrix face 1        |
     |  ...                                                                   |
     |  Offset 5 × sizeof(mat4)       → point light view matrix face 5        |
     |  Offset 6 × sizeof(mat4)       → sun light view matrix                 |
     |  ...                                                                   |
     '------------------------------------------------------------------------'

      3. Write to each mesh shadow view uniform the views with the
      index * sizeof(mat4) as offset. (shader_update_uniform)

      4. Draw the mesh with the same offset (wgpuRenderPassEncoderSetBindGroup)

   */

  const MeshRefList *target_mesh_list = desc->mesh_list;

  const size_t point_count = desc->lights->point.shadow.count;
  const size_t spot_count = desc->lights->spot.shadow.count;
  const size_t sun_count = desc->lights->sun.shadow.count;

  debug_view_count = 0;

  /*
    ==== Point Lights ====
    */
  WGPUCommandEncoder point_encoder =
      render_pass_im_begin(&desc->lights->point.shadow.pass);
  {
    for (size_t p = 0; p < point_count; p++) {
      ShadowMapDrawPointLightDescriptor point_draw_desc = {
          .texture_layer = p,
          .light = desc->lights->point.shadow.entries[p],
          .pass = &desc->lights->point.shadow.pass,
          .command_encoder = point_encoder,
          .profiler = desc->profiler,
      };
      renderer_draw_shadow_map_point_light(&point_draw_desc, debug);
    }
  }
  render_pass_im_end(&desc->lights->point.shadow.pass);

  WGPUCommandEncoder dir_encoder =
      render_pass_im_begin(&desc->lights->spot.shadow.pass);
  {
    /*
      ==== Spot Lights ====
    */
    for (size_t p = 0; p < spot_count; p++) {
      ShadowMapDrawSpotLightDescriptor spot_draw_desc = {
          .texture_layer = p,
          .light = desc->lights->spot.shadow.entries[p],
          .command_encoder = dir_encoder,
          .pass = &desc->lights->spot.shadow.pass,
          .profiler = desc->profiler,
      };
      renderer_draw_shadow_map_spot_light(&spot_draw_desc, debug);
    }

    /*
      ==== Sun Lights ====
    */
    for (size_t p = 0; p < sun_count; p++) {

      ShadowMapDrawSunLightDescriptor sun_draw_desc = {
          // TODO: currently use spot light color_map, maybe make a linked
          // pointer
          // to the same map but include it in the sun light list struct
          // itself.
          .texture_layer = spot_count + p,
          .command_encoder = dir_encoder,
          .pass = &desc->lights->spot.shadow.pass,
          .light = desc->lights->sun.shadow.entries[p],
          .profiler = desc->profiler,
      };
      renderer_draw_shadow_map_sun_light(&sun_draw_desc, debug);
    }
  }
  render_pass_im_end(&desc->lights->spot.shadow.pass);
}

/**
   ▗▄▄▄ ▗▄▄▖  ▗▄▖ ▗▖ ▗▖     ▗▄▄▖ ▗▄▖ ▗▖    ▗▄▖
   ▐▌  █▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌    ▐▌   ▐▌ ▐▌▐▌   ▐▌ ▐▌
   ▐▌  █▐▛▀▚▖▐▛▀▜▌▐▌ ▐▌     ▝▀▚▖▐▌ ▐▌▐▌   ▐▌ ▐▌
   ▐▙▄▄▀▐▌ ▐▌▐▌ ▐▌▐▙█▟▌    ▗▄▄▞▘▝▚▄▞▘▐▙▄▄▖▝▚▄▞▘

 */

void renderer_draw_shadow_map_point_light(
    const ShadowMapDrawPointLightDescriptor *desc,
    const ShadowMapDebug *debug) {

  // render scene and store depth map for each view
  for (size_t v = 0; v < desc->light->views.count; v++) {

    // Render scene (create shadow render pass to texture layer)
    size_t layer = desc->texture_layer * desc->light->views.count + v;

    ShadowMapDrawDescriptor draw_desc = {
        .pass = desc->pass,
        .texture_layer = layer,
        .command_encoder = desc->command_encoder,
        .ubo_offset = desc->light->ubo_projection[v].id,
        .profiler = desc->profiler,
    };
    renderer_draw_shadow_map(&draw_desc, debug);
  }
}

/**
   Function to handle both spot and sun light drawing
 */
void renderer_draw_shadow_map_dir_light(
    const ShadowMapDrawDirLightDescriptor *desc, const ShadowMapDebug *debug) {

  // Render scene (create shadow render pass to texture layer)
  for (size_t v = 0; v < desc->views->count; v++) {
    ShadowMapDrawDescriptor draw_desc = {
        .pass = desc->pass,
        .texture_layer = desc->texture_layer,
        .ubo_offset = desc->ubo_offset,
        .command_encoder = desc->command_encoder,
        .profiler = desc->profiler,
    };
    renderer_draw_shadow_map(&draw_desc, debug);
  }
}

void renderer_draw_shadow_map_sun_light(
    const ShadowMapDrawSunLightDescriptor *desc, const ShadowMapDebug *debug) {

  ShadowMapDrawDirLightDescriptor draw_desc = {
      .pass = desc->pass,
      .command_encoder = desc->command_encoder,
      .texture_layer = desc->texture_layer,
      .ubo_offset = desc->light->ubo_projection.id,
      .views = &desc->light->views,
      .profiler = desc->profiler,
  };
  renderer_draw_shadow_map_dir_light(&draw_desc, debug);
}

void renderer_draw_shadow_map_spot_light(
    const ShadowMapDrawSpotLightDescriptor *desc, const ShadowMapDebug *debug) {

  ShadowMapDrawDirLightDescriptor draw_desc = {
      .pass = desc->pass,
      .command_encoder = desc->command_encoder,
      .texture_layer = desc->texture_layer,
      .ubo_offset = desc->light->ubo_projection.id,
      .views = &desc->light->views,
      .profiler = desc->profiler,
  };
  renderer_draw_shadow_map_dir_light(&draw_desc, debug);
}

EXTERN_C_END
#endif
