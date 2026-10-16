#include "tree.hpp"
#include "backend/logger.h"
#include "backend/registry.h"
#include "backend/renderer/core.h"
#include "backend/std_pipeline/core.h"
#include "backend/theme/core.h"
#include "imgui/imgui.h"
#include "runtime/gui/components/button_icon.hpp"
#include "runtime/gui/components/tree_item.hpp"
#include "runtime/gui/core.h"
#include "runtime/light/core.h"
#include "runtime/mesh/core.h"
#include "runtime/mesh/shader/core.h"
#include "runtime/probe/reflection/grid.h"
#include "runtime/probe/reflection/plane.h"
#include "runtime/scene/core.h"
#include "runtime/scene/selection/core.h"
#include "runtime/scene/selection/filter.h"
#include "runtime/systems/selection_system.h"
#include "runtime/systems/mesh_route_system.h"
#include "utils/name.h"

void UI::Tree::draw_mesh(Mesh *mesh, const size_t index) {

  int flag = TreeItemFlag_None;

  if (index % 2 == 0)
    flag |= TreeItemFlag_AltBg;

  if (mesh->children.count)
    flag |= TreeItemFlag_HasChild;

  UI::TreeItemMesh item =
      UI::TreeItemMesh(gui, mesh->name, mesh->id, ThemeIcon_Mesh,
                       theme_size(gui->theme, ThemeSize_Button_InspectorTab),
                       (TreeItemFlag)flag);

  bool draw_label = item.draw();

  if (item.clicked) {
    selection_system_toggle_mesh(&scene->selection, scene, renderer, mesh);
    item.close_click();
  }

  item.draw_visibility();
  if (ImGui::IsItemClicked()) {

    RendererStatus mesh_state =
        mesh_route_system_toggle_mesh(scene, renderer, mesh);

    const RenderPipelineType pipeline = std_render_pipeline_type(
        (*mesh_shader(mesh, MeshShader_Texture)->pipeline));

    const RendererMeshConfig *batch =
        renderer_mesh_config_from_pipeline(pipeline);

    const SceneSelectionType selection_type =
        (batch->flags & RendererMeshFlag_Shadow)
            ? SceneSelectionType_MeshShadow
            : SceneSelectionType_Mesh;

    if (RendererStatus_MeshVisible == mesh_state)
      scene_selection_register_mesh(&scene->selection, mesh, mesh->id,
                                    selection_type);
    else if (RendererStatus_MeshHidden == mesh_state)
      scene_selection_unregister_mesh(&scene->selection, mesh, selection_type);
  }

  if (draw_label) {
    for (size_t i = 0; i < mesh->children.count; i++)
      draw_mesh(mesh->children.entries[i], i);
    ImGui::TreePop();
  }
}

static const ThemeIcon sem_icon_map[THEME_ICON_COUNT] = {
    [RegEntryType_SceneEditorMeshList_AmbientLight] = ThemeIcon_AmbientLight,
    [RegEntryType_SceneEditorMeshList_PointLight] = ThemeIcon_PointLight,
    [RegEntryType_SceneEditorMeshList_PointLightShadow] = ThemeIcon_PointLight,
    [RegEntryType_SceneEditorMeshList_SunLight] = ThemeIcon_SunLight,
    [RegEntryType_SceneEditorMeshList_SunLightShadow] = ThemeIcon_SunLight,
    [RegEntryType_SceneEditorMeshList_SpotLight] = ThemeIcon_SpotLight,
    [RegEntryType_SceneEditorMeshList_SpotLightShadow] = ThemeIcon_SpotLight,
    [RegEntryType_SceneEditorMeshList_ProbeReflectionGrid] =
        ThemeIcon_ProbeReflectionGrid,
    [RegEntryType_SceneEditorMeshList_ProbeReflectionPlane] =
        ThemeIcon_ProbeReflectionPlane,
};

void UI::Tree::draw_mesh_list(SceneEditorMeshList *list,
                              const RegEntryType type, const size_t index) {

  // define icon based on SEM type
  ThemeIcon icon = sem_icon_map[type];

  UI::TreeItemMesh item =
      UI::TreeItemMesh(gui, list->name, list->id, icon,
                       theme_size(gui->theme, ThemeSize_Button_InspectorTab),
                       index % 2 == 0 ? TreeItemFlag_AltBg : TreeItemFlag_None);

  if (item.draw()) {
    ImGui::TreePop();
  }

  if (item.clicked) {
    selection_system_toggle_mesh(&scene->selection, scene, renderer,
                                 list->entries->mesh);
    item.close_click();
  }

  item.draw_visibility();

  if (ImGui::IsItemClicked())
    for (size_t i = 0; i < list->count; i++)
      selection_system_toggle_mesh(&scene->selection, scene, renderer,
                                   list->entries[i].mesh);
}

void UI::Tree::draw() {

  ImGui::BeginChild("Tree", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.3f),
                    true);
  ImGui::Text("Scene Tree");

  ImGui::BeginChild("Tree items", ImVec2(0, 0), true);
  {
    size_t index = 0;
    for (size_t i = 0; i < gui->tree.count; i++) {

      const RegEntry *entry = reg_lookup(gui->tree.entries[i]);

      switch (entry->type) {

      case RegEntryType_Mesh:
        draw_mesh((Mesh *)entry->ptr, index++);
        break;

      case RegEntryType_SceneEditorMeshList_AmbientLight:
      case RegEntryType_SceneEditorMeshList_PointLight:
      case RegEntryType_SceneEditorMeshList_SunLight:
      case RegEntryType_SceneEditorMeshList_SpotLight:
      case RegEntryType_SceneEditorMeshList_PointLightShadow:
      case RegEntryType_SceneEditorMeshList_SunLightShadow:
      case RegEntryType_SceneEditorMeshList_SpotLightShadow:
      case RegEntryType_SceneEditorMeshList_ProbeReflectionGrid:
      case RegEntryType_SceneEditorMeshList_ProbeReflectionPlane:
        draw_mesh_list((SceneEditorMeshList *)entry->ptr, entry->type, index++);
        break;

      default:
        // unhandeld type
        break;
      }
    }
  }
  ImGui::EndChild();
  ImGui::EndChild();
}
