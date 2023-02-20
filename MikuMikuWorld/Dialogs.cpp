#include "Application.h"
#include "UI.h"
#include "StringOperations.h"
#include "Localization.h"
#include <Windows.h>

namespace MikuMikuWorld
{
	std::string Application::getVersion()
	{
		wchar_t filename[1024];
		lstrcpyW(filename, mbToWideStr(std::string(appDir + "MikuMikuWorld.exe")).c_str());

		DWORD  verHandle	= 0;
		UINT   size			= 0;
		LPBYTE lpBuffer		= NULL;
		DWORD  verSize		= GetFileVersionInfoSizeW(filename, &verHandle);

		int major = 0, minor = 0, build = 0, rev = 0;
		if (verSize != NULL)
		{
			LPSTR verData = new char[verSize];

			if (GetFileVersionInfoW(filename, verHandle, verSize, verData))
			{
				if (VerQueryValue(verData, "\\", (VOID FAR * FAR*) & lpBuffer, &size))
				{
					if (size)
					{
						VS_FIXEDFILEINFO* verInfo = (VS_FIXEDFILEINFO*)lpBuffer;
						if (verInfo->dwSignature == 0xfeef04bd)
						{
							major = (verInfo->dwFileVersionMS >> 16) & 0xffff;
							minor = (verInfo->dwFileVersionMS >> 0) & 0xffff;
							rev = (verInfo->dwFileVersionLS >> 16) & 0xffff;
						}
					}
				}
			}
			delete[] verData;
		}

		return formatString("%d.%d.%d", major, minor, rev);
	}

	bool Application::warnUnsaved()
	{
		bool result = false;
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(450, 200), ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(MODAL_TITLE("unsaved_changes"), NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::Text(getString("ask_save"));
			ImGui::Text(getString("warn_unsaved"));

			ImVec2 padding = ImGui::GetStyle().WindowPadding;
			ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
			float xPos = padding.x;
			float yPos = ImGui::GetWindowSize().y - UI::btnNormal.y - padding.y;
			ImGui::SetCursorPos(ImVec2(xPos, yPos));

			ImVec2 btnSz = ImVec2((ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) / 3.0f, UI::btnNormal.y);

			if (ImGui::Button(getString("save_changes"), btnSz))
			{
				ImGui::CloseCurrentPopup();
				editor->save();
				result = true;
				windowState.unsavedOpen = false;
			}

			ImGui::SameLine();
			if (ImGui::Button(getString("discard_changes"), btnSz))
			{
				ImGui::CloseCurrentPopup();
				result = true;
				windowState.unsavedOpen = false;
			}

			ImGui::SameLine();
			if (ImGui::Button(getString("cancel"), btnSz))
			{
				ImGui::CloseCurrentPopup();
				resetting = windowState.closing = windowState.unsavedOpen = false;
				result = false;
			}

			ImGui::EndPopup();
		}

		return result;
	}

	void Application::about()
	{
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(MODAL_TITLE("about"), NULL, ImGuiWindowFlags_NoResize))
		{

			ImGui::Text("MikuMikuWorld\nCopyright (C) 2022 Crash5b\n\n");
			ImGui::Separator();

			ImGui::Text("Version %s", version.c_str());
			ImGui::SetCursorPos(ImVec2 {
				ImGui::GetStyle().WindowPadding.x,
				ImGui::GetWindowSize().y - UI::btnNormal.y - ImGui::GetStyle().WindowPadding.y
			});

			if (ImGui::Button("OK", ImVec2((ImGui::GetContentRegionAvail().x), UI::btnNormal.y)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}

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
						autoSave.settings();
					}

					// theme
					if (ImGui::CollapsingHeader(getString("accent_color"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

						ImGui::TextWrapped(getString("accent_color_help"));
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x + 3, 15));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);

						for (int i = 0; i < UI::accentColors.size(); ++i)
						{
							bool apply = false;
							std::string id = i == imgui.getAccentColor() ? ICON_FA_CHECK : i == 0 ? "C" : "##" + std::to_string(i);
							ImGui::PushStyleColor(ImGuiCol_Button, UI::accentColors[i].color);
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UI::accentColors[i].color);
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, UI::accentColors[i].color);

							apply = ImGui::Button(id.c_str(), UI::btnNormal);

							ImGui::PopStyleColor(3);

							if ((i < UI::accentColors.size() - 1) && ImGui::GetCursorPosX() < ImGui::GetWindowSize().x - UI::btnNormal.x - 50.0f)
								ImGui::SameLine();

							if (apply)
								imgui.applyAccentColor(i);
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

						if (ImGui::IsItemDeactivated() && imgui.getAccentColor() == 0)
							imgui.applyAccentColor(0);

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
					// charting
					if (ImGui::CollapsingHeader(getString("timeline"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

						int laneWidth = editor->canvas.getLaneWidth();
						int notesHeight = editor->canvas.getNotesHeight();
						bool smoothScrolling = editor->canvas.isUseSmoothScrolling();
						float smoothScrollingTime = editor->canvas.getSmoothScrollingTime();
						float backgroundBrightness = editor->canvas.getBackgroundBrightness();
						float laneOpacity = editor->canvas.getLaneOpacity();

						UI::beginPropertyColumns();
						UI::addSliderProperty(getString("lane_width"), laneWidth, MIN_LANE_WIDTH, MAX_LANE_WIDTH, "%d");
						UI::addSliderProperty(getString("notes_height"), notesHeight, MIN_NOTES_HEIGHT, MAX_NOTES_HEIGHT, "%d");

						ImGui::Checkbox(getString("use_smooth_scroll"), &smoothScrolling);
						ImGui::NextColumn();
						ImGui::NextColumn();
						UI::addSliderProperty(getString("smooth_scroll_time"), smoothScrollingTime, 10.0f, 150.0f, "%.2fms");
						UI::endPropertyColumns();

						ImGui::Separator();

						UI::beginPropertyColumns();
						UI::addPercentSliderProperty(getString("background_brightnes"), backgroundBrightness);
						UI::addPercentSliderProperty(getString("lanes_opacity"), laneOpacity);
						UI::endPropertyColumns();

						if (laneWidth != editor->canvas.getLaneWidth())
							editor->canvas.setLaneWidth(laneWidth);

						if (notesHeight != editor->canvas.getNotesHeight())
							editor->canvas.setNotesHeight(notesHeight);

						if (smoothScrolling != editor->canvas.isUseSmoothScrolling())
							editor->canvas.setUseSmoothScrolling(smoothScrolling);

						if (smoothScrollingTime != editor->canvas.getSmoothScrollingTime())
							editor->canvas.setSmoothScrollingTime(smoothScrollingTime);
						
						if (backgroundBrightness != editor->canvas.getBackgroundBrightness())
							editor->canvas.setBackgroundBrightness(backgroundBrightness);

						if (laneOpacity != editor->canvas.getLaneOpacity())
							editor->canvas.setLaneOpacity(laneOpacity);

						ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(getString("key_config")))
				{
					commandManager.inputSettings();
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