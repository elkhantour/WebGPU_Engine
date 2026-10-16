#ifndef _SCENE_MESH_QUERY_H_
#define _SCENE_MESH_QUERY_H_

#include "backend/renderer/mesh_config.h"
#include "runtime/scene/core.h"
#include "utils/defines.h"

#include "runtime/mesh/core.h"
#include "runtime/scene/mesh_cache.h"

// The query layer has no state: it translates "what the renderer wants"
// into lookups on a SceneMeshCache. It reuses the cache status codes.
//
// Results are read-only, owned by the cache, and valid until the next
// cache refresh/clear/destroy. Copy (mesh_ref_list_create_and_copy) if you
// need to modify them.

EXTERN_C_BEGIN

SceneMeshCacheStatus scene_mesh_query_with_flags(Scene *,
                                                 const RendererMeshFlag,
                                                 const MeshRefList **result);

SceneMeshCacheStatus scene_mesh_query_without_flags(Scene *,
                                                    const RendererMeshFlag,
                                                    const MeshRefList **result);

// RendererLayer_All returns the cache's source list (everything matches).
SceneMeshCacheStatus scene_mesh_query_from_layer(Scene *, const RendererLayer,
                                                 const MeshRefList **result);

SceneMeshCacheStatus scene_mesh_query_from_pipeline(Scene *,
                                                    const RenderPipelineType,
                                                    const MeshRefList **result);

// TODO replace with Preset
// Combine pipeline + layer + flags by starting from the most selective
// cache (pipeline) and filtering by the rest.
MeshRefList *scene_mesh_query_from_config(SceneMeshCache *,
                                          const RendererMeshConfig *);

EXTERN_C_END

#endif
