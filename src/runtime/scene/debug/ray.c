#include "ray.h"

#include <stdio.h>

#include "backend/resource_manager.h"
#include "runtime/geometry/line/core.h"
#include "runtime/mesh/core.h"
#include "runtime/mesh/list.h"
#include "runtime/mesh/ref_list.h"
#include "runtime/mesh/shader/core.h"
#include "runtime/shader/update.h"
#include "utils/color.h"

void scene_debug_ray_create(SceneDebug *debug, Mesh **line) {
  *line = rem_new_mesh();
  line_create(*line, &(LineCreateDescriptor){
                         .name = "Debug line mesh",
                     });

  mesh_shader_bind_mvp(*line, MeshShader_Texture, debug->ubo);

  shader_update_uniform_data(mesh_shader(*line, MeshShader_Texture), 1, 0,
                             &(color){0.0f, 1.0f, 0.0f, 1.0f},
                             ShaderUpdateFlag_None);
}

void scene_debug_ray_add_point(Mesh *line, vec3 origin, vec3 target,
                               color color) {

  line_add_point(origin, target, color, &line->topology.base.attribute,
                 &line->topology.base.index);
}

void scene_debug_ray_build(SceneDebug *debug, Mesh *line) {
  line_update_buffer(line);
  mesh_ref_list_insert(&debug->object_list[SceneDebugObject_Ray], line);
  printf("debug ray count: %lu\n",
         debug->object_list[SceneDebugObject_Ray].count);
}
