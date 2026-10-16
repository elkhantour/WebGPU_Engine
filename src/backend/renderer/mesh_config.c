#include "mesh_config.h"
#include "backend/logger.h"
#include "backend/renderer/core.h"
#include "backend/std_pipeline/core.h"
#include "runtime/mesh/core.h"
#include "runtime/mesh/ref_list.h"
#include "runtime/pipeline/render.h"

#include <stdint.h>

/**

  Initialise the renderer batch hash table based on the configuration above.
  We also initalise eatch Renderer Batch Bucket's mesh list as to append meshes
  pointer in it.

  Renderer Batch allows to efficiently group meshes based on their pipeline and
  properties such as:
  - allow shadow pass
  - allow reflection pass
  - being in absolute front or in backgound
  - using a fixed render mode

  According to those proerties (defined as flags) as well as targeted render
  pipeline type we define a hash.
 */


const RendererMeshConfig *
renderer_mesh_config_from_preset(const RendererMeshPreset preset) {
  return &RENDERER_MESH_CONFIG_PRESET[preset];
}

const RendererMeshConfig *
renderer_mesh_config_from_pipeline(const RenderPipelineType pipeline_type) {

  if (pipeline_type == RENDER_PIPELINE_UNDEFINED) {
    logger_add(LoggerFlag_Error,
               "Couldn't find any valid type for mesh pipeline.");
    return NULL;
  }

  for (uint8_t i = 0; i < RendererMeshPreset_COUNT; i++)
    if (RENDERER_MESH_CONFIG_PRESET[i].pipeline == pipeline_type)
      return &RENDERER_MESH_CONFIG_PRESET[i];

  return NULL;
}

const RendererMeshConfig *
renderer_mesh_config_from_descriptor(const RendererMeshConfigDescriptor *desc) {

  for (uint8_t i = 0; i < RendererMeshPreset_COUNT; i++)
    if (RENDERER_MESH_CONFIG_PRESET[i].flags == desc->flags &&
        RENDERER_MESH_CONFIG_PRESET[i].layer == desc->layer &&
        RENDERER_MESH_CONFIG_PRESET[i].pipeline == desc->pipeline)
      return &RENDERER_MESH_CONFIG_PRESET[i];

  return NULL;
}
