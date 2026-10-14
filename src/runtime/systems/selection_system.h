#ifndef _SELECTION_SYSTEM_H_
#define _SELECTION_SYSTEM_H_

#include "backend/renderer/core.h"
#include "runtime/gizmo/core.h"
#include "runtime/mesh/ref_list.h"
#include "runtime/scene/core.h"
#include "runtime/scene/selection/core.h"

typedef struct {
  Scene *scene;
  SceneSelection *selection;
  Renderer *renderer;
} SelectionSystemCallbackData;

typedef struct {
  vec3 *delta;
  const Axis axis;
  const GizmoMode transform_mode;
  SceneSelectionObjectList *selection;
  Scene *scene;
  Renderer *renderer;
} SceneSelectionTransform;

typedef void (*selection_system_transform_callback)(SceneSelectionTransform *);

typedef void (*selection_system_highlight_callback)(SceneSelection *, Scene *,
                                                    Renderer *,
                                                    SceneSelectionObjectList *,
                                                    void *);

// TODO: Not sure about have a global var for it...
extern SelectionSystemCallbackData selection_system_event_payload;
extern MeshRefListArray selection_system_gizmo_raycast_list;

EXTERN_C_BEGIN

void selection_system_init(SceneSelection *, Scene *, Renderer *);

void selection_system_draw_callback(void *);

void selection_system_toggle_mesh(SceneSelection *, Scene *, Renderer *,
                                  Mesh *);

// === Highlight callback ===
void selection_system_callback_mesh_highlight(SceneSelection *, Scene *,
                                              Renderer *,
                                              SceneSelectionObjectList *,
                                              void *);

void selection_system_callback_sem_highlight(SceneSelection *, Scene *,
                                             Renderer *,
                                             SceneSelectionObjectList *,
                                             void *);

// === Transform callback ===
void selection_system_callback_mesh_transform(SceneSelectionTransform *);
void selection_system_callback_sem_transform(SceneSelectionTransform *);
void selection_system_callback_mesh_shadow_transform(SceneSelectionTransform *);

// TODO: move this function to a more appropriate space and rename it
// accordingly. (This process isn't exclusive to selection...)
void selection_system_callback_mesh_update_probe_uniform(
    Mesh *, ProbeReflectionGridList *, ProbeReflectionPlaneList *,
    UBOManager *);

// === Mouse event ===

void selection_system_init_mouse_events(SceneSelection *, Scene *, Renderer *);

// camera raycast callbacks
void selection_system_callback_raycast_mesh(CameraRaycastCallback *,
                                            const EmscriptenMouseEvent *,
                                            void *);

void selection_system_callback_raycast_gizmo_down(CameraRaycastCallback *,
                                                  const EmscriptenMouseEvent *,
                                                  void *);

void selection_system_callback_raycast_gizmo_hover(CameraRaycastCallback *,
                                                   const EmscriptenMouseEvent *,
                                                   void *);

// mouse event callback DELETEME ?
bool selection_system_callback_html_reset(int, const EmscriptenMouseEvent *,
                                          void *);

// === Key event ===

void selection_system_init_key_events(SceneSelection *, Scene *, Renderer *);

void selection_system_callback_key_sequence_select_all(KeyRecordSequence *,
                                                       void *);

void selection_system_callback_key_sequence_set_gizmo_mode(KeyRecordSequence *,
                                                           void *);

void selection_system_callback_key_sequence_transform(KeyRecordSequence *,
                                                      void *);

// === Gizmo ===
void selection_system_update_gizmo_pos_to_selection(Gizmo *, SceneSelection *,
                                                    UBOManager *);

void selection_system_update_gizmo_raycast_list(Gizmo *, const GizmoMode);

EXTERN_C_END
#endif
