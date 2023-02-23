#include "PresetManager.h"
#include "StringOperations.h"
#include "Application.h"
#include "File.h"
#include "Utilities.h"
#include "ImGui/imgui.h"
#include "Localization.h"
#include <tinyfiledialogs.h>
#include <fstream>
#include <filesystem>
#include <execution>
#include "UI.h"

using namespace nlohmann;

namespace MikuMikuWorld
{
	void PresetManager::loadPresets(const std::string& path)
	{
		std::wstring wPath = mbToWideStr(path);
		if (!std::filesystem::exists(wPath))
			return;

		std::vector<std::string> filenames;
		for (const auto& file : std::filesystem::directory_iterator(wPath))
		{
			if (file.path().extension().wstring() == L".json")
				filenames.push_back(wideStringToMb(file.path().wstring()));
		}

		std::mutex m2;
		presets.reserve(filenames.size());

		std::vector<Result> warnings;
		std::vector<Result> errors;

		std::for_each(std::execution::par, filenames.begin(), filenames.end(), [this, &warnings, &errors, &m2](const auto& filename) {
			std::wstring wFilename = mbToWideStr(filename);
			std::ifstream presetFile(wFilename);

			json presetJson;
			presetFile >> presetJson;
			presetFile.close();
			int id = nextPresetID++;

			NotesPreset preset(id, "");
			Result result = preset.read(presetJson, filename);
			{
				std::lock_guard<std::mutex> lock{ m2 };
				if (preset.notes.size())
					presets.emplace(id, std::move(preset));

				if (result.getStatus() == ResultStatus::Warning)
					warnings.push_back(result);
				else if (result.getStatus() == ResultStatus::Error)
					errors.push_back(result);
			}
		});

		if (errors.size())
		{
			std::string message;
			for (auto& error : errors)
				message += "- " + error.getMessage() + "\n";

			tinyfd_messageBox(APP_NAME, message.c_str(), "ok", "error", 1);
		}

		if (warnings.size())
		{
			std::string message;
			for (auto& warning : warnings)
				message += "- " + warning.getMessage() + "\n";

			tinyfd_messageBox(APP_NAME, message.c_str(), "ok", "warning", 1);
		}
	}

	void PresetManager::savePresets(const std::string& path)
	{
		std::wstring wPath = mbToWideStr(path);
		if (!std::filesystem::exists(wPath))
			std::filesystem::create_directory(wPath);

		for (const std::string& filename : deletePresets)
		{
			std::wstring wFullPath = mbToWideStr(path + filename) + L".json";
			if (std::filesystem::exists(wFullPath))
				std::filesystem::remove(wFullPath);
		}

		std::for_each(std::execution::par, createPresets.begin(), createPresets.end(), [this, &path](int id) {
			if (presets.find(id) != presets.end())
				writePreset(presets.at(id), path, false);
		});
	}

	void PresetManager::writePreset(NotesPreset& preset, const std::string& path, bool overwrite)
	{
		std::string filename = path + fixFilename(preset.getName()) + ".json";
		std::wstring wFilename = mbToWideStr(filename);

		if (!overwrite)
		{
			int count = 1;
			std::wstring suffix = L"";
			while (std::filesystem::exists(wFilename + suffix))
				suffix = L"(" + std::to_wstring(count++) + L")";

			wFilename += suffix;
		}

		std::ofstream presetFile(wFilename);

		json presetJson = preset.write();
		presetFile << std::setw(2) << presetJson;
		presetFile.flush();
		presetFile.close();
	}

	void PresetManager::normalizeTicks(NotesPreset& preset)
	{
		int leastTick = preset.notes.begin()->second.tick;
		for (auto& [id, note] : preset.notes)
		{
			if (note.tick < leastTick)
				leastTick = note.tick;
		}

		for (auto& [id, note] : preset.notes)
			note.tick -= leastTick;
	}

	void PresetManager::createPreset(const Score& score, const std::unordered_set<int>& selectedNotes,
		const std::string &name, const std::string& desc)
	{
		if (!selectedNotes.size() || !name.size())
			return;

		NotesPreset preset(nextPresetID++, name);
		preset.name = name;
		preset.description = desc;

		// copy notes
		int noteID = 0;
		std::unordered_set<int> selectHolds;
		for (int id : selectedNotes)
		{
			Note note = score.notes.at(id);
			switch (note.getType())
			{
			case NoteType::Tap:
				note.ID = noteID++;
				preset.notes[note.ID] = note;
				break;

			case NoteType::Hold:
				selectHolds.insert(note.ID);
				break;

			case NoteType::HoldMid:
			case NoteType::HoldEnd:
				selectHolds.insert(note.parentID);
				break;

			default:
				break;
			}
		}

		for (int id : selectHolds)
		{
			HoldNote hold = score.holdNotes.at(id);
			HoldNote newHold = hold;
			Note start = score.notes.at(hold.start.ID);
			Note end = score.notes.at(hold.end);

			start.ID = noteID++;
			end.ID = noteID++;
			end.parentID = start.ID;
			preset.notes[start.ID] = start;
			preset.notes[end.ID] = end;

			newHold.start.ID = start.ID;
			newHold.end = end.ID;
			newHold.steps.clear();

			for (const auto& step : hold.steps)
			{
				Note mid = score.notes.at(step.ID);
				mid.ID = noteID++;
				mid.parentID = start.ID;
				preset.notes[mid.ID] = mid;

				HoldStep newStep = step;
				newStep.ID = mid.ID;
				newHold.steps.push_back(newStep);
			}

			preset.holds[start.ID] = newHold;
		}

		normalizeTicks(preset);
		presets[preset.getID()] = preset;
		createPresets.insert(preset.getID());
	}

	void PresetManager::removePreset(int id)
	{
		if (presets.find(id) == presets.end())
			return;

		const auto& preset = presets.at(id);
		if (preset.getFilename().size())
			deletePresets.insert(preset.getFilename());

		presets.erase(id);
	}

	std::string PresetManager::fixFilename(const std::string& name)
	{
		std::string result = name;
		int length = strlen(invalidFilenameChars);
		for (auto& c : result)
		{
			for (int i = 0; i < length; ++i)
			{
				if (c == invalidFilenameChars[i])
				{
					c = '_';
					break;
				}
			}
		}

		return result;
	}

	const NotesPreset& PresetManager::getSelected() const
	{
		return selectedPreset;
	}

	bool PresetManager::updateWindow(const Score& score, const std::unordered_set<int>& selection)
	{
		bool selected = false;
		static bool dialogOpen = false;
		static std::string presetName = "";
		static std::string presetDesc = "";
		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_DRAFTING_COMPASS, "presets")))
		{
			int removePattern = -1;

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, ImGui::GetStyle().ItemSpacing.y));
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			float filterWidth = ImGui::GetContentRegionAvail().x - UI::btnSmall.x - 2;

			presetFilter.Draw("##preset_filter", concat(ICON_FA_SEARCH, getString("search"), " ").c_str(), filterWidth);
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_TIMES, UI::btnSmall))
				presetFilter.Clear();

			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			float windowHeight = ImGui::GetContentRegionAvail().y - ((ImGui::GetFrameHeight()) + 10.0f);
			if (ImGui::BeginChild("presets_child_window", ImVec2(-1, windowHeight), true))
			{
				if (!presets.size())
				{
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
					Utilities::ImGuiCenteredText(getString("no_presets"));
					ImGui::PopStyleVar();
				}
				else
				{
					for (const auto& [id, preset] : presets)
					{
						if (!presetFilter.PassFilter(preset.getName().c_str()))
							continue;

						std::string strID = std::to_string(id) + "_" + preset.getName();
						ImGui::PushID(strID.c_str());
						if (ImGui::Button(preset.getName().c_str(), ImVec2(ImGui::GetContentRegionAvail().x - UI::btnSmall.x - 2.0f, UI::btnSmall.y + 2.0f)))
						{
							selectedPreset = preset;
							selected = true;
						}

						if (preset.description.size())
							UI::tooltip(preset.description.c_str());

						ImGui::SameLine();
						if (UI::transparentButton(ICON_FA_TRASH, ImVec2(UI::btnSmall.x, UI::btnSmall.y + 2.0f)))
							removePattern = id;

						ImGui::PopID();
					}
				}
			}
			ImGui::EndChild();
			ImGui::Separator();

			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !selection.size());
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1 - (0.5f * !selection.size()));
			if (ImGui::Button(getString("create_preset"), ImVec2(-1, UI::btnSmall.y + 2.0f)))
				dialogOpen = true;

			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			ImGui::PopItemFlag();

			if (removePattern != -1)
				removePreset(removePattern);
		}

		ImGui::End();

		if (dialogOpen)
		{
			ImGui::OpenPopup(MODAL_TITLE("create_preset"));
			dialogOpen = false;
		}

		if (updatePresetCreationDialog(presetName, presetDesc))
		{
			createPreset(score, selection, presetName, presetDesc);
			presetName.clear();
			presetDesc.clear();
		}

		return selected;
	}

	bool PresetManager::updatePresetCreationDialog(std::string& name, std::string& description)
	{
		bool result = false;

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(MODAL_TITLE("create_preset"), NULL, ImGuiWindowFlags_NoResize))
		{
			ImVec2 padding = ImGui::GetStyle().WindowPadding;
			ImVec2 spacing = ImGui::GetStyle().ItemSpacing;

			float xPos = padding.x;
			float yPos = ImGui::GetWindowSize().y - UI::btnSmall.y - 2.0f - (padding.y * 2);
			
			ImGui::Text(getString("name"));
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##preset_name", &name);

			ImGui::Text(getString("description"));
			ImGui::InputTextMultiline("##preset_desc", &description,
				{ -1, ImGui::GetContentRegionAvail().y - UI::btnSmall.y - 10.0f - padding.y});

			ImVec2 btnSz{ (ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) / 2.0f, UI::btnSmall.y + 2.0f };
			ImGui::SetCursorPos(ImVec2(xPos, yPos));
			if (ImGui::Button(getString("cancel"), btnSz))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !name.size());
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1 - (0.5f * !name.size()));
			if (ImGui::Button(getString("confirm"), btnSz))
			{
				result = true;
				ImGui::CloseCurrentPopup();
			}

			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
			ImGui::EndPopup();
		}

		return result;
	}
}