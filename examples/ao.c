#include "ao.h"
#include "runtime/engine/core.h"
#include "runtime/scene/mesh_query.h"

void example_ao(Engine *engine, bool debug) {

  AOBakeDrawDebug debug_options = {0};

  if (debug) {
    debug_options = (AOBakeDrawDebug){
        .debug_scene = &engine_get_active_scene(engine)->debug,
        .color = &(color){0.0f, 1.0f, 0.0f, 1.0f},
        .max_ray = 1,
    };
  }

  const MeshRefList *lit_meshes;
  scene_mesh_query_with_flags(engine_get_active_scene(engine),
                              RendererMeshFlag_Lit, &lit_meshes);

  ao_bake_draw_list(&engine_get_renderer(engine)->texture.ambient_occlusion,
                    &(AOBakeDrawDescriptor){
                        .mesh_list = lit_meshes,
                        .global =
                            {
                                AO_GLOBAL_RAY_AMOUNT,
                                AO_GLOBAL_RAY_MAX_DISTANCE,
                            },
                        .local =
                            {
                                AO_LOCAL_RAY_AMOUNT,
                                AO_LOCAL_RAY_MAX_DISTANCE,
                            },
                        .debug = &debug_options,
                    });
}
