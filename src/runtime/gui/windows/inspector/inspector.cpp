#include "inspector.hpp"
#include "backend/context.h"
#include "backend/profiler.h"
#include "backend/registry.h"
#include "backend/stat.h"
#include "backend/shaders/core.h"
#include "imgui/imgui.h"
#include "runtime/gui/components/button_icon.hpp"
#include "runtime/gui/components/input.hpp"
#include "runtime/gui/components/spacing.hpp"
#include "runtime/gui/components/time_bar.hpp"
#include "runtime/gui/components/tree_item.hpp"
#include "runtime/gui/core.h"
#include "runtime/gui/windows/inspector/inspector.ambient_light.hpp"
#include "runtime/gui/windows/inspector/inspector.mesh.hpp"
#include "runtime/gui/windows/inspector/inspector.point_light.hpp"
#include "runtime/gui/windows/inspector/inspector.probe_reflection_plane.hpp"
#include "runtime/gui/windows/inspector/inspector.spot_light.hpp"
#include "runtime/gui/windows/inspector/inspector.sun_light.hpp"
#include "runtime/light/core.h"
#include "runtime/mesh/core.h"
#include "runtime/scene/core.h"
#include "webgpu/webgpu.h"
#include <cstdio>
#include <cstdlib>

int UI::Inspector::active_tab = 0;

void UI::Inspector::draw() {

  const float page_width = theme_size(gui->theme, ThemeSize_RightPanel_Width);
  const float bar_width = theme_size(gui->theme, ThemeSize_RightPanelTab_Width);
  const float gap = gui_scale(gui, 4.f);
  const float btn_size = theme_size(gui->theme, ThemeSize_Button_InspectorTab);
  const ImVec2 icon_size{btn_size, btn_size};

  ImGui::BeginChild("##Properties", ImVec2(0, 0), true);
  {
    ImGui::Separator();
    ImGui::Text("%s", label);
    ImGui::Spacing();
    {
      ImGui::BeginChild("##Properties Content", ImVec2(0, 0), true);
      {
        // tighten horizontal spacing
        ImGuiStyle &style = ImGui::GetStyle();
        float savedSpacingX = style.ItemSpacing.x;
        style.ItemSpacing.x = gap;

        // always-visible icon strip
        ImGui::BeginChild("##icon_bar", ImVec2(bar_width, 0), false,
                          ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse);
        {
          for (int i = 0; i < tab_count; ++i) {

            ImGui::PushID(i);
            bool sel = (active_tab == i);

            // highlight selected tab
            if (sel) {
              ImGui::PushStyleColor(
                  ImGuiCol_Button,
                  (ImVec4 &)*theme_color(gui->theme,
                                         ThemeColor_Background_Brand_Base));

              ImGui::PushStyleColor(
                  ImGuiCol_ButtonHovered,
                  (ImVec4 &)*theme_color(gui->theme,
                                         ThemeColor_Background_Brand_Base));
            }

            bool pressed =
                ButtonIcon(gui, tabs[i]->icon, tabs[i]->tooltip, icon_size)
                    .draw();

            if (pressed)
              active_tab = i;

            if (ImGui::IsItemHovered() && strlen(tabs[i]->tooltip))
              ImGui::SetTooltip("%s", tabs[i]->tooltip);

            if (sel)
              ImGui::PopStyleColor(2);

            ImGui::PopID();
          }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // draw content
        if (active_tab >= 0) {
          ImGui::BeginChild("##page",
                            ImVec2(page_width - 1.5f * bar_width - gap, 0),
                            true, ImGuiWindowFlags_NoScrollWithMouse);

          // header
          { ImGui::Text("%s", tabs[active_tab]->tooltip); }

          tabs[active_tab]->draw();

          ImGui::EndChild();
        }

        style.ItemSpacing.x = savedSpacingX;
      }
      ImGui::EndChild();
    }
    ImGui::EndChild();
  }
}
