#ifndef _ENGINE_ADD_H_
#define _ENGINE_ADD_H_

#include "backend/renderer/mesh_config.h"
#include "core.h"
#include "runtime/light/list.h"

typedef enum {
  EngineAddFlag_None = 0,
  EngineAddFlag_Hide = 1 << 0,
  EngineAddFlag_Unselectable = 1 << 1,
  EngineAddFlag_TreeHide = 1 << 2,
} EngineAddFlag;

#define ENGINE_ADD_AUTO_BATCH 0

Scene *engine_add_scene(Engine *, const SceneCreateDescriptor *);

// === Add Light ===
SceneEditorMeshList *engine_scene_add_point_light(Engine *,
                                                  PointLightDescriptor *,
                                                  const LightCreateFlag,
                                                  PointLight **);

SceneEditorMeshList *engine_scene_add_spot_light(Engine *,
                                                 SpotLightDescriptor *,
                                                 const LightCreateFlag,
                                                 SpotLight **);

SceneEditorMeshList *engine_scene_add_sun_light(Engine *, SunLightDescriptor *,
                                                const LightCreateFlag,
                                                SunLight **);

SceneEditorMeshList *engine_scene_add_ambient_light(Engine *,
                                                    AmbientLightDescriptor *,
                                                    AmbientLight **);

// === Add Probe ===
SceneEditorMeshList *engine_scene_add_probe_reflection_grid(
    Engine *, ProbeReflectionGridDescriptor *, ProbeReflectionGrid **);

SceneEditorMeshList *engine_scene_add_probe_reflection_plane(
    Engine *, ProbeReflectionPlaneDescriptor *, ProbeReflectionPlane **);

// === Add Camera ===
SceneEditorMeshList *
engine_scene_add_camera(Engine *, const CameraCreateDescriptor *, Camera **);

// === Add Mesh ===

// automatically assign the mesh within the default renderer batch based on its
// render pipeline type.
EngineStatus engine_scene_add_mesh(Engine *, Mesh *, const char *,
                                   const EngineAddFlag);

// manually enter the batch descriptor
EngineStatus engine_scene_add_mesh_custom(Engine *, Mesh *, const char *,
                                          const RendererMeshConfigDescriptor *,
                                          const EngineAddFlag);

// === Remove Mesh ===
EngineStatus engine_scene_remove_mesh(Engine *, Mesh *);
EngineStatus engine_scene_remove_mesh_ref_list(Engine *, MeshRefList *);

#endif
