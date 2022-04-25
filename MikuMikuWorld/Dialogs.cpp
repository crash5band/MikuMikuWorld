#include "Application.h"
#include "UI.h"
#include "StringOperations.h"
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
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(450, 200), ImGuiCond_Always);

		ImVec2 padding = ImGui::GetStyle().WindowPadding;
		ImVec2 spacing = ImGui::GetStyle().ItemSpacing;

		bool result = false;
		if (ImGui::BeginPopupModal(unsavedModalTitle, NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::Text("Save changes to current file?");
			ImGui::Text("Any unsaved changes will be lost.");

			float xPos = padding.x;
			float yPos = ImGui::GetWindowSize().y - UI::btnNormal.y - padding.y;
			ImGui::SetCursorPos(ImVec2(xPos, yPos));

			ImVec2 btnSz = ImVec2((ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) / 3.0f, UI::btnNormal.y);

			if (ImGui::Button("Save Changes", btnSz))
			{
				ImGui::CloseCurrentPopup();
				editor->save();
				result = true;
				unsavedOpen = false;
			}

			ImGui::SameLine();
			if (ImGui::Button("Discard Changes", btnSz))
			{
				ImGui::CloseCurrentPopup();
				result = true;
				unsavedOpen = false;
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel", btnSz))
			{
				ImGui::CloseCurrentPopup();
				resetting = exiting = unsavedOpen = false;
				result = false;
			}

			ImGui::EndPopup();
		}

		return result;
	}

	void Application::about()
	{
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 250), ImGuiCond_Always);

		ImVec2 padding = ImGui::GetStyle().WindowPadding;
		ImVec2 spacing = ImGui::GetStyle().ItemSpacing;

		if (ImGui::BeginPopupModal(aboutModalTitle, NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::Text("MikuMikuWorld\nCopyright (C) 2022 Crash5b\n\n");
			ImGui::Separator();

			ImGui::Text("Version %s", version.c_str());

			float xPos = padding.x;
			float yPos = ImGui::GetWindowSize().y - UI::btnNormal.y - padding.y;
			ImGui::SetCursorPos(ImVec2(xPos, yPos));

			ImVec2 btnSz = ImVec2((ImGui::GetContentRegionAvail().x), UI::btnNormal.y);

			if (ImGui::Button("OK", btnSz))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}

	void Application::settings()
	{
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 460), ImGuiCond_Always);

		ImVec2 padding = ImGui::GetStyle().WindowPadding;
		ImVec2 spacing = ImGui::GetStyle().ItemSpacing;

		if (ImGui::BeginPopupModal(settingsModalTitle, NULL, ImGuiWindowFlags_NoResize))
		{
			ImVec2 confirmBtnPos = ImGui::GetWindowSize() + ImVec2(-100, -UI::btnNormal.y) - padding;
			ImGui::BeginChild("##settings_panel", ImGui::GetContentRegionAvail() - ImVec2(0, UI::btnNormal.y + padding.y), true);

			// theme
			if (ImGui::CollapsingHeader("Accent Color", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x + 3, 15));
				for (int i = 0; i < UI::accentColors.size(); ++i)
				{
					bool apply = false;
					std::string id = "##" + std::to_string(i);
					ImGui::PushStyleColor(ImGuiCol_Button, UI::accentColors[i].color);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UI::accentColors[i].color);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, UI::accentColors[i].color);

					apply = ImGui::Button(id.c_str(), UI::btnNormal);

					ImGui::PopStyleColor(3);

					if ((i < UI::accentColors.size() - 1) && ImGui::GetCursorPosX() < ImGui::GetWindowSize().x - UI::btnNormal.x - 50.0f)
						ImGui::SameLine();

					if (apply)
						applyAccentColor(i);
				}
				ImGui::PopStyleVar();

				ImVec4& customColor = UI::accentColors[0].color;
				float col[]{customColor.x, customColor.y, customColor.z};
				static ColorDisplay displayMode = ColorDisplay::HEX;

				UI::beginPropertyColumns();
				UI::propertyLabel("Display Mode");
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
				UI::propertyLabel("Custom Color");

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x + 3, 15));
				ImGuiColorEditFlags flags = 1 << (20 + (int)displayMode);
				if (ImGui::ColorEdit3("##custom_accent_color", col, flags))
				{
					customColor.x = col[0];
					customColor.y = col[1];
					customColor.z = col[2];
				}

				UI::endPropertyColumns();

				if (ImGui::IsItemDeactivated() && accentColor == 0)
					applyAccentColor(0);

				ImGui::PopStyleVar();
			}

			// charting
			if (ImGui::CollapsingHeader("Timeline", ImGuiTreeNodeFlags_DefaultOpen))
			{
				int laneWidth = editor->getLaneWidth();
				int notesHeight = editor->getNotesHeight();

				UI::beginPropertyColumns();
				UI::addSliderProperty("Lane Width", laneWidth, MIN_LANE_WIDTH, MAX_LANE_WIDTH, "%d");
				UI::addSliderProperty("Notes Height", notesHeight, MIN_NOTES_HEIGHT, MAX_NOTES_HEIGHT, "%d");
				UI::endPropertyColumns();

				if (laneWidth != editor->getLaneWidth())
					editor->setLaneWidth(laneWidth);

				if (notesHeight != editor->getNotesHeight())
					editor->setNotesHeight(notesHeight);
			}

			// graphics
			if (ImGui::CollapsingHeader("Video", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (ImGui::Checkbox("Enable VSync", &vsync))
					glfwSwapInterval((int)vsync);
			}

			ImGui::EndChild();
			ImGui::SetCursorPos(confirmBtnPos);
			if (ImGui::Button("OK", ImVec2(100, UI::btnNormal.y - 5)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}
}