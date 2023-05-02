#include "Application.h"
#include "UI.h"
#include "StringOperations.h"
#include "Localization.h"
#include <Windows.h>

namespace MikuMikuWorld
{
	void Application::settingsDialog()
	{
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(750, 600), ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 10));
		if (ImGui::BeginPopupModal(MODAL_TITLE("settings"), NULL, ImGuiWindowFlags_NoResize))
		{
			ImVec2 padding = ImGui::GetStyle().WindowPadding;
			ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
			ImVec2 confirmBtnPos = ImGui::GetWindowSize() + ImVec2(-100, -UI::btnNormal.y) - padding;
			ImGui::BeginChild("##settings_panel", ImGui::GetContentRegionAvail() - ImVec2(0, UI::btnNormal.y + padding.y));

			if (ImGui::BeginTabBar("##settings_tabs"))
			{
				if (ImGui::BeginTabItem(getString("general")))
				{
					if (ImGui::CollapsingHeader(getString("auto_save"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						//autoSave.settings();
					}

					// theme
					if (ImGui::CollapsingHeader(getString("base_theme"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

						UI::beginPropertyColumns();
						UI::propertyLabel(getString("base_theme"));
						if (ImGui::BeginCombo("##color_theme", UI::baseThemeToStr(
							imgui->getBaseTheme()
						).c_str()))
						{
							for (int i = 0; i < 2; ++i)
							{
								const BaseTheme theme = UI::intToBaseTheme(i);
								const bool selected = (int)imgui->getBaseTheme() == i;
								if (ImGui::Selectable(UI::baseThemeToStr(theme).c_str(), selected)) {
									imgui->setBaseTheme(UI::intToBaseTheme(i));
									imgui->applyAccentColor(config.accentColor);
								}
							}

							ImGui::EndCombo();
						}
						UI::endPropertyColumns();
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
					}

					// accent color
					if (ImGui::CollapsingHeader(getString("accent_color"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

						ImGui::TextWrapped(getString("accent_color_help"));
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x + 3, 15));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);

						for (int i = 0; i < UI::accentColors.size(); ++i)
						{
							bool apply = false;
							std::string id = i == imgui->getAccentColor() ? ICON_FA_CHECK : i == 0 ? "C" : "##" + std::to_string(i);
							ImGui::PushStyleColor(ImGuiCol_Button, UI::accentColors[i].color);
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UI::accentColors[i].color);
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, UI::accentColors[i].color);

							apply = ImGui::Button(id.c_str(), UI::btnNormal);

							ImGui::PopStyleColor(3);

							if ((i < UI::accentColors.size() - 1) && ImGui::GetCursorPosX() < ImGui::GetWindowSize().x - UI::btnNormal.x - 50.0f)
								ImGui::SameLine();

							if (apply)
								imgui->applyAccentColor(i);
						}
						ImGui::PopStyleVar(2);

						ImVec4& customColor = UI::accentColors[0].color;
						float col[]{ customColor.x, customColor.y, customColor.z };
						static ColorDisplay displayMode = ColorDisplay::HEX;

						ImGui::Separator();
						ImGui::Text(getString("select_accent_color"));
						UI::beginPropertyColumns();
						UI::propertyLabel(getString("display_mode"));
						if (ImGui::BeginCombo("##color_display_mode", colorDisplayStr[(int)displayMode]))
						{
							for (int i = 0; i < 3; ++i)
							{
								const bool selected = (int)displayMode == i;
								if (ImGui::Selectable(colorDisplayStr[i], selected))
									displayMode = (ColorDisplay)i;
							}

							ImGui::EndCombo();
						}
						ImGui::NextColumn();
						UI::propertyLabel(getString("custom_color"));

						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x + 3, 15));
						ImGuiColorEditFlags flags = 1 << (20 + (int)displayMode);
						if (ImGui::ColorEdit3("##custom_accent_color", col, flags))
						{
							customColor.x = col[0];
							customColor.y = col[1];
							customColor.z = col[2];
						}

						UI::endPropertyColumns();

						if (ImGui::IsItemDeactivated() && imgui->getAccentColor() == 0)
							imgui->applyAccentColor(0);

						ImGui::PopStyleVar();
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
					}

					// graphics
					if (ImGui::CollapsingHeader(getString("video"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
						if (ImGui::Checkbox(getString("vsync_enable"), &windowState.vsync))
							glfwSwapInterval((int)windowState.vsync);

						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(getString("timeline")))
				{
					// timeline
					if (ImGui::CollapsingHeader(getString("timeline"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

						//int laneWidth = editor->canvas.getLaneWidth();
						//int notesHeight = editor->canvas.getNotesHeight();
						//bool smoothScrolling = editor->canvas.isUseSmoothScrolling();
						//float smoothScrollingTime = editor->canvas.getSmoothScrollingTime();

						//UI::beginPropertyColumns();
						//UI::addSliderProperty(getString("lane_width"), laneWidth, MIN_LANE_WIDTH, MAX_LANE_WIDTH, "%d");
						//UI::addSliderProperty(getString("notes_height"), notesHeight, MIN_NOTES_HEIGHT, MAX_NOTES_HEIGHT, "%d");

						//UI::addCheckboxProperty(getString("use_smooth_scroll"), smoothScrolling);
						//UI::addSliderProperty(getString("smooth_scroll_time"), smoothScrollingTime, 10.0f, 150.0f, "%.2fms");
						//UI::endPropertyColumns();

						//if (laneWidth != editor->canvas.getLaneWidth())
						//	editor->canvas.setLaneWidth(laneWidth);

						//if (notesHeight != editor->canvas.getNotesHeight())
						//	editor->canvas.setNotesHeight(notesHeight);

						//if (smoothScrolling != editor->canvas.isUseSmoothScrolling())
						//	editor->canvas.setUseSmoothScrolling(smoothScrolling);

						//if (smoothScrollingTime != editor->canvas.getSmoothScrollingTime())
						//	editor->canvas.setSmoothScrollingTime(smoothScrollingTime);
					}

					// background
					if (ImGui::CollapsingHeader(getString("background"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

						//float backgroundBrightness = editor->canvas.getBackgroundBrightness();
						//float laneOpacity = editor->canvas.getLaneOpacity();

						//UI::beginPropertyColumns();
						//UI::addPercentSliderProperty(getString("background_brightnes"), backgroundBrightness);
						//UI::addPercentSliderProperty(getString("lanes_opacity"), laneOpacity);
						//UI::endPropertyColumns();

						//if (backgroundBrightness != editor->canvas.getBackgroundBrightness())
						//	editor->canvas.setBackgroundBrightness(backgroundBrightness);

						//if (laneOpacity != editor->canvas.getLaneOpacity())
						//	editor->canvas.setLaneOpacity(laneOpacity);
					}

					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();

			ImGui::EndChild();
			ImGui::SetCursorPos(confirmBtnPos);
			if (ImGui::Button("OK", ImVec2(100, UI::btnNormal.y - 5)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}

		ImGui::PopStyleVar();
	}
}