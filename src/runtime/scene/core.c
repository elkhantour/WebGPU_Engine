#include "core.h"

#include "./layer.h"
#include "backend/clock.h"
#include "backend/context.h"
#include "backend/logger.h"
#include "backend/registry.h"
#include "backend/renderer/core.h"
#include "backend/stat.h"
#include "backend/ubo.h"
#include "debug/core.h"
#include "event/event.html.h"
#include "runtime/camera/core.h"
#include "runtime/camera/list.h"
#include "runtime/light/core.h"
#include "runtime/light/list.h"
#include "runtime/mesh/core.h"
#include "runtime/mesh/list.h"
#include "runtime/mesh/ref_list.h"
#include "runtime/pipeline/render.h"
#include "runtime/probe/core.h"
#include "runtime/probe/reflection/core.h"
#include "runtime/probe/reflection/grid.h"
#include "runtime/probe/reflection/plane.h"
#include "runtime/scene/editor_mesh/list.h"
#include "runtime/scene/environment/core.h"
#include "runtime/scene/mesh_cache.h"
#include "runtime/scene/stat.h"
#include "runtime/texture/core.h"
#include "runtime/viewport/core.h"
#include "utils/dyli.h"

// initializers
static inline Camera *scene_init_main_camera(Scene *);
static inline void scene_camera_init(Scene *);

void scene_create(Scene *scene, const SceneCreateDescriptor *desc) {

  TIMER("Scene Load", {
    
    {
      scene->id = reg_register(scene, RegEntryType_Scene);
      scene->ubo = desc->ubo;
        stat_init(&scene->stats);
      /*  ===== SCENE RENDER =====   */
      scene_environment_init(&scene->environment);

    
    scene->environment.ubo_slot =
        ubo_new_entry(scene->ubo, UBOType_Environment);

    scene_environment_update_uniform(&scene->environment);

    ubo_upload_entry(scene->ubo, UBOType_Environment,
                     &scene->environment.ubo_slot);

    static const size_t SCENE_MESH_CACHE_CAPACITY = 32;
    scene_mesh_cache_init(&scene->mesh_cache, &scene->meshes, SCENE_MESH_CACHE_CAPACITY);
}

{

  /*  ===== MESH | SEM | LAYERS | LIGHTS | PROBES LISTS ===== */
  mesh_ref_list_create(&scene->meshes, SCENE_MESH_LIST_DEFAULT_CAPACITY);
  sem_list_array_create(&scene->editor_meshes,
                        SCENE_EDITOR_OBJECT_LIST_CAPACITY_DEFAULT);

  scene_layer_init(&scene->layers);

  {
    light_list_create(&scene->lights, LIGHT_MAX_CAPACITY);
    scene->lights.ubo_slot = ubo_new_entry(scene->ubo, UBOType_LightList);
  }

  {
    scene->probes.ubo_slot = ubo_new_entry(scene->ubo, UBOType_ProbeList);

    probe_reflection_grid_list_create(&scene->probes.reflection_probe,
                                      PROBE_REFLECTION_GRID_LIST_CAPACITY);

    probe_reflection_plane_list_create(&scene->probes.reflection_plane,
                                       PROBE_REFLECTION_GRID_LIST_CAPACITY);
  }
}

{
  /*  ===== CAMERA & VIEWPORT =====  */
  scene_camera_init(scene);

  viewport_create(&scene->viewport, &(ViewportCreateDescriptor){
                                        .fov = desc->viewport->fov,
                                        .near_clip = desc->viewport->near_clip,
                                        .far_clip = desc->viewport->far_clip,
                                        .width = context_width(),
                                        .height = context_height(),
                                    });

  scene->viewport.ubo_slot = ubo_new_entry(scene->ubo, UBOType_Viewport);
  viewport_uniform_update(&scene->viewport);
  ubo_upload_entry(scene->ubo, UBOType_Viewport, &scene->viewport.ubo_slot);
}

{
  /*  ===== EVENT =====  */
  scene_event_html(scene);
}

{
  /*  ===== EDITOR =====  */
  scene_debug_init(&scene->debug, &(SceneDebugDescriptor){
                                      .camera = scene->active_camera,
                                      .viewport = &scene->viewport,
                                      .ubo = scene->ubo,
                                  });
}
});
}

/**
   Create scene camera list and main camera.
 */
void scene_camera_init(Scene *scene) {

  // create camera list, and set active camera
  camera_list_create(&scene->cameras, SCENE_CAMERA_LIST_CAPACITY);
  scene->camera = scene_init_main_camera(scene);

  // set scene main camera as active
  scene->active_camera = scene->camera;
}

/**
   Define scene main edit camera
 */
Camera *scene_init_main_camera(Scene *scene) {

  Camera *camera = camera_list_new_camera(&scene->cameras);

  // create main camera
  camera_create(camera, &(CameraCreateDescriptor){
                            .speed = 20.0f,
                            .mode = CameraMode_Edit,
                            .sensitivity =
                                {
                                    .move = 0.02f,
                                    .rotate = 0.002f,
                                    .zoom = 0.02f,
                                },

                        });

  camera->ubo_slot = ubo_new_entry(scene->ubo, UBOType_Camera);

  // init main camera position
  camera_lookat(camera, (vec3){20.0f, 20.0f, 20.0f}, (vec3){0.0f, 0.0f, 0.0f});

  ubo_upload_entry(scene->ubo, UBOType_Camera, &camera->ubo_slot);

  return camera;
}

/**
   Quick access to a scene layer mesh list.
 */
MeshRefList *scene_layer_meshes(Scene *scene, const char *name) {

  SceneLayer *layer = scene_layer_set_find(&scene->layers, name);

  if (layer == NULL)
    return NULL;

  return &layer->meshes;
}

void scene_destroy(Scene *scene) {
  // TODO

  mesh_ref_list_free(&scene->meshes);
  scene_mesh_cache_destroy(&scene->mesh_cache);
}
