#include "wireframe.h"

#include "backend/shaders/core.h"
#include "runtime/geometry/line/core.h"
#include "runtime/mesh/core.h"
#include "runtime/mesh/shader/core.h"
#include "runtime/mesh/topology/core.h"
#include "runtime/mesh/topology/wireframe.h"
#include "runtime/pipeline/render.h"
#include "runtime/shader/core.h"
#include "runtime/shader/update.h"

/**
   Setup a wireframe mesh with the given vertex/index attributes and color.
   Since gizmo is part of the Fixed pipeline. It's necessary to :
   1. call mesh wireframe topology manually
   2. set override topology wireframe

   For the casual pipeline ('lit' and 'unlit' meshes), the wireframe is
   automatically handled during the scene build scene.
   However since here gizmo are part of the fixed pipeline, it's the developer
   responsibility to handle the wireframe generation as well as the overriden
   topology/shader to be rendered all the time.
 */
void sem_create_wireframe(Mesh *mesh,
                          const SEMCreateWireframeDescriptor *desc) {

  // create mesh from vertex/index attributes
  mesh_create(mesh, &(MeshCreateDescriptor){
                        .index = *desc->index,
                        .vertex = *desc->vertex,
                        .name = desc->name,
                    });

  // generate wirerfame topology
  MeshTopology base_topo = mesh_topology_base(mesh);
  MeshTopologyWireframe *wireframe_topo = &mesh->topology.wireframe;
  mesh_topology_wireframe_create(&base_topo, wireframe_topo);

  // set wireframe shader
  mesh_shader_create(
      mesh, &(ShaderCreateDescriptor){
                .name = "SEM wireframe shader",
                .pipeline = std_render_pipeline(RenderPipelineType_Line),
            });

  const float line_thickness = LINE_THICKNESS_BASE;
  shader_update_uniform_data(mesh_shader(mesh, MeshShader_Texture), 1, 1,
                             (void *)&line_thickness, ShaderUpdateFlag_None);

  shader_update_uniform_data(mesh_shader(mesh, MeshShader_Texture), 1, 0,
                             desc->color, ShaderUpdateFlag_None);

  // set override topology and shader as wireframe
  mesh_topology_set_override(mesh, mesh_topology_wireframe(mesh));
}

/*
  Generic highligh function for all SEM Wireframes objects
 */
void sem_wireframe_select_callback(SEMHighlightCallback *desc) {

  Shader *shader = mesh_shader(desc->sem->mesh, MeshShader_Texture);

  const float line_thickness = LINE_THICKNESS_STRONG;
  shader_update_uniform_data(shader, 1, 1, (void *)&line_thickness,
                             ShaderUpdateFlag_None);
  shader_update_uniform_data(shader, 1, 0, &(color){1.0f, 0.0f, 0.0f, 1.0f},
                             ShaderUpdateFlag_None);
}

void sem_wireframe_deselect_callback(SEMHighlightCallback *desc) {

  Shader *shader = mesh_shader(desc->sem->mesh, MeshShader_Texture);

  const float line_thickness = LINE_THICKNESS_BASE;
  shader_update_uniform_data(shader, 1, 1, (void *)&line_thickness,
                             ShaderUpdateFlag_None);
  shader_update_uniform_data(shader, 1, 0, &(color){0.0f, 0.0f, 0.0f, 1.0f},
                             ShaderUpdateFlag_None);
}
