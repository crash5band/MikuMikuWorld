#include "Application.h"
#include "UI.h"
#include "StringOperations.h"
#include <Windows.h>

namespace MikuMikuWorld
{
	std::string Application::getVersion()
	{
		char filename[1024];
		strcpy_s(filename, std::string(appDir + "MikuMikuWorld.exe").c_str());

		DWORD  verHandle	= 0;
		UINT   size			= 0;
		LPBYTE lpBuffer		= NULL;
		DWORD  verSize		= GetFileVersionInfoSize(filename, &verHandle);

		int major = 0, minor = 0, build = 0, rev = 0;
		if (verSize != NULL)
		{
			LPSTR verData = new char[verSize];

			if (GetFileVersionInfo(filename, verHandle, verSize, verData))
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
		ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);

		ImVec2 padding = ImGui::GetStyle().WindowPadding;
		ImVec2 spacing = ImGui::GetStyle().ItemSpacing;

		bool result = false;
		if (ImGui::BeginPopupModal(unsavedModalTitle, NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::Text("Save changes to current file?");
			ImGui::Text("Any unsaved changes will be lost.");

			float xPos = padding.x;
			float yPos = ImGui::GetWindowSize().y - btnNormal.y - padding.y;
			ImGui::SetCursorPos(ImVec2(xPos, yPos));

			ImVec2 btnSz = ImVec2((ImGui::GetContentRegionAvail().x - spacing.x) / 3.0f, btnNormal.y);

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
			float yPos = ImGui::GetWindowSize().y - btnNormal.y - padding.y;
			ImGui::SetCursorPos(ImVec2(xPos, yPos));

			ImVec2 btnSz = ImVec2((ImGui::GetContentRegionAvail().x), btnNormal.y);

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
			ImVec2 confirmBtnPos = ImGui::GetWindowSize() + ImVec2(-100, -btnNormal.y) - padding;
			ImGui::BeginChild("##settings_panel", ImGui::GetContentRegionAvail() - ImVec2(0, btnNormal.y + padding.y), true);

			// theme
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x + 3, 15));
			if (ImGui::CollapsingHeader("Accent Color", ImGuiTreeNodeFlags_DefaultOpen))
			{
				for (int i = 0; i < accentColors.size(); ++i)
				{
					bool apply = false;
					std::string id = "##" + std::to_string(i);
					ImGui::PushStyleColor(ImGuiCol_Button, accentColors[i].color);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, accentColors[i].color);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, accentColors[i].color);

					apply = ImGui::Button(id.c_str(), btnNormal);

					ImGui::PopStyleColor(3);

					if (i < accentColors.size() - 1 && ImGui::GetCursorPosX() < ImGui::GetWindowSize().x - btnNormal.x - 50.0f)
						ImGui::SameLine();

					if (apply)
						applyAccentColor(i);
				}
			}
			ImGui::PopStyleVar();

			// charting
			if (ImGui::CollapsingHeader("Timeline", ImGuiTreeNodeFlags_DefaultOpen))
			{
				int laneWidth = editor->getLaneWidth();
				int notesHeight = editor->getNotesHeight();

				beginPropertyColumns();
				addSliderProperty("Lane Width", laneWidth, MIN_LANE_WIDTH, MAX_LANE_WIDTH, "%d");
				addSliderProperty("Notes Height", notesHeight, MIN_NOTES_HEIGHT, MAX_NOTES_HEIGHT, "%d");
				endPropertyColumns();

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
			if (ImGui::Button("OK", ImVec2(100, btnNormal.y - 5)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}
}