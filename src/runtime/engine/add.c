#include "add.h"
#include "backend/logger.h"
#include "backend/renderer/batch.h"
#include "backend/renderer/core.h"
#include "backend/renderer/reflection/draw.h"
#include "backend/renderer/render_pass/core.h"
#include "backend/renderer/render_pass/visibility.h"
#include "backend/renderer/shadow_map/draw.h"
#include "backend/resource_manager.h"
#include "backend/std_pipeline/core.h"
#include "runtime/engine/build.h"
#include "runtime/engine/core.h"
#include "runtime/mesh/core.h"
#include "runtime/mesh/ref_list.h"
#include "runtime/mesh/shader/core.h"
#include "runtime/pipeline/render.h"
#include "runtime/scene/add.h"
#include "runtime/scene/core.h"
#include "runtime/scene/selection/core.h"
#include "runtime/scene/stat.h"
#include "runtime/systems/visibility_system.h"
#include <stdint.h>

static inline void engine_scene_add_sem(Engine *, SceneEditorMeshList *);

static inline void engine_enable_mesh_in_passes(Engine *, Mesh *,
                                                const RenderPipelineType);

static inline void engine_add_mesh_core(Engine *, Mesh *, const char *,
                                        const RendererBatchKey *,
                                        const EngineAddFlag);

Scene *engine_add_scene(Engine *engine, const SceneCreateDescriptor *desc) {

  if (engine->scenes.count >= ENGINE_SCENE_CAPACITY) {
    logger_add(LoggerFlag_Error, "Engine reached max scene capacity (%lu)",
               ENGINE_SCENE_CAPACITY);
    return NULL;
  }

  Scene *scene = rem_new_scene();
  scene_create(scene, desc);

  if (!scene) {
    logger_add(LoggerFlag_Error,
               "Resource manager unable to provide a new scene.",
               ENGINE_SCENE_CAPACITY);
    return NULL;
  }

  engine->scenes.entries[engine->scenes.count++] = scene;

  return scene;
}

/**
  Update passes draw list (sync with their respective scene pipeline)
 */
void engine_enable_mesh_in_passes(Engine *engine, Mesh *mesh,
                                  const RenderPipelineType pipeline) {

  Scene *scene = engine_get_active_scene(engine);
  Renderer *renderer = engine_get_renderer(engine);

  visibility_system_show_mesh(scene, renderer, mesh);
  scene_stat_update_draw_call_count(scene, 0);
  scene_stat_update_vertex_count(scene, 0);
}

// === Add Light ===
SceneEditorMeshList *engine_scene_add_point_light(Engine *engine,
                                                  PointLightDescriptor *desc,
                                                  const LightCreateFlag flag,
                                                  PointLight **dest) {

  Scene *scene = engine_get_active_scene(engine);
  Renderer *renderer = engine_get_renderer(engine);

  PointLight *light;
  SceneEditorMeshList *sem_list =
      scene_add_point_light(scene, desc, flag, &light);

  if (dest)
    *dest = light;

  // recompute shadow map if render mode
  if ((flag & LightCreateFlag_Shadow) &&
      renderer_draw_mode(renderer) == RendererDrawMode_Texture) {

    PointLightListShadow *shadow_list = &scene->lights.point.shadow;
    renderer_draw_shadow_map_point_light(
        &(ShadowMapDrawPointLightDescriptor){
            .light = light,
            .pass = &scene->lights.point.shadow.pass,
            .texture_layer = shadow_list->count,
            .command_encoder = NULL,
            .profiler = &renderer->profiler,
        },
        SCENE_DEBUG_UNDEFINED);
  }

  engine_scene_add_sem(engine, sem_list);

  return sem_list;
}

SceneEditorMeshList *engine_scene_add_spot_light(Engine *engine,
                                                 SpotLightDescriptor *desc,
                                                 const LightCreateFlag flag,
                                                 SpotLight **dest) {

  Scene *scene = engine_get_active_scene(engine);
  Renderer *renderer = engine_get_renderer(engine);

  SpotLight *light;
  SceneEditorMeshList *sem_list =
      scene_add_spot_light(scene, desc, flag, &light);

  if (dest)
    light = *dest;

  if ((flag & LightCreateFlag_Shadow) &&
      renderer_draw_mode(renderer) == RendererDrawMode_Texture) {

    SpotLightListShadow *shadow_list = &scene->lights.spot.shadow;
    renderer_draw_shadow_map_spot_light(
        &(ShadowMapDrawSpotLightDescriptor){
            .light = light,
            .pass = &shadow_list->pass,
            .texture_layer = shadow_list->count,
            .command_encoder = NULL,
            .profiler = &renderer->profiler,
        },
        SCENE_DEBUG_UNDEFINED);
  }

  engine_scene_add_sem(engine, sem_list);

  return sem_list;
}

SceneEditorMeshList *engine_scene_add_sun_light(Engine *engine,
                                                SunLightDescriptor *desc,
                                                const LightCreateFlag flag,
                                                SunLight **dest) {

  Scene *scene = engine_get_active_scene(engine);
  Renderer *renderer = engine_get_renderer(engine);

  SunLight *light;
  SceneEditorMeshList *sem_list =
      scene_add_sun_light(scene, desc, flag, &light);

  if (dest)
    *dest = light;

  // recompute shadow map if render mode
  if ((flag & LightCreateFlag_Shadow) &&
      renderer_draw_mode(renderer) == RendererDrawMode_Texture) {

    SunLightListShadow *shadow_list = &scene->lights.sun.shadow;
    const size_t target_index =
        light_list_sun_layer_index(&scene->lights, shadow_list->count);

    renderer_draw_shadow_map_sun_light(
        &(ShadowMapDrawSunLightDescriptor){
            .light = light,
            .pass = &scene->lights.spot.shadow.pass,
            .texture_layer = target_index,
            .command_encoder = NULL,
            .profiler = &renderer->profiler,
        },
        SCENE_DEBUG_UNDEFINED);
  }

  engine_scene_add_sem(engine, sem_list);

  return sem_list;
}

SceneEditorMeshList *
engine_scene_add_ambient_light(Engine *engine, AmbientLightDescriptor *desc,
                               AmbientLight **dest) {

  Scene *scene = engine_get_active_scene(engine);
  SceneEditorMeshList *sem_list = scene_add_ambient_light(scene, desc, dest);
  engine_scene_add_sem(engine, sem_list);

  return sem_list;
}

// === Add Probe ===
SceneEditorMeshList *
engine_scene_add_probe_reflection_grid(Engine *engine,
                                       ProbeReflectionGridDescriptor *desc,
                                       ProbeReflectionGrid **dest) {

  Scene *scene = engine_get_active_scene(engine);

  SceneEditorMeshList *sem_list =
      scene_add_probe_reflection_grid(scene, desc, dest);
  engine_scene_add_sem(engine, sem_list);

  return sem_list;
}

SceneEditorMeshList *
engine_scene_add_probe_reflection_plane(Engine *engine,
                                        ProbeReflectionPlaneDescriptor *desc,
                                        ProbeReflectionPlane **dest) {

  Scene *scene = engine_get_active_scene(engine);
  Renderer *renderer = engine_get_renderer(engine);

  // add draw callback if first probe
  if (scene->probes.reflection_plane.count == 0) {

    // TODO: FREE RESOURCE
    RendererDrawPlaneRelfectionData *data =
        calloc(1, sizeof(RendererDrawPlaneRelfectionData));

    if (data != NULL) {
      data->list = &scene->probes.reflection_plane;
      data->renderer = renderer;
    }

    renderer_add_draw_callback(renderer, renderer_draw_plane_reflection,
                               (void *)data, RendererDrawMode_Texture);
  }

  SceneEditorMeshList *sem_list =
      scene_add_probe_reflection_plane(scene, desc, dest);
  engine_scene_add_sem(engine, sem_list);

  return sem_list;
}

// === Add Camera ===
SceneEditorMeshList *engine_scene_add_camera(Engine *engine,
                                             const CameraCreateDescriptor *desc,
                                             Camera **dest) {

  Scene *scene = engine_get_active_scene(engine);
  SceneEditorMeshList *sem_list = scene_add_camera(scene, desc, dest);
  engine_scene_add_sem(engine, sem_list);

  return sem_list;
}

/**
   Compared to casual meshes, Scene Editor Objects (lights/ camera) need to
   follow a more specific path when it comes to be added to the scene and
   especially how they are handled for selection.

   SEM are segmented into:
   1. Meshes list: a visual helper/ representaiton of the entity
   2. Target: the actual data

   As a result in order to add them to the scene we need to add their meshes
   list in the scene pool, but need to add them in a different "branch" of the
   selection system (SceneSelectionType_SEM).

   We use the below function to do such operation.
 */
void engine_scene_add_sem(Engine *engine, SceneEditorMeshList *list) {

  Scene *scene = engine_get_active_scene(engine);
  Renderer *renderer = engine_get_renderer(engine);

  for (size_t i = 0; i < list->count; i++) {
    Mesh *mesh = list->entries[i].mesh;

    engine_scene_add_mesh_custom(
        engine, mesh, NULL,
        // default configuration for Scene Editor Meshes
        &(RendererBatchKeyDescriptor){
            .flags = RendererBatchFlag_Fixed,
            .layer = RendererLayer_Default,
            .pipeline = std_render_pipeline_type(
                (*mesh_shader(mesh, MeshShader_Texture)->pipeline)),
            .draw_mode = RendererDrawMode_All,
        },
        // Note that we do not add the mesh to ui
        // tree since we actually add the whole
        // List id below. Same for the selection.
        EngineAddFlag_TreeHide | EngineAddFlag_Unselectable);

    scene_selection_register_mesh(&scene->selection, mesh, list->id,
                                  SceneSelectionType_SEM);
  }

  // EDITORONLY
  if (engine->gui)
    gui_tree_insert(&engine->gui->tree, list->id);
}

// === Add Mesh ===

static inline void
engine_add_mesh_core_insert_pipeline_batch(Engine *, Mesh *,
                                           const RenderPipelineType);

static inline void engine_add_mesh_core_insert_shadow_batch(Engine *, Mesh *);

static inline void
engine_add_mesh_core_insert_selection_batch(Engine *, Mesh *,
                                            const SceneSelectionType);

void engine_add_mesh_core_insert_pipeline_batch(
    Engine *engine, Mesh *mesh, const RenderPipelineType pipeline) {

  Renderer *renderer = engine_get_renderer(engine);

  RendererBatchMeshLists mesh_lists;
  renderer_batch_get_mesh_list_from_pipeline(&renderer->batches, pipeline,
                                             &mesh_lists);

  // insert mesh in each batch that have this pipeline type
  for (size_t i = 0; i < mesh_lists.count; i++)
    mesh_ref_list_insert(mesh_lists.entries[i], mesh);
}

void engine_add_mesh_core_insert_shadow_batch(Engine *engine, Mesh *mesh) {

  Scene *scene = engine_get_active_scene(engine);
  Renderer *renderer = engine_get_renderer(engine);

  RendererBatchMeshLists shadow_mesh_lists;
  renderer_batch_get_mesh_list_from_pipeline(
      &renderer->batches, RenderPipelineType_Shadow, &shadow_mesh_lists);

  for (size_t i = 0; i < shadow_mesh_lists.count; i++) {

    mesh_ref_list_insert(shadow_mesh_lists.entries[i], mesh);

    // eventually trigger de draw shadow to update scene shadow
    if (renderer->draw_mode == RendererDrawMode_Texture) {
      renderer_draw_shadow_map_all(
          &(ShadowMapDrawAllDescriptor){
              .mesh_list = shadow_mesh_lists.entries[i],
              .lights = &scene->lights,
              .profiler = &renderer->profiler,
          },
          SCENE_DEBUG_UNDEFINED);
    }
  }
}

void engine_add_mesh_core_insert_selection_batch(
    Engine *engine, Mesh *mesh, const SceneSelectionType selection_type) {

  Scene *scene = engine_get_active_scene(engine);
  Renderer *renderer = engine_get_renderer(engine);

  scene_selection_register_mesh(&scene->selection, mesh, mesh->id,
                                selection_type);

  // add to "selection" batch meshes list (stencil & outline) so we can
  // enable them on hightlight.
  if (SceneSelectionType_Mesh == selection_type ||
      SceneSelectionType_MeshShadow == selection_type) {

    RendererBatchMeshLists selection_lists;
    renderer_batch_get_mesh_list_with_flags(
        &renderer->batches, RendererBatchFlag_Selection, &selection_lists);

    for (size_t i = 0; i < selection_lists.count; i++)
      mesh_ref_list_insert(selection_lists.entries[i], mesh);
  }
}

void engine_add_mesh_core(Engine *engine, Mesh *mesh, const char *layer,
                          const RendererBatchKey *batch,
                          const EngineAddFlag flag) {

  Scene *scene = engine_get_active_scene(engine);
  Renderer *renderer = engine_get_renderer(engine);
  SceneSelectionType selection_type = SceneSelectionType_Mesh;

  /*
     LAYER:
     add to scene layers ('Default' layer if NULL)
  */
  scene_layer_set_insert_mesh(&scene->layers,
                              layer ? layer : SCENE_LAYER_DEFAULT, mesh);

  /*
     PIPELINE:
     insert the mesh in each renderer batch that have the same pipeline as the
     provided one.
   */
  engine_add_mesh_core_insert_pipeline_batch(engine, mesh, batch->pipeline);

  /*
     SHADOW:
     if batch has a shadow flag, it means we need to insert the mesh in
     the batch that owns all the 'shadowable' meshes.
   */
  if ((batch->flags & RendererBatchFlag_Shadow)) {
    engine_add_mesh_core_insert_shadow_batch(engine, mesh);
    selection_type = SceneSelectionType_MeshShadow;
  }

  /*
     VISIBILITY:
     Actually enable and show the mesh in each batch that share this pipeline
  */
  if ((EngineAddFlag_Hide & flag) == 0)
    engine_enable_mesh_in_passes(engine, mesh, batch->pipeline);

  /*
     [EDITORONLY] SELECTION:
     register mesh to the selection system so it can be
     detected with raycast and be highlighted with the right callback.
   */
  if ((EngineAddFlag_Unselectable & flag) == 0)
    engine_add_mesh_core_insert_selection_batch(engine, mesh, selection_type);

  /*
     [EDITORONLY] TREE:
     Add the mesh to the GUI tree
  */
  if (engine_get_gui(engine) && (flag & EngineAddFlag_TreeHide) == 0 &&
      mesh->parent == NULL)
    gui_tree_insert(&engine_get_gui(engine)->tree, mesh->id);
}

/*
   Automatically map renderer pipeline based on mesh pso pointer
 */
EngineStatus engine_scene_add_mesh(Engine *engine, Mesh *mesh,
                                   const char *layer,
                                   const EngineAddFlag flag) {

  Scene *scene = engine_get_active_scene(engine);
  SceneStatus add_result = scene_add_mesh(scene, mesh, layer);

  static const MeshShader dynamic_shaders[] = {
      MeshShader_Texture,
      MeshShader_Solid,
      MeshShader_Wireframe,
  };
  static const uint8_t dynamic_shaders_count =
      sizeof(dynamic_shaders) / sizeof(dynamic_shaders[0]);

  // For dynamic objects we manually add them to the solid/wireframe/boundbox
  // batch so they get drawn during those mode.
  for (uint8_t i = 0; i < dynamic_shaders_count; i++) {

    const RenderPipelineType pipeline = std_render_pipeline_type(
        *mesh_shader(mesh, dynamic_shaders[i])->pipeline);

    const RendererBatchKey *batch_config =
        renderer_batch_get_key_from_pipeline(pipeline);

    // only build mesh once
    if (i == 0) {
      engine_build_mesh(engine, mesh, batch_config->flags);
      engine_add_mesh_core(engine, mesh, layer, batch_config, flag);
    } else {
      // only add them to the pass list (do not add them to tree etc...)
      engine_add_mesh_core(engine, mesh, layer, batch_config,
                           flag | EngineAddFlag_TreeHide |
                               EngineAddFlag_Unselectable);
    }

    // exit after applying the Texture for fixed mesh (since they won't change
    // shaders on different draw mode)
    if (batch_config->flags & RendererBatchFlag_Fixed)
      break;
  }

  return EngineStatus_Success;
}

/*
  Manually Define the mesh Renderer Batch configuration
 */
EngineStatus
engine_scene_add_mesh_custom(Engine *engine, Mesh *mesh, const char *layer,
                             const RendererBatchKeyDescriptor *batch,
                             const EngineAddFlag flag) {

  Scene *scene = engine_get_active_scene(engine);
  SceneStatus add_result = scene_add_mesh(scene, mesh, layer);

  // get source batch from descriptor
  const RendererBatchKey *source_batch =
      renderer_batch_get_key_from_descriptor(batch);

  if (source_batch == NULL) {
    logger_add(LoggerFlag_Error,
               "Unable to locate the configured batch for mesh '%s', make sure "
               "the batch configuration matches with one in the configuration.",
               mesh->name);
    return EngineStatus_UnfoundEntity;
  }

  engine_build_mesh(engine, mesh, source_batch->flags);
  engine_add_mesh_core(engine, mesh, layer, source_batch, flag);

  return EngineStatus_Success;
}

// === Remove Mesh ===
EngineStatus engine_scene_remove_mesh(Engine *engine, Mesh *mesh) {

  Scene *scene = engine_get_active_scene(engine);
  scene_remove_mesh(scene, mesh);

  return EngineStatus_Success;
}

EngineStatus engine_scene_remove_mesh_ref_list(Engine *engine,
                                               MeshRefList *list) {

  Scene *scene = engine_get_active_scene(engine);
  scene_remove_mesh_ref_list(scene, list);

  return EngineStatus_Success;
}
