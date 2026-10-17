#include "grid.h"

#include <cglm/types.h>

#include "backend/shaders/core.h"
#include "runtime/mesh/core.h"
#include "runtime/mesh/shader/core.h"
#include "runtime/mesh/transform.h"
#include "runtime/pipeline/render.h"
#include "runtime/primitive/core.h"
#include "runtime/primitive/plane.h"
#include "runtime/shader/core.h"
#include "runtime/shader/update.h"

void sem_grid_create(Mesh *mesh, const GridUniform *uniform) {

  Primitive plane = primitive_plane();

  mesh_create_primitive(mesh, &(MeshCreatePrimitiveDescriptor){
                                  .name = "grid",
                                  .primitive = &plane,
                              });

  mesh_shader_create(
      mesh, &(ShaderCreateDescriptor){
                .pipeline = std_render_pipeline(RenderPipelineType_Grid),
                .name = "grid",
            });

  mesh_set_scale(mesh, (vec3){
                           uniform->size,
                           uniform->size,
                           uniform->size,
                       });

  shader_update_uniform_data(mesh_shader(mesh, MeshShader_Texture), 1, 0,
                             (void *)uniform, ShaderUpdateFlag_None);
}
