#include "billboard.h"

#include <stdbool.h>
#include <stdint.h>

#include "backend/shaders/core.h"
#include "runtime/geometry/vertex/attribute.h"
#include "runtime/mesh/core.h"
#include "runtime/mesh/shader/core.h"
#include "runtime/mesh/transform.h"
#include "runtime/pipeline/render.h"
#include "runtime/primitive/core.h"
#include "runtime/primitive/plane.h"
#include "runtime/shader/core.h"
#include "runtime/shader/update.h"
#include "runtime/texture/core.h"
#include "runtime/texture/create.h"
#include "webgpu/webgpu.h"

/**
   Create a plane mesh with a billboard shader
 */
void sem_create_billboard(Mesh *mesh,
                          const SEMCreateBillboardDescriptor *desc) {

  // create plane
  Primitive plane = primitive_plane();

  {
    const vertex_uv new_uv[4] = {
        {desc->uv0[0], desc->uv1[1]},
        {desc->uv1[0], desc->uv1[1]},
        {desc->uv1[0], desc->uv0[1]},
        {desc->uv0[0], desc->uv0[1]},
    };

    for (uint8_t i = 0; i < 4; i++)
      vertex_attribute_set_uv_at_index(&plane.vertex, new_uv[i], i);
  }

  mesh_create_primitive(mesh, &(MeshCreatePrimitiveDescriptor){
                                  .primitive = &plane,
                                  .name = "SEM Billboard",
                              });

  // assign billboard shader
  mesh_shader_create(
      mesh, &(ShaderCreateDescriptor){
                .name = "SEM billboard shader",
                .pipeline = std_render_pipeline(RenderPipelineType_Billboard),
            });

  // set mesh position to light position
  mesh_set_position(mesh, *desc->position);

  // scale down gizmo
  mesh_set_scale(mesh, *desc->scale);

  // bind texture + sampler
  shader_update_texture_view(mesh_shader(mesh, MeshShader_Texture), 1, 0,
                             desc->view, TEXTURE_FORMAT_OFFSCREEN,
                             ShaderUpdateFlag_ReleasePrevious);

  const uint32_t size = 0;
  shader_update_uniform_data(mesh_shader(mesh, MeshShader_Texture), 1, 2,
                             (void *)&size, ShaderUpdateFlag_None);
}
