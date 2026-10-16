#include "mesh_query.h"

SceneMeshCacheStatus
scene_mesh_query_with_flags(Scene *scene,
                               const RendererMeshFlag flag,
                               const MeshRefList **result) {
  return scene_mesh_cache_get(&scene->mesh_cache, SceneMeshCacheType_Flag, flag,
                                 result);
}

SceneMeshCacheStatus
scene_mesh_query_without_flags(Scene *scene,
                                  const RendererMeshFlag flag,
                                  const MeshRefList **result) {
  return scene_mesh_cache_get(&scene->mesh_cache, SceneMeshCacheType_WithoutFlag, flag,
                                 result);
}

SceneMeshCacheStatus
scene_mesh_query_from_layer(Scene *scene,
                               const RendererLayer layer,
                               const MeshRefList **result) {
  // "All" has no cache slot: everything matches, so return the source
  if (layer == RendererLayer_All) {
    *result = scene_mesh_cache_source(&scene->mesh_cache);
    return SceneMeshCacheStatus_Success;
  }

  return scene_mesh_cache_get(&scene->mesh_cache, SceneMeshCacheType_Layer, layer,
                                 result);
}

SceneMeshCacheStatus
scene_mesh_query_from_pipeline(Scene *scene,
                                  const RenderPipelineType pipeline,
                                  const MeshRefList **result) {
  return scene_mesh_cache_get(&scene->mesh_cache, SceneMeshCacheType_Pipeline,
                                 (size_t)pipeline, result);
}

// TODO replace with Preset
MeshRefList *scene_mesh_query_from_config(SceneMeshCache *cache,
                                             const RendererMeshConfig *key) {
  (void)cache;
  (void)key;
  return NULL;
}
