#include "view.h"

#include <cglm/cam.h>
#include <cglm/vec3.h>
#include <stddef.h>

#include "backend/logger.h"
#include "backend/shaders/core.h"
#include "runtime/camera/core.h"
#include "runtime/mesh/core.h"
#include "runtime/mesh/shader/core.h"
#include "runtime/mesh/transform.h"
#include "runtime/mesh/uniform.h"
#include "runtime/pipeline/render.h"
#include "runtime/primitive/core.h"
#include "runtime/primitive/plane.h"
#include "runtime/scene/debug/view.h"
#include "runtime/shader/core.h"
#include "runtime/shader/update.h"
#include "runtime/texture/core.h"
#include "runtime/viewport/core.h"

static void debug_view_compute_position(DebugView *, vec3);

void debug_view_create(DebugView *debug_view,
                       const DebugViewCreateDescriptor *desc) {
  debug_view->count = 0;
  debug_view->capacity = VIEW_MAX_CAPACITY;
}

void debug_view_add(DebugView *debug_view, const ViewDescriptor *view) {

  if (debug_view->count == debug_view->capacity) {
    logger_add(LoggerFlag_Print,
               "Debug view Currently holding max capacity, no more views "
               "can be added\n");
    return;
  }

  // create view mesh
  Mesh *new_view = &debug_view->mesh[debug_view->count++];
  Primitive plane = primitive_plane();

  mesh_create_primitive(new_view, &(MeshCreatePrimitiveDescriptor){
                                      .primitive = &plane,
                                      .name = "debug view",
                                  });

  // set view texture
  mesh_shader_create(
      new_view, &(ShaderCreateDescriptor){
                    .pipeline = std_render_pipeline(RenderPipelineType_Screen),
                    .name = "Debug view billboard shader",
                });

  mesh_set_scale(new_view, (vec3){view->size[0], 1.0f, view->size[1]});

  // compute view new position
  vec3 new_position;
  debug_view_compute_position(debug_view, new_position);
  mesh_set_position(new_view, new_position);

  // bind model & viewport matrix
  MeshUniform *uModel = mesh_uniform(new_view);
  ViewportUniform uViewport = {0};
  CameraUniform uCamera = {0};

  // create mock orthographic projection
  float ortho_size = 1.5f;
  float ratio = 16.0f / 9.0f;
  glm_ortho(-ortho_size * ratio, ortho_size * ratio, -ortho_size, ortho_size,
            0.1f, 100.0f, uViewport.projection);

  // create mock camera
  glm_lookat((vec3){0.0f, 1.0f, 0.0f}, GLM_VEC3_ZERO, (vec3){0.0f, 0.0f, -1.0f},
             uCamera.view);

  ShaderBindGroupUniformEntry entries[3] = {
      {
          .binding = 0,
          .size = sizeof(MeshUniform),
          .data = &uModel,
          .offset = 0,
      },
      {
          .binding = 1,
          .size = sizeof(ViewportUniform),
          .data = &uViewport,
          .offset = 0,
      },
      {
          .binding = 2,
          .size = sizeof(CameraUniform),
          .data = &uCamera,
          .offset = 0,
      },
  };

  for (size_t i = 0; i < 3; i++) {
    ShaderBindGroupUniformEntry *entry = &entries[i];
    shader_update_uniform_data(mesh_shader(new_view, MeshShader_Texture), 0,
                               entry->binding, entry->data,
                               ShaderUpdateFlag_None);
  }

  // bind texture view
  shader_update_texture_view(mesh_shader(new_view, MeshShader_Texture), 1, 0,
                             view->texture_view, TEXTURE_FORMAT_OFFSCREEN,
                             ShaderUpdateFlag_ReleasePrevious);

}

void debug_view_compute_position(DebugView *debug_view, vec3 result) {

  float init_offset = -2.0f;
  float col = init_offset;
  float row = init_offset;

  for (size_t v = 1; v < debug_view->count; v++) {
    col += VIEW_MARGIN / 100.0f + debug_view->mesh[v].scale[0];

    // skip to new line
    if (col >= 2.0) {
      col = init_offset;
      row += VIEW_MARGIN / 100.0f + debug_view->mesh[v].scale[1];
    }
  }

  result[0] = col;
  result[2] = row;
}

size_t debug_view_count(DebugView *db_view) { return db_view->count; }
