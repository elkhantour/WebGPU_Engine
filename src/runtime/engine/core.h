#ifndef _ENGINE_CORE_H_
#define _ENGINE_CORE_H_

#include "backend/job_system.h"
#include "backend/renderer/core.h"
#include "runtime/gui/core.h"
#include "runtime/light/core.h"
#include "runtime/mesh/core.h"
#include "runtime/pipeline/render.h"
#include "runtime/scene/add.h"
#include "runtime/scene/core.h"
#include <stdint.h>

/**
   The Engine implementation serves as a high level orchestration that
   automatically links up engine entities for systematized process (add, remove,
   move, update...).
   As a result instead of having:
   {
     scene_add_mesh();
     renderer_update_shadow_map();
     ubo_new_entry();
   }

   we would simply call engine_add_mesh();
 */

#define ENGINE_SCENE_CAPACITY 3

typedef enum {
  EngineStatus_Success,
  EngineStatus_ReachMaxCapacity,
  EngineStatus_AllocFail,
  EngineStatus_UnfoundEntity,
  EngineStatus_InvalidPipeline,
  EngineStatus_RenderModeEqual,
  EngineStatus_UndefError,
} EngineStatus;

typedef struct {

  Renderer *renderer;
  JobSystem *job_system;
  Gui *gui;

  struct {
    Scene *entries[ENGINE_SCENE_CAPACITY];
    uint8_t count;
    Scene *active;
  } scenes;

} Engine;

EngineStatus engine_init(Engine *);

EngineStatus engine_set_active_scene(Engine *, Scene *);

static inline Scene *engine_get_active_scene(Engine *engine) {
  return engine->scenes.active;
}

static inline Renderer *engine_get_renderer(Engine *engine) {
  return engine->renderer;
}

static inline Gui *engine_get_gui(Engine *engine) { return engine->gui; }

#endif
