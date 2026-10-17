#include "core.h"

#include <cglm/util.h>
#include <stdint.h>
#include <string.h>

#include "backend/logger.h"
#include "backend/postfx/core.h"
#include "backend/shaders/core.h"
#include "draw.h"
#include "runtime/mesh/core.h"
#include "runtime/mesh/draw.h"
#include "runtime/mesh/ref_list.h"
#include "runtime/pipeline/render.h"
#include "runtime/shader/core.h"
#include "texture.h"
#include "utils/dyli.h"
#include "utils/stli.h"
#include "webgpu/webgpu.h"

static inline void render_pass_draw_pass(RenderPass *, WGPUCommandEncoder);

static inline void render_pass_init_color(RenderPass *,
                                          const RenderPassCreateDescriptor *);

static inline void render_pass_init_depth(RenderPass *,
                                          const RenderPassCreateDescriptor *);

void render_pass_create(RenderPass *pass,
                        const RenderPassCreateDescriptor *desc) {

  // === assign core attributes ===
  pass->label = strdup(desc->label);
  pass->multisample = desc->multisample;
  pass->type = desc->type;

  // === assign draw callbacks ===
  pass->draw_callback = render_pass_im_draw;

  // === create render textures ===
  if (desc->color)
    render_pass_init_color(pass, desc);

  if (desc->depth)
    render_pass_init_depth(pass, desc);

  if (desc->draw_list)
    render_pass_draw_list_copy(desc->draw_list, &pass->layouts);
}

void render_pass_init_color(RenderPass *pass,
                            const RenderPassCreateDescriptor *desc) {

  // assign color attributes
  pass->color.texture = desc->color->texture;
  pass->color.attachment = desc->color->attachment;
  pass->color.views_count = 1;

  render_pass_texture_create_color(pass,
                                   &(RenderPassTextureDescriptor){
                                       .height = desc->height,
                                       .width = desc->width,
                                       .multisample = pass->multisample,
                                       .format = desc->color->format,
                                   },
                                   RenderPassTextureFlag_None);
}

void render_pass_init_depth(RenderPass *pass,
                            const RenderPassCreateDescriptor *desc) {

  pass->depth.texture = desc->depth->texture;
  pass->depth.attachment = desc->depth->attachment;
  pass->depth.views_count = 1;

  if (pass->depth.attachment.view == NULL)
    render_pass_texture_create_depth(pass,
                                     &(RenderPassTextureDescriptor){
                                         .height = desc->height,
                                         .width = desc->width,
                                         .multisample = desc->multisample,
                                         .format = desc->depth->format,
                                     },
                                     RenderPassTextureFlag_None);
}

void render_pass_list_create(RenderPassList *list) { list->count = 0; }

RenderPass *
render_pass_list_insert_pass(RenderPassList *list,
                             const RenderPassCreateDescriptor *desc) {

  if (list->count == RENDER_PASS_MAX_DRAW_LIST) {
    logger_add(LoggerFlag_Warning,
               "Render pass list reached maxed capacity (%d)",
               RENDER_PASS_MAX_DRAW_LIST);
    return NULL;
  }

  RenderPass *new_pass = &list->passes[list->count++];
  render_pass_create(new_pass, desc);
  render_pass_list_update_child_passes_callback(list);
  return new_pass;
}

/**
   Define automatically each last pass draw callback depending on the pass
   multisample.
 */
void render_pass_list_update_child_passes_callback(RenderPassList *list) {

  // reset previous pass callback to default
  for (size_t i = 0; i < list->count; i++)
    list->passes[i].draw_callback = render_pass_im_draw;

  // set last pass resolve
  {
    RenderPass *last_pass = &list->passes[list->count - 1];

    if (PipelineMultisampleCount_1x == last_pass->multisample)
      last_pass->draw_callback = render_pass_draw_callback_resolve_monosample;
    else if (PipelineMultisampleCount_4x == last_pass->multisample)
      last_pass->draw_callback = render_pass_draw_callback_resolve_multisample;
    else
      logger_add(
          LoggerFlag_Error,
          "Error while assigning render pass draw callback (pass: %s <%p>). No "
          "callback found "
          "for the Multisample %d, make sure the child render-pass has a valid "
          "multisample value.",
          last_pass->label, last_pass, last_pass->multisample);
  }
}

/**
   Create scene renderer draw config, which basically is an array of callback
   functions and mesh referecences list lists that will be picked during the
   draw loop.

   Basically for each draw call we require a "topology callback" and a
   "shader callback" to define which topology and shader we want to draw for
   each mesh.

   Note that the order of the array is relative to the RendererMode:

   0 - Texture config
          L Render Pass 1
          L Render Pass 2
               L Length
               L Draw Layouts[]
                    L Stored Mesh List * (linked with scene)
                    L Drawn Mesh List
                    L Shader Callback
                    L Topo Callback

   1 - Solid config
   2 - Wireframe config
   3 - Boundbox config

   By following this order, we can simply map the right array entry depending on
   the scene render mode.
 */

void render_pass_draw_list_copy(const RenderPassLayoutListDescriptor *src,
                                RenderPassLayoutList *dest) {

  size_t count = glm_imin(src->count, RENDER_PASS_MAX_DRAW_LIST);
  dest->count = count;

  for (size_t i = 0; i < count; i++) {

    const RenderPassLayoutDescriptor *s = &src->entries[i];
    RenderPassLayout *d = &dest->entries[i];

    d->shader = s->shader;
    d->pipeline = s->pipeline;
    d->topology_callback = s->topology_callback;
    d->mesh_preprocessor_callback = s->mesh_preprocessor_callback;
    d->mesh_preprocessor_data = s->mesh_preprocessor_data;

    //// DELETEME (useful debug)
    // printf("[%lu] pipeline: %s | shader: %d | mesh list: %p \n", i,
    //        std_render_pipeline_label(d->pipeline), d->shader,
    //        d->src_meshes->entries);

    mesh_draw_packet_list_create(&d->drawn_meshes, MESH_REF_LIST_CAPACITY);
  }
}

RenderPassStatus render_pass_update_preprocessor_data(RenderPass *pass,
                                                      uint8_t index,
                                                      void *data) {
  if (index > pass->layouts.count) {
    logger_add(LoggerFlag_Warning,
               "Trying to update an out of bound (%d) render pass "
               "preprocessor data. Target render pass has %lu draw lists.",
               index, pass->layouts.count);
    return RenderPassStatus_OutOfBoundDrawIndex;
  }

  pass->layouts.entries[index].mesh_preprocessor_data = data;

  return RenderPassStatus_Success;
}

RenderPassStatus render_pass_update_all_preprocessor_data(RenderPass *pass,
                                                          void *data) {

  for (size_t i = 0; i < pass->layouts.count; i++)
    if (pass->layouts.entries[i].mesh_preprocessor_callback)
      pass->layouts.entries[i].mesh_preprocessor_data = data;

  return RenderPassStatus_Success;
}

StaticListStatus render_pass_view_color_insert(RenderPass *pass,
                                               WGPUTextureView view) {

  return stli_insert((void *)pass->color.views, RENDER_PASS_VIEW_CAPACITY,
                     &pass->color.views_count, sizeof(WGPUTextureView),
                     (void *)&view, "Render Pass Color View List");
}

StaticListStatus render_pass_view_depth_insert(RenderPass *pass,
                                               WGPUTextureView view) {
  return stli_insert((void *)pass->depth.views, RENDER_PASS_VIEW_CAPACITY,
                     &pass->depth.views_count, sizeof(WGPUTextureView),
                     (void *)&view, "Render Pass Depth View List");
}

StaticListStatus render_pass_view_color_remove(RenderPass *pass,
                                               WGPUTextureView view) {
  return stli_remove((void *)pass->color.views, &pass->color.views_count,
                     sizeof(WGPUTextureView), (void *)view,
                     "Render Pass Color View List");
}
StaticListStatus render_pass_view_depth_remove(RenderPass *pass,
                                               WGPUTextureView view) {
  return stli_remove((void *)pass->depth.views, &pass->depth.views_count,
                     sizeof(WGPUTextureView), (void *)view,
                     "Render Pass Color View List");
}

WGPUTextureView render_pass_view_color(RenderPass *pass, size_t index) {
  return pass->color.views[index];
}
WGPUTextureView render_pass_view_depth(RenderPass *pass, size_t index) {
  return pass->depth.views[index];
}

/*
  All default stagging layout, however some of them might not have any drawn
  mesh, so we need a second list with the layout that have drawn meshes.

  We basically register or unregister a layout from the list depending on its
  drawn meshes count. Cause if they don't have any drawn mesh, there is no
  point calling setRenderPipeline

  TODO: replace this by:
  1 - sorting the layouts by putting the one with meshes first
  2 - adjusting the count (active count) according to it
*/
void render_pass_sync_drawn_layouts(RenderPass *pass) {

  // pass->layouts.count = 0;
  //
  //  for (size_t i = 0; i < pass->layouts.count; i++) {
  //    RenderPassLayout *stagged_layout = &pass->stagged_list.entries[i];
  //
  //    if (stagged_layout->drawn_meshes.count)
  //      pass->drawn_list.entries[pass->drawn_list.count++] = stagged_layout;
  //  }
}

/**
   Should `mesh` be drawn by `layout`, which lives in the pass for `layer`
   (pass index == layer bit) of draw mode `mode` (index)?
 */
bool render_pass_layout_accepts(Mesh *mesh, const RenderPassLayout *layout,
                                const size_t mode, const size_t layer) {

  const RendererMeshConfig *cfg = &mesh->render_config;

  // 1. flags predominate: a Hidden mesh is accepted nowhere
  if (cfg->flags & RendererMeshFlag_Hidden)
    return false;

  // 2. draw mode
  if (!(cfg->draw_mode & (1u << mode)))
    return false;

  // the mesh must have the shader this layout draws with
  Shader *shader = mesh_shader(mesh, layout->shader);
  if (!shader)
    return false;

  // 3. outline pass: takes selected meshes, on flags alone (its layouts use
  //    their own pipeline, so the layer and pipeline rules don't apply)
  if (layer == (size_t)__builtin_ctz(RendererLayer_Outline))
    return (cfg->flags & RendererMeshFlag_Selection) != 0;

  // 4. layer (a mesh on Layer_All goes to every pass)
  if (cfg->layer != RendererLayer_All && !(cfg->layer & (1u << layer)))
    return false;

  // 5. pipeline: the one used by the mesh's shader for this layout's shader
  return std_render_pipeline_type(*shader->pipeline) == layout->pipeline;
}

bool render_pass_layout_add_mesh(RenderPassLayout *layout, Mesh *mesh) {

  Shader *shader = mesh_shader(mesh, layout->shader);
  if (!shader)
    return false;

  MeshDrawPacket *pack = mesh_draw_packet_list_new_entry(&layout->drawn_meshes);
  if (!pack)
    return false;

  mesh_create_draw_packet(layout->topology_callback(mesh), shader, mesh, pack);
  return true;
}
