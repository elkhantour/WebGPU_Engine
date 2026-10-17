#include "view.h"

#include <cglm/types.h>
#include <cglm/vec3.h>
#include <stddef.h>

#include "backend/resource_manager.h"
#include "backend/shaders/core.h"
#include "core.h"
#include "runtime/mesh/core.h"
#include "runtime/mesh/list.h"
#include "runtime/mesh/ref_list.h"
#include "runtime/mesh/shader/core.h"
#include "runtime/mesh/transform.h"
#include "runtime/mesh/uniform.h"
#include "runtime/pipeline/render.h"
#include "runtime/primitive/core.h"
#include "runtime/primitive/plane.h"
#include "runtime/shader/core.h"
#include "runtime/shader/update.h"
#include "runtime/texture/core.h"

static inline void scene_debug_view_compute_position(MeshRefList *, vec3);

const float height_ratio = 9.0f / 16.0f;

void scene_debug_view_create(SceneDebug *debug, const WGPUTextureView view) {

  // create view mesh
  Mesh *mesh = rem_new_mesh();

  Primitive plane = primitive_plane();

  mesh_create_primitive(mesh, &(MeshCreatePrimitiveDescriptor){
                                  .primitive = &plane,
                                  .name = "Scene debug view",
                              });

  // set view texture
  mesh_shader_create(
      mesh, &(ShaderCreateDescriptor){
                .pipeline = std_render_pipeline(RenderPipelineType_Screen),
                .name = "Debug view billboard shader",
            });

  float ratio =
      ((float)debug->viewport->width / (float)debug->viewport->height) * 9.0f /
      16.0f;
  vec3 scale = {1.0f, 1.0f, ratio};
  glm_vec3_scale(scale, 0.3f, scale);
  mesh_set_scale(mesh, scale);

  // compute view new position
  vec3 new_position;

  scene_debug_view_compute_position(&debug->object_list[SceneDebugObject_View],
                                    new_position);
  mesh_set_position(mesh, new_position);

  // bind model matrix
  shader_update_uniform_data(mesh_shader(mesh, MeshShader_Texture), 0, 0,
                             mesh_uniform(mesh), ShaderUpdateFlag_None);

  // bind texture view
  shader_update_texture_view(mesh_shader(mesh, MeshShader_Texture), 1, 0, view,
                             TEXTURE_FORMAT_OFFSCREEN,
                             ShaderUpdateFlag_ReleasePrevious);

  mesh_ref_list_insert(&debug->object_list[SceneDebugObject_View], mesh);
}

void scene_debug_view_compute_position(MeshRefList *views, vec3 result) {

  float init_offset = 0.8f;
  float col = -init_offset;
  float row = init_offset;

  for (size_t v = 0; v < views->count; v++) {
    col += VIEW_MARGIN / 100.0f + views->entries[v]->scale[0];

    // skip to new line
    if (col >= init_offset) {
      col = -init_offset;
      row -= VIEW_MARGIN / 100.0f + views->entries[v]->scale[1];
    }
  }

  result[0] = col;
  result[2] = row;
}
