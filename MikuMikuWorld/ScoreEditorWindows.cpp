#include "ScoreEditorWindows.h"
#include "UI.h"
#include "FileDialog.h"
#include "Constants.h"
#include "Utilities.h"
#include "Application.h"
#include "ApplicationConfiguration.h"

namespace MikuMikuWorld
{
	void ScorePropertiesWindow::update(ScoreContext& context)
	{
		if (ImGui::CollapsingHeader(concat(ICON_FA_ALIGN_LEFT, getString("metadata"), " ").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::beginPropertyColumns();
			UI::addStringProperty(getString("title"), context.workingData.title);
			UI::addStringProperty(getString("designer"), context.workingData.designer);
			UI::addStringProperty(getString("artist"), context.workingData.artist);

			std::string jacketFile = context.workingData.jacket.getFilename();
			int result = UI::addFileProperty(getString("jacket"), jacketFile);
			if (result == 1)
			{
				context.workingData.jacket.load(jacketFile);
			}
			else if (result == 2)
			{
				std::string name;
				if (FileDialog::openFile(name, FileType::ImageFile))
					context.workingData.jacket.load(name);
			}
			context.workingData.jacket.draw();
			UI::endPropertyColumns();
		}

		if (ImGui::CollapsingHeader(concat(ICON_FA_VOLUME_UP, getString("audio"), " ").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::beginPropertyColumns();

			std::string filename = context.workingData.musicFilename;
			int filePickResult = UI::addFileProperty(getString("music_file"), filename);
			if (filePickResult == 1 && filename != context.workingData.musicFilename)
			{
				context.audio.changeBGM(filename);
			}
			else if (filePickResult == 2 && FileDialog::openFile(filename, FileType::AudioFile) && filename != context.workingData.musicFilename)
			{
				context.audio.changeBGM(filename);
			}

			float offset = context.workingData.musicOffset;
			UI::addFloatProperty(getString("music_offset"), offset, "%.03fms");
			if (offset != context.workingData.musicOffset)
			{
				context.workingData.musicOffset = offset;
				context.audio.setBGMOffset(0, offset);
			}

			// volume controls
			float master = context.audio.getMasterVolume();
			float bgm = context.audio.getBGMVolume();
			float se = context.audio.getSEVolume();

			UI::addPercentSliderProperty(getString("volume_master"), master);
			UI::addPercentSliderProperty(getString("volume_bgm"), bgm);
			UI::addPercentSliderProperty(getString("volume_se"), se);
			UI::endPropertyColumns();

			if (master != context.audio.getMasterVolume())
				context.audio.setMasterVolume(master);

			if (bgm != context.audio.getBGMVolume())
				context.audio.setBGMVolume(bgm);

			if (se != context.audio.getSEVolume())
				context.audio.setSEVolume(se);
		}

		if (ImGui::CollapsingHeader(concat(ICON_FA_CHART_BAR, getString("statistics"), " ").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::beginPropertyColumns();
			UI::addReadOnlyProperty(getString("taps"), context.scoreStats.getTaps());
			UI::addReadOnlyProperty(getString("flicks"), context.scoreStats.getFlicks());
			UI::addReadOnlyProperty(getString("holds"), context.scoreStats.getHolds());
			UI::addReadOnlyProperty(getString("steps"), context.scoreStats.getSteps());
			UI::addReadOnlyProperty(getString("total"), context.scoreStats.getTotal());
			UI::addReadOnlyProperty(getString("combo"), context.scoreStats.getCombo());
			UI::endPropertyColumns();
		}
	}

	void ScoreOptionsWindow::update(ScoreContext& context, EditArgs& edit, TimelineMode currentMode)
	{
		UI::beginPropertyColumns();
		//UI::addCheckboxProperty(getString("show_step_outlines"), drawHoldStepOutline);
		switch (currentMode)
		{
		case TimelineMode::InsertBPM:
			UI::addFloatProperty(getString("bpm"), edit.bpm, "%g BPM");
			edit.bpm = std::clamp(edit.bpm, MIN_BPM, MAX_BPM);
			break;

		case TimelineMode::InsertTimeSign:
			UI::timeSignatureSelect(edit.timeSignatureNumerator, edit.timeSignatureDenominator);
			break;

		case TimelineMode::InsertHiSpeed:
			UI::addFloatProperty(getString("hi_speed_speed"), edit.hiSpeed, "%gx");
			break;

		default:
			UI::addIntProperty(getString("note_width"), edit.noteWidth, MIN_NOTE_WIDTH, MAX_NOTE_WIDTH);
			UI::addSelectProperty(getString("step_type"), edit.stepType, stepTypes, TXT_ARR_SZ(stepTypes));
			UI::addSelectProperty(getString("ease_type"), edit.easeType, easeTypes, TXT_ARR_SZ(easeTypes));
			UI::addSelectProperty(getString("flick"), edit.flickType, flickTypes, TXT_ARR_SZ(flickTypes));
			break;
		}
		UI::endPropertyColumns();
	}

	void PresetsWindow::update(ScoreContext& context, PresetManager& presetManager)
	{
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
				if (!presetManager.presets.size())
				{
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
					ImGui::TextWrapped(getString("no_presets"));
					ImGui::PopStyleVar();
				}
				else
				{
					for (const auto& [id, preset] : presetManager.presets)
					{
						if (!presetFilter.PassFilter(preset.getName().c_str()))
							continue;

						ImGui::PushID(id);

						if (ImGui::Button(preset.getName().c_str(), ImVec2(ImGui::GetContentRegionAvail().x - UI::btnSmall.x - 2.0f, UI::btnSmall.y + 2.0f)))
							presetManager.applyPreset(id, context);

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

			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !context.selectedNotes.size());
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1 - (0.5f * !context.selectedNotes.size()));
			if (ImGui::Button(getString("create_preset"), ImVec2(-1, UI::btnSmall.y + 2.0f)))
				dialogOpen = true;

			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			ImGui::PopItemFlag();

			if (removePattern != -1)
				presetManager.removePreset(removePattern);
		}

		ImGui::End();

		if (dialogOpen)
		{
			ImGui::OpenPopup(MODAL_TITLE("create_preset"));
			dialogOpen = false;
		}

		if (updateCreationDialog() == DialogResult::Ok)
		{
			presetManager.createPreset(context.score, context.selectedNotes, presetName, presetDesc);
			presetName.clear();
			presetDesc.clear();
		}
	}

	DialogResult PresetsWindow::updateCreationDialog()
	{
		DialogResult result = DialogResult::None;

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
			ImGui::InputText("##preset_name", &presetName);

			ImGui::Text(getString("description"));
			ImGui::InputTextMultiline("##preset_desc", &presetDesc,
				{ -1, ImGui::GetContentRegionAvail().y - UI::btnSmall.y - 10.0f - padding.y });

			ImVec2 btnSz{ (ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) / 2.0f, UI::btnSmall.y + 2.0f };
			ImGui::SetCursorPos(ImVec2(xPos, yPos));
			if (ImGui::Button(getString("cancel"), btnSz))
			{
				result = DialogResult::Cancel;
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !presetName.size());
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1 - (0.5f * !presetName.size()));
			if (ImGui::Button(getString("confirm"), btnSz))
			{
				result = DialogResult::Ok;
				ImGui::CloseCurrentPopup();
			}

			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
			ImGui::EndPopup();
		}

		return result;
	}

	DialogResult UnsavedChangesDialog::update()
	{
		DialogResult result = DialogResult::None;
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
			float yPos = ImGui::GetWindowSize().y - UI::btnSmall.y - 2.0f - padding.y;
			ImGui::SetCursorPos(ImVec2(xPos, yPos));

			ImVec2 btnSz = ImVec2((ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) / 3.0f, UI::btnSmall.y + 2.0f);

			if (ImGui::Button(getString("save_changes"), btnSz))
			{
				close();
				result = DialogResult::Yes;
			}

			ImGui::SameLine();
			if (ImGui::Button(getString("discard_changes"), btnSz))
			{
				close();
				result = DialogResult::No;
			}

			ImGui::SameLine();
			if (ImGui::Button(getString("cancel"), btnSz))
			{
				close();
				result = DialogResult::Cancel;
			}

			ImGui::EndPopup();
		}

		return result;
	}

	DialogResult AboutDialog::update()
	{
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(450, 250), ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(MODAL_TITLE("about"), NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::Text("MikuMikuWorld\nCopyright (C) 2022 Crash5b\n\n");
			ImGui::Separator();

			ImGui::Text("Version %s", Application::getAppVersion().c_str());
			ImGui::SetCursorPos({
				ImGui::GetStyle().WindowPadding.x,
				ImGui::GetWindowSize().y - UI::btnSmall.y - ImGui::GetStyle().WindowPadding.y
			});

			if (ImGui::Button("OK", { ImGui::GetContentRegionAvail().x, UI::btnSmall.y }))
			{
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return DialogResult::Ok;
			}

			ImGui::EndPopup();
		}

		return DialogResult::None;
	}

	void SettingsWindow::updateKeyConfig(MultiInputBinding* bindings[], int count)
	{
		ImVec2 size = ImVec2(-1, ImGui::GetContentRegionAvail().y * 0.7);
		const ImGuiTableFlags tableFlags =
			ImGuiTableFlags_BordersOuter
			| ImGuiTableFlags_BordersInnerH
			| ImGuiTableFlags_ScrollY
			| ImGuiTableFlags_RowBg;

		const ImGuiSelectableFlags selectionFlags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
		int rowHeight = ImGui::GetFrameHeight() + 5;

		if (ImGui::BeginTable("##commands_table", 2, tableFlags, size))
		{
			ImGui::TableSetupColumn(getString("action"));
			ImGui::TableSetupColumn(getString("keys"));
			for (int i = 0; i < count; ++i)
			{
				ImGui::TableNextRow(0, rowHeight);

				ImGui::TableSetColumnIndex(0);
				ImGui::PushID(i);
				if (ImGui::Selectable(getString(bindings[i]->name), i == selectedBindingIndex, selectionFlags))
					selectedBindingIndex = i;

				ImGui::PopID();
				ImGui::TableSetColumnIndex(1);
				ImGui::Text(ToFullShortcutsString(*bindings[i]).c_str());
			}
			ImGui::EndTable();
		}
		ImGui::Separator();

		if (selectedBindingIndex > -1 && selectedBindingIndex < count)
		{
			int deleteBinding = -1;
			const bool canAdd = bindings[selectedBindingIndex]->count < 4;

			UI::beginPropertyColumns();
			ImGui::Text(getString(bindings[selectedBindingIndex]->name));
			ImGui::NextColumn();

			if (!canAdd)
				UI::beginNextItemDisabled();

			if (ImGui::Button(getString("add"), ImVec2(-1, UI::btnSmall.y)))
				bindings[selectedBindingIndex]->addBinding(InputBinding{});

			if (!canAdd)
				UI::endNextItemDisabled();

			UI::endPropertyColumns();

			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
			ImGui::BeginChild("##binding_keys_edit_window", ImVec2(-1, -1), true);
			for (int b = 0; b < bindings[selectedBindingIndex]->count; ++b)
			{
				ImGui::PushID(b);

				std::string buttonText;
				std::string cmdText = ToShortcutString(bindings[selectedBindingIndex]->bindings[b]);

				buttonText = listeningForInput && editBindingIndex == b ? getString("cmd_key_listen") : cmdText;
				if (ImGui::Button(buttonText.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 105, UI::btnSmall.y)))
				{
					listeningForInput = true;
					inputTimer.reset();
					editBindingIndex = b;
				}

				ImGui::SameLine();
				if (ImGui::Button(getString("remove"), ImVec2(100, UI::btnSmall.y)))
					deleteBinding = b;

				ImGui::PopID();
			}
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			if (deleteBinding > -1)
			{
				listeningForInput = false;
				bindings[selectedBindingIndex]->removeAt(deleteBinding);
			}
		}

		if (listeningForInput)
		{
			if (inputTimer.elapsed() >= inputTimeoutSeconds)
			{
				listeningForInput = false;
				editBindingIndex = -1;
			}
			else
			{
				for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_MouseLeft; ++key)
				{
					bool isCtrl = key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl || key == ImGuiKey_ModCtrl;
					bool isShift = key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift || key == ImGuiKey_ModShift;
					bool isAlt = key == ImGuiKey_LeftAlt || key == ImGuiKey_RightAlt || key == ImGuiKey_ModAlt;
					bool isSuper = key == ImGuiKey_LeftSuper || key == ImGuiKey_RightSuper || key == ImGuiKey_ModSuper;

					// execute if a non-modifier key is tapped
					if (ImGui::IsKeyPressed(key) && !isCtrl && !isShift && !isAlt && !isSuper)
					{
						bindings[selectedBindingIndex]->bindings[editBindingIndex] = InputBinding{ (uint16_t)key, (uint8_t)ImGui::GetIO().KeyMods };
						listeningForInput = false;
						editBindingIndex = -1;
					}
				}
			}
		}
	}

	DialogResult SettingsWindow::update()
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
						UI::beginPropertyColumns();
						UI::addCheckboxProperty(getString("auto_save_enable"), config.autoSaveEnabled);
						UI::addIntProperty(getString("auto_save_interval"), config.autoSaveInterval);
						UI::addIntProperty(getString("auto_save_count"), config.autoSaveMaxCount);
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("theme"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::addSelectProperty(getString("base_theme"), config.baseTheme, baseThemes, (int)BaseTheme::BASE_THEME_MAX);
						UI::endPropertyColumns();

						ImGui::TextWrapped(getString("accent_color_help"));
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x + 3, 15));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);

						for (int i = 0; i < UI::accentColors.size(); ++i)
						{
							bool apply = false;
							std::string id = i == config.accentColor ? ICON_FA_CHECK : i == 0 ? "C" : "##" + std::to_string(i);
							ImGui::PushStyleColor(ImGuiCol_Button, UI::accentColors[i]);
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UI::accentColors[i]);
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, UI::accentColors[i]);

							apply = ImGui::Button(id.c_str(), UI::btnNormal);

							ImGui::PopStyleColor(3);

							if ((i < UI::accentColors.size() - 1) && ImGui::GetCursorPosX() < ImGui::GetWindowSize().x - UI::btnNormal.x - 50.0f)
								ImGui::SameLine();

							if (apply)
								config.accentColor = i;
						}
						ImGui::PopStyleVar(2);

						ImVec4& customColor = UI::accentColors[0];
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
						ImGui::PopStyleVar();
					}

					if (ImGui::CollapsingHeader(getString("video"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						if (ImGui::Checkbox(getString("vsync_enable"), &Application::windowState.vsync))
							glfwSwapInterval((int)Application::windowState.vsync);
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(getString("timeline")))
				{
					if (ImGui::CollapsingHeader(getString("timeline"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::addSliderProperty(getString("lane_width"), config.timelineWidth, MIN_LANE_WIDTH, MAX_LANE_WIDTH, "%d");
						ImGui::Separator();

						UI::addCheckboxProperty(getString("use_smooth_scroll"), config.useSmoothScrolling);
						UI::addSliderProperty(getString("smooth_scroll_time"), config.smoothScrollingTime, 10.0f, 150.0f, "%.2fms");
						ImGui::Separator();

						UI::addCheckboxProperty("Cursor Auto Scroll While Playing", config.followCursorInPlayback);
						UI::addPercentSliderProperty("Cursor Auto Scroll Percentage From Timeline", config.cursorPositionThreshold);
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("background"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::addPercentSliderProperty(getString("background_brightnes"), config.backgroundBrightness);
						UI::addPercentSliderProperty(getString("lanes_opacity"), config.laneOpacity);
						UI::endPropertyColumns();
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(getString("key_config")))
				{
					updateKeyConfig(bindings, sizeof(bindings) / sizeof(MultiInputBinding*));
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
		return DialogResult::None;
	}
}
