#include "display.hpp"

#include "../imgui_style/style.carbon.hpp"
#include "backend/registry.h"
#include "backend/theme/core.h"
#include "imgui/imgui.h"
#include "runtime/gui/components/button_icon.hpp"
#include "runtime/gui/components/checkbox.hpp"
#include "runtime/scene/core.h"
#include "runtime/systems/scene_editor_mesh_system.h"
#include "runtime/systems/scene_system.h"
#include "runtime/systems/mesh_route_system.h"

int UI::Display::state = DisplayState_Activity | DisplayState_Grid |
                         DisplayState_Layout | DisplayState_Light |
                         DisplayState_Probe;

void UI::Display::checkbox_update_state(bool active,
                                        const DisplayState target) {

  if (active)
    state |= target;
  else
    state &= ~target;
}

void UI::Display::checkbox_on_change_base(Scene *scene, Renderer *renderer,
                                          bool active, void *user_data) {

  UI::DisplayState target = *(UI::DisplayState *)user_data;
  checkbox_update_state(active, target);
}

void UI::Display::checkbox_on_change_light(Scene *scene, Renderer *renderer,
                                           bool active, void *user_data) {

  UI::DisplayState target = *(UI::DisplayState *)user_data;
  checkbox_update_state(active, target);

  static const RegEntryType light_type[] = {
      RegEntryType_SceneEditorMeshList_AmbientLight,
      RegEntryType_SceneEditorMeshList_PointLight,
      RegEntryType_SceneEditorMeshList_SunLight,
      RegEntryType_SceneEditorMeshList_SpotLight,
      RegEntryType_SceneEditorMeshList_PointLightShadow,
      RegEntryType_SceneEditorMeshList_SunLightShadow,
      RegEntryType_SceneEditorMeshList_SpotLightShadow,
  };

  static const int light_type_count =
      sizeof(light_type) / sizeof(light_type[0]);

  sem_list_system_toggle_visibility(&scene->editor_meshes, scene, renderer,
                                    light_type, light_type_count, active);
}

void UI::Display::checkbox_on_change_probe(Scene *scene, Renderer *renderer,
                                           bool active, void *user_data) {

  UI::DisplayState target = *(UI::DisplayState *)user_data;
  checkbox_update_state(active, target);

  static const RegEntryType probe_type[] = {
      RegEntryType_SceneEditorMeshList_ProbeReflectionGrid,
      RegEntryType_SceneEditorMeshList_ProbeReflectionPlane,
  };

  static const int probe_type_count =
      sizeof(probe_type) / sizeof(probe_type[0]);

  sem_list_system_toggle_visibility(&scene->editor_meshes, scene, renderer,
                                    probe_type, probe_type_count, active);
}

void UI::Display::checkbox_on_change_grid(Scene *scene, Renderer *renderer,
                                          bool active, void *user_data) {

  UI::DisplayState target = *(UI::DisplayState *)user_data;
  checkbox_update_state(active, target);

  if (active)
    mesh_route_system_show_mesh(scene, renderer, scene->grid);
  else
    mesh_route_system_hide_mesh(scene, renderer, scene->grid);
}

void UI::Display::draw() {

  ImGui::SetNextWindowPos(
      ImVec2(theme_size(gui->theme, ThemeSize_Gizmo_Margin), 0),
      ImGuiCond_Always);
  ImGui::Begin("Display Frame", nullptr,
               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
                   ImGuiWindowFlags_NoBackground);

  if (ImGui::BeginCombo("##Display", "Display",
                        ImGuiComboFlags_WidthFitPreview)) {
    for (int i = 0; i < filter_count; i++)
      UI::Checkbox(gui, filters[i].label, (state & filters[i].target_state),
                   filters[i].icon, filters[i].on_change,
                   (void *)&filters[i].target_state)
          .draw();

    ImGui::EndCombo();
  }

  ImGui::End();
}
