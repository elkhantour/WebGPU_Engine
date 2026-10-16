#include "inspector.object.hpp"
#include "imgui/imgui.h"
#include "runtime/gui/windows/inspector/inspector.ambient_light.hpp"
#include "runtime/gui/windows/inspector/inspector.mesh.hpp"
#include "runtime/gui/windows/inspector/inspector.point_light.hpp"
#include "runtime/gui/windows/inspector/inspector.probe_reflection_plane.hpp"
#include "runtime/gui/windows/inspector/inspector.spot_light.hpp"
#include "runtime/gui/windows/inspector/inspector.sun_light.hpp"
#include "runtime/scene/mesh_query.h"

RegEntry const *UI::ObjectTab::active_object = nullptr;

bool UI::ObjectTab::is_valid_type(const RegEntryType type) {

  for (uint8_t i = 0; i < valid_type_len; i++)
    if (valid_type[i] == type)
      return true;

  return false;
}

/**
   Return the registry ID of the selected mesh.
   If no meshes are selected, it returns the first mesh id of the dynamic
   pipelines.
 */
reg_id_t UI::ObjectTab::set_active_target() {

  if (scene_selection_count(&scene->selection)) {
    // get selection 1st entry
    for (int i = 0; i < SCENE_SELECTION_TYPE_COUNT; i++) {
      SceneSelectionObjectList *selection_list =
          &scene->selection.filters[i].selection;
      if (selection_list->count)
        return selection_list->entries[0].target;
    }
  }

  const MeshRefList *dynamic_meshes;
  scene_mesh_query_without_flags(scene, RendererMeshFlag_Fixed,
                                 &dynamic_meshes);

  for (size_t j = 0; j < dynamic_meshes->count; j++)
    return dynamic_meshes->entries[j]->id;

  // TODO Make fallback id more robust
  return REG_OWNER_UNDEFINED;
}

void UI::ObjectTab::draw() {

  ImGui::BeginChild("##ObjectTab", ImVec2(0, 0), true);
  {

    reg_id_t target_id = set_active_target();
    active_object = reg_lookup(target_id); // O(1) so cheap in hot loop

    switch (active_object->type) {

    case RegEntryType_Mesh:
      InspectorMesh(gui, "Mesh properties", (Mesh *)active_object->ptr).draw();
      break;

    case RegEntryType_SceneEditorMeshList_ProbeReflectionPlane:
      InspectorProbeReflectionPlane(gui, "Probe reflection plane properties",
                                    (SceneEditorMeshList *)active_object->ptr)
          .draw();
      break;

    case RegEntryType_SceneEditorMeshList_AmbientLight:
      InspectorAmbientLight(gui, "Light properties",
                            (SceneEditorMeshList *)active_object->ptr)
          .draw();
      break;

    case RegEntryType_SceneEditorMeshList_PointLight:
    case RegEntryType_SceneEditorMeshList_PointLightShadow:
      InspectorPointLight(gui, "Light properties",
                          (SceneEditorMeshList *)active_object->ptr,
                          active_object->type)
          .draw();
      break;

    case RegEntryType_SceneEditorMeshList_SpotLight:
    case RegEntryType_SceneEditorMeshList_SpotLightShadow:
      InspectorSpotLight(gui, "Light properties",
                         (SceneEditorMeshList *)active_object->ptr,
                         active_object->type)
          .draw();
      break;

    case RegEntryType_SceneEditorMeshList_SunLight:
    case RegEntryType_SceneEditorMeshList_SunLightShadow:
      InspectorSunLight(gui, "Light properties",
                        (SceneEditorMeshList *)active_object->ptr,
                        active_object->type)
          .draw();
      break;

    default:
      break;
    }
  }
  ImGui::EndChild();
}
