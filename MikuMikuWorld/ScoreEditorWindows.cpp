#include "Application.h"
#include "ScoreEditorWindows.h"
#include "UI.h"
#include "File.h"
#include "Constants.h"
#include "Utilities.h"
#include "ApplicationConfiguration.h"
#include "NoteSkin.h"
#include "ResourceManager.h"

namespace MikuMikuWorld
{
	void ScorePropertiesWindow::statsTableRow(const char* lbl, size_t row)
	{
		ImGui::TableSetColumnIndex(0);

		int index = ResourceManager::getTexture("note_stats");
		if (isArrayIndexInBounds(index, ResourceManager::textures))
		{
			const Texture& tex = ResourceManager::textures[index];
			if (row < scoreStatsImages.size() && isArrayIndexInBounds(scoreStatsImages[row], tex.sprites))
			{
				const Sprite& spr = tex.sprites[scoreStatsImages[row]];
				ImVec2 uv0{ spr.getX1() / tex.getWidth(), spr.getY1() / tex.getHeight() };
				ImVec2 uv1{ spr.getX2() / tex.getWidth(), spr.getY2() / tex.getHeight() };
				ImGui::Image((ImTextureID)(size_t)tex.getID(), { 20, 20 }, uv0, uv1);
			}
			else
			{
				ImGui::Text(getString(lbl));
			}
		}
		else
		{
			ImGui::Text(getString(lbl));
		}

		ImGui::TableSetColumnIndex(1);
	}

	void ScorePropertiesWindow::update(ScoreContext& context)
	{
		if (ImGui::CollapsingHeader(IO::concat(ICON_FA_ALIGN_LEFT, getString("metadata"), " ").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::beginPropertyColumns();
			UI::addStringProperty(getString("title"), context.workingData.title);
			UI::addStringProperty(getString("designer"), context.workingData.designer);
			UI::addStringProperty(getString("artist"), context.workingData.artist);

			std::string jacketFile = context.workingData.jacket.getFilename();

			ImGui::BeginDisabled(isLoadingJacket);
			int result = UI::addFileProperty(getString("jacket"), isLoadingJacket ? loadingText : jacketFile);
			ImGui::EndDisabled();
			
			if (result == 1)
			{
				context.workingData.jacket.load(jacketFile);
			}
			else if (result == 2)
			{
				IO::FileDialog fileDialog{};
				fileDialog.title = "Open Image File";
				fileDialog.filters = { IO::imageFilter, IO::allFilter };
				fileDialog.parentWindowHandle = Application::windowState.windowHandle;

				if (fileDialog.openFile() == IO::FileDialogResult::OK)
					context.workingData.jacket.load(fileDialog.outputFilename);
			}
			 
			context.workingData.jacket.draw();
			UI::endPropertyColumns();
		}

		if (ImGui::CollapsingHeader(IO::concat(ICON_FA_VOLUME_UP, getString("audio"), " ").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::beginPropertyColumns();
			
			std::string musicFilename = context.workingData.musicFilename;

			ImGui::BeginDisabled(isLoadingMusic);
			int filePickResult = UI::addFileProperty(getString("music_file"), isLoadingMusic ? loadingText : musicFilename);
			ImGui::EndDisabled();

			if (filePickResult == 1 && musicFilename != context.workingData.musicFilename)
			{
				isPendingLoadMusic = true;
				pendingLoadMusicFilename = musicFilename;
			}
			else if (filePickResult == 2)
			{
				IO::FileDialog fileDialog{};
				fileDialog.title = "Open Audio File";
				fileDialog.filters = { IO::audioFilter, IO::allFilter };	
				fileDialog.parentWindowHandle = Application::windowState.windowHandle;

				if (fileDialog.openFile() == IO::FileDialogResult::OK)
				{
					pendingLoadMusicFilename = fileDialog.outputFilename;
					isPendingLoadMusic = true;
				}
			}

			float offset = context.workingData.musicOffset;
			UI::addDragFloatProperty(getString("music_offset"), offset, "%.3fms");
			if (offset != context.workingData.musicOffset)
			{
				context.workingData.musicOffset = offset;
				context.audio.setMusicOffset(context.getTimeAtCurrentTick(), offset);
			}

			// volume controls
			float master = context.audio.getMasterVolume();
			float bgm = context.audio.getMusicVolume();
			float se = context.audio.getSoundEffectsVolume();

			UI::addPercentSliderProperty(getString("volume_master"), master);
			UI::addPercentSliderProperty(getString("volume_bgm"), bgm);
			UI::addPercentSliderProperty(getString("volume_se"), se);
			UI::endPropertyColumns();

			if (master != context.audio.getMasterVolume())
				context.audio.setMasterVolume(master);

			if (bgm != context.audio.getMusicVolume())
				context.audio.setMusicVolume(bgm);

			if (se != context.audio.getSoundEffectsVolume())
				context.audio.setSoundEffectsVolume(se);
		}

		if (ImGui::CollapsingHeader(IO::concat(ICON_FA_CHART_BAR, getString("statistics"), " ").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			constexpr ImGuiTableFlags tableFlags
			{	ImGuiTableFlags_BordersInner |
				ImGuiTableFlags_ScrollY |
				ImGuiTableFlags_SizingStretchSame |
				ImGuiTableFlags_PadOuterX |
				ImGuiTableFlags_RowBg
			};

			float tableHeight{ std::min(ImGui::GetFrameHeight() * (scoreStatsImages.size() + 2), ImGui::GetContentRegionAvail().y) };
			if (ImGui::BeginTable("stats_table", 2, tableFlags, { 0, tableHeight }))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 10, 1 });

				ImGui::TableNextRow();
				statsTableRow("tap", 0);
				ImGui::Text("%d", context.scoreStats.getTaps());

				ImGui::TableNextRow();
				statsTableRow("flick", 1);
				ImGui::Text("%d", context.scoreStats.getFlicks());

				ImGui::TableNextRow();
				statsTableRow("hold", 2);
				ImGui::Text("%d", context.scoreStats.getHolds());

				ImGui::TableNextRow();
				statsTableRow("step", 3);
				ImGui::Text("%d", context.scoreStats.getSteps());

				ImGui::TableNextRow();
				statsTableRow("trace", 4);
				ImGui::Text("%d", context.scoreStats.getTraces());

				ImGui::TableNextRow();
				statsTableRow("total", 5);
				ImGui::Text("%d", context.scoreStats.getTotal());

				ImGui::TableNextRow();
				statsTableRow("combo", 6);
				ImGui::Text("%d", context.scoreStats.getCombo());

				ImGui::PopStyleVar();
				ImGui::EndTable();
			}
		}
	}

	void ScoreOptionsWindow::update(ScoreContext& context, EditArgs& edit, TimelineMode currentMode)
	{
		UI::beginPropertyColumns();
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
			UI::addSelectProperty(getString("step_type"), edit.stepType, stepTypes, arrayLength(stepTypes));
			UI::addSelectProperty(getString("ease_type"), edit.easeType, easeTypes, arrayLength(easeTypes));
			UI::addSelectProperty<FlickType>(getString("flick"), edit.flickType, flickTypes, arrayLength(flickTypes));
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

			presetFilter.Draw("##preset_filter", IO::concat(ICON_FA_SEARCH, getString("search"), " ").c_str(), filterWidth);
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_TIMES, UI::btnSmall))
				presetFilter.Clear();

			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			const int deletePresetId{presetManager.deletedPreset.getID()};
			if (deletePresetId > -1)
			{
				const float itemSpacingX = ImGui::GetStyle().ItemSpacing.x * 2;
				float availableWidth = ImGui::GetContentRegionAvail().x + (UI::btnSmall.x * 2) + itemSpacingX;
				ImGui::SetNextItemWidth(availableWidth);
				ImGui::Text(getString("preset_deleted"));
				ImGui::SameLine();

				// Shift undo and close to the right end of the window
				ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (UI::btnSmall.x * 2) - itemSpacingX - ImGui::GetStyle().WindowPadding.x);
				if (UI::transparentButton(ICON_FA_UNDO, UI::btnSmall, false))
					presetManager.undoDeletePreset();

				UI::tooltip(getString("undo"));
				ImGui::SameLine();

				// Conflict with preset filter clear button
				ImGui::PushID("undo_preset_delete");
				if (UI::transparentButton(ICON_FA_TIMES, UI::btnSmall, false))
					presetManager.deletedPreset = {};

				UI::tooltip(getString("close"));
				ImGui::PopID();
			}

			float presetButtonHeight = ImGui::GetFrameHeight();
			float windowHeight = ImGui::GetContentRegionAvail().y - presetButtonHeight - ImGui::GetStyle().WindowPadding.y;
			if (ImGui::BeginChild("presets_child_window", ImVec2(-1, windowHeight), true))
			{
				if (loadingPresets)
				{
					Utilities::ImGuiCenteredText("Loading...");
				}
				else if (!presetManager.presets.size())
				{
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().DisabledAlpha);
					Utilities::ImGuiCenteredText(getString("no_presets"));
					ImGui::PopStyleVar();
				}
				else
				{
					for (size_t i = 0; i < presetManager.presets.size(); i++)
					{
						const auto& preset = presetManager.presets.at(i);
						if (!presetFilter.PassFilter(preset.getName().c_str()))
							continue;

						ImGui::PushID(preset.getID());

						if (ImGui::Button(preset.getName().c_str(), ImVec2(ImGui::GetContentRegionAvail().x - UI::btnSmall.x - 2.0f, presetButtonHeight)))
							presetManager.applyPreset(i, context);

						if (!preset.description.empty())
							UI::tooltip(preset.description.c_str());

						ImGui::SameLine();
						if (UI::transparentButton(ICON_FA_TRASH, ImVec2(UI::btnSmall.x, presetButtonHeight)))
							removePattern = i;

						ImGui::PopID();
					}
				}
			}
			ImGui::EndChild();
			ImGui::Separator();

			ImGui::BeginDisabled(context.selectedNotes.empty() || loadingPresets);
			if (ImGui::Button(getString("create_preset"), ImVec2(-1, presetButtonHeight)))
				dialogOpen = true;

			ImGui::EndDisabled();

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
			presetManager.createPreset(context, presetName, presetDesc);
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

			ImVec2 btnSz{ (ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) / 2.0f, ImGui::GetFrameHeight() };
			ImGui::SetCursorPos(ImVec2(xPos, yPos));
			if (ImGui::Button(getString("cancel"), btnSz))
			{
				result = DialogResult::Cancel;
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			ImGui::BeginDisabled(presetName.empty());
			if (ImGui::Button(getString("confirm"), btnSz))
			{
				result = DialogResult::Ok;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndDisabled();
			ImGui::EndPopup();
		}

		return result;
	}

	DialogResult RecentFileNotFoundDialog::update()
	{
		DialogResult result{ DialogResult::None };
		if (open)
		{
			ImGui::OpenPopup(MODAL_TITLE("file_not_found"));
			open = false;
		}

		std::string dialogText = IO::formatString("%s \"%s\" %s. %s",
			getString("file_not_found_msg1"),
			removeFilename.c_str(),
			getString("file_not_found_msg2"),
			getString("remove_recent_file_not_found")
		);

		float maxDialogSizeX{ ImGui::GetMainViewport()->WorkSize.x * 0.80f };
		ImVec2 padding = ImGui::GetStyle().WindowPadding;
		ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
		ImVec2 textSize = ImGui::CalcTextSize(dialogText.c_str());

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(std::min(maxDialogSizeX, textSize.x + (padding.x * 2) + spacing.x ), 0), ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(MODAL_TITLE("file_not_found"), NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::TextWrapped(dialogText.c_str());
			
			// New line to move the buttons a bit down
			ImGui::Text("\n");

			ImVec2 btnSz{ (ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) / 2.0f, ImGui::GetFrameHeight() };
			btnSz.x = std::min(btnSz.x, 150.0f);
			
			// Right align buttons
			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x - (btnSz.x * 2) - spacing.x - padding.x, ImGui::GetCursorPosY()));

			if (ImGui::Button(getString("yes"), btnSz))
			{
				close();
				result = DialogResult::Yes;
			}

			ImGui::SameLine();
			if (ImGui::Button(getString("no"), btnSz))
			{
				close();
				result = DialogResult::No;
			}

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
			
			float btnsHeight = ImGui::GetFrameHeight();
			float xPos = padding.x;
			float yPos = ImGui::GetWindowSize().y - btnsHeight - padding.y;
			ImGui::SetCursorPos(ImVec2(xPos, yPos));

			ImVec2 btnSz{ (ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) / 3.0f, btnsHeight };

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
		if (open)
		{
			ImGui::OpenPopup(MODAL_TITLE("about"));
			open = false;
		}

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(450, 250), ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(MODAL_TITLE("about"), NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::Text("MikuMikuWorld\nCopyright (C) 2022 Crash5b\n\n");
			ImGui::Separator();

			float okButtonHeight = ImGui::GetFrameHeight();

			ImGui::Text("Version %s", Application::getAppVersion().c_str());
			ImGui::SetCursorPos({
				ImGui::GetStyle().WindowPadding.x,
				ImGui::GetWindowSize().y - okButtonHeight - ImGui::GetStyle().WindowPadding.y
			});

			if (ImGui::Button("OK", { ImGui::GetContentRegionAvail().x, okButtonHeight }))
			{
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return DialogResult::Ok;
			}

			ImGui::EndPopup();
		}

		return DialogResult::None;
	}

	void DebugWindow::update(ScoreContext& context, ScoreEditorTimeline& timeline)
	{
		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_BUG, "debug")))
		{
			constexpr ImGuiTreeNodeFlags headerFlags = ImGuiTreeNodeFlags_DefaultOpen;
			constexpr ImGuiTreeNodeFlags treeNodeFlags = headerFlags | ImGuiTreeNodeFlags_Framed;
			if (ImGui::TreeNodeEx("Audio", treeNodeFlags))
			{
				if (ImGui::CollapsingHeader("Engine", headerFlags))
				{
					UI::beginPropertyColumns();
					UI::addReadOnlyProperty("Sample Rate", context.audio.getDeviceSampleRate());
					UI::addReadOnlyProperty("Channel Count", context.audio.getDeviceChannelCount());
					UI::addReadOnlyProperty("Latency", IO::formatString("%.2fms", context.audio.getDeviceLatency() * 1000));
					UI::endPropertyColumns();
				}

				if (ImGui::CollapsingHeader("Music", headerFlags))
				{
					UI::beginPropertyColumns();
					UI::addReadOnlyProperty("Music Initialized", boolToString(context.audio.isMusicInitialized()));
					UI::addReadOnlyProperty("Music Filename", context.audio.musicBuffer.name);

					float musicTime = context.audio.getMusicPosition(), musicLength = context.audio.getMusicLength();
					int musicTimeSeconds = static_cast<int>(musicTime), musicLengthSeconds = static_cast<int>(musicLength);
					UI::addReadOnlyProperty("Music Time",IO::formatString("%02d:%02d/%02d:%02d",
						musicTimeSeconds, static_cast<int>((musicTime - musicTimeSeconds) * 100),
						musicLengthSeconds, static_cast<int>((musicLength - musicLengthSeconds) * 100)
					));

					UI::addReadOnlyProperty("Sample Rate", context.audio.musicBuffer.sampleRate);
					UI::addReadOnlyProperty("Effective Sample Rate", context.audio.musicBuffer.effectiveSampleRate);
					UI::addReadOnlyProperty("Channel Count", context.audio.musicBuffer.channelCount);
					UI::endPropertyColumns();
				}

				if (ImGui::CollapsingHeader("Waveform", headerFlags))
				{
					UI::beginPropertyColumns();
					UI::addReadOnlyProperty("Waveform L", boolToString(!context.waveformL.isEmpty()));
					UI::addReadOnlyProperty("Waveform L Mip Count", context.waveformL.getUsedMipCount());
					UI::addReadOnlyProperty("Waveform L Samples", context.waveformL.mips->absoluteSamples.size());
					UI::addReadOnlyProperty("Waveform R", boolToString(!context.waveformR.isEmpty()));
					UI::addReadOnlyProperty("Waveform R Mip Count", context.waveformR.getUsedMipCount());
					UI::addReadOnlyProperty("Waveform R Samples", context.waveformL.mips->absoluteSamples.size());
					UI::endPropertyColumns();

					if (ImGui::Button("Re-Generate Waveform", { -1, UI::btnSmall.y }))
					{
						context.waveformL.generateMipChainsFromSampleBuffer(context.audio.musicBuffer, 0);
						context.waveformR.generateMipChainsFromSampleBuffer(context.audio.musicBuffer, 1);
					}
				}

				if (ImGui::CollapsingHeader("Sound Test", headerFlags))
				{
					constexpr ImGuiTableFlags tableFlags =
						ImGuiTableFlags_BordersOuter |
						ImGuiTableFlags_BordersInnerH |
						ImGuiTableFlags_BordersInnerV |
						ImGuiTableFlags_ScrollY |
						ImGuiTableFlags_RowBg;

					const int rowHeight = ImGui::GetFrameHeight() + 5;

					if (ImGui::BeginTable("##sound_test_table", 3, tableFlags, { -1, 200 }))
					{
						ImGui::TableSetupScrollFreeze(0, 1);
						ImGui::TableSetupColumn("Name");
						ImGui::TableSetupColumn("Duration (sec)", ImGuiTableColumnFlags_WidthFixed);
						ImGui::TableSetupColumn("Play/Stop", ImGuiTableColumnFlags_WidthFixed);
						ImGui::TableHeadersRow();

						for (size_t i = 0; i < arrayLength(SE_NAMES) * Audio::soundEffectsProfileCount; i++)
						{
							Audio::SoundInstance& sound = context.audio.debugSounds[i];

							ImGui::PushID(i);
							ImGui::TableNextRow(0, rowHeight);
							ImGui::TableSetColumnIndex(0);

							float ratio = sound.getCurrentFrame() / static_cast<float>(sound.getLengthInFrames());
							if (!sound.isPlaying())
								ratio = 0.0f;

							ImGui::ProgressBar(ratio, { -1, 0 }, sound.name.c_str());
							
							float duration = sound.getDuration(); int durationSecondsOnly = duration;
							float time = sound.isPlaying() ? sound.getCurrentTime() : 0.0f; int timeSecondsOnly = time;
							ImGui::TableSetColumnIndex(1);
							ImGui::Text("%02d:%02d/%02d:%02d",
								timeSecondsOnly,
								static_cast<int>((time - timeSecondsOnly) * 100),
								durationSecondsOnly,
								static_cast<int>((duration - durationSecondsOnly) * 100)
							);

							ImGui::TableSetColumnIndex(2);
							if (UI::transparentButton(ICON_FA_PLAY, UI::btnSmall))
							{
								sound.seek(0);
								sound.play();
							}

							ImGui::SameLine();
							ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
							ImGui::SameLine();
							if (UI::transparentButton(ICON_FA_STOP, UI::btnSmall))
								sound.stop();

							ImGui::PopID();
						}

						ImGui::EndTable();
					}
				}

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Timeline", treeNodeFlags))
			{
				timeline.debug(context);
				ImGui::TreePop();
			}
		}

		ImGui::End();
	}

	void SettingsWindow::updateKeyConfig(MultiInputBinding* bindings[], int count)
	{
		ImVec2 size = ImVec2(-1, ImGui::GetContentRegionAvail().y * 0.7);
		constexpr ImGuiTableFlags tableFlags =
			ImGuiTableFlags_BordersOuter |
			ImGuiTableFlags_BordersInnerH |
			ImGuiTableFlags_ScrollY |
			ImGuiTableFlags_RowBg;

		constexpr ImGuiSelectableFlags selectionFlags =
			ImGuiSelectableFlags_SpanAllColumns |
			ImGuiSelectableFlags_AllowItemOverlap;

		const int rowHeight = ImGui::GetFrameHeight() + 5;

		if (ImGui::BeginTable("##commands_table", 2, tableFlags, size))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn(getString("action"));
			ImGui::TableSetupColumn(getString("keys"));
			ImGui::TableHeadersRow();

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
			int moveIndex = -1;
			int moveDirection = 0;
			const bool canAdd = bindings[selectedBindingIndex]->count < 4;

			const float btnHeight = ImGui::GetFrameHeight();

			UI::beginPropertyColumns();
			ImGui::Text(getString(bindings[selectedBindingIndex]->name));
			ImGui::NextColumn();

			ImGui::BeginDisabled(!canAdd);
			if (ImGui::Button(getString("add"), {-1, btnHeight}))
				bindings[selectedBindingIndex]->addBinding(InputBinding{});
			ImGui::EndDisabled();

			UI::endPropertyColumns();

			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
			ImGui::BeginChild("##binding_keys_edit_window", ImVec2(-1, -1), true);

			float btnWidth = (UI::btnSmall.x * 2) + 100 + (ImGui::GetStyle().ItemSpacing.x * 3);
			for (int b = 0; b < bindings[selectedBindingIndex]->count; ++b)
			{
				const bool canMoveDown = !(bindings[selectedBindingIndex]->count <= b + 1);
				const bool canMoveUp = !(b < 1);
				ImGui::PushID(b);

				std::string buttonText = ToShortcutString(bindings[selectedBindingIndex]->bindings[b]);;
				if (!buttonText.size())
					buttonText = getString("none");

				if (listeningForInput && editBindingIndex == b)
					buttonText = getString("cmd_key_listen");

				if (ImGui::Button(buttonText.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - btnWidth, btnHeight)))
				{
					listeningForInput = true;
					inputTimer.reset();
					editBindingIndex = b;
				}

				ImGui::SameLine();
				ImGui::BeginDisabled(!canMoveUp);
				if (ImGui::Button(ICON_FA_CARET_UP, { btnHeight, btnHeight }))
				{
					moveIndex = b;
					moveDirection = -1;
				}
				ImGui::EndDisabled();

				ImGui::SameLine();
				ImGui::BeginDisabled(!canMoveDown);
				if (ImGui::Button(ICON_FA_CARET_DOWN, { btnHeight, btnHeight }))
				{
					moveIndex = b;
					moveDirection = 1;
				}
				ImGui::EndDisabled();

				ImGui::SameLine();
				if (ImGui::Button(getString("remove"), { -1, btnHeight }))
					deleteBinding = b;

				ImGui::PopID();
			}
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			if (moveIndex > -1)
			{
				listeningForInput = false;
				if (moveDirection == -1) bindings[selectedBindingIndex]->moveUp(moveIndex);
				if (moveDirection == 1) bindings[selectedBindingIndex]->moveDown(moveIndex);
			}

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
				for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_MouseLeft; ++key)
				{
					bool isCtrl = key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl || key == ImGuiKey_ModCtrl;
					bool isShift = key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift || key == ImGuiKey_ModShift;
					bool isAlt = key == ImGuiKey_LeftAlt || key == ImGuiKey_RightAlt || key == ImGuiKey_ModAlt;
					bool isSuper = key == ImGuiKey_LeftSuper || key == ImGuiKey_RightSuper || key == ImGuiKey_ModSuper;

					// execute if a non-modifier key is tapped
					if (ImGui::IsKeyPressed((ImGuiKey)key) && !isCtrl && !isShift && !isAlt && !isSuper)
					{
						bindings[selectedBindingIndex]->bindings[editBindingIndex] = InputBinding((ImGuiKey)key, (ImGuiModFlags_)ImGui::GetIO().KeyMods);
						listeningForInput = false;
						editBindingIndex = -1;
					}
				}
			}
		}
	}

	DialogResult SettingsWindow::update()
	{
		if (open)
		{
			ImGui::OpenPopup(MODAL_TITLE("settings"));
			open = false;
		}

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
				if (ImGui::BeginTabItem(IMGUI_TITLE("", "general")))
				{
					if (ImGui::CollapsingHeader(getString("language"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::propertyLabel(getString("language"));

						std::string curr = getString("auto");
						auto langIt = Localization::languages.find(config.language);
						if (langIt != Localization::languages.end())
							curr = langIt->second->getDisplayName();

						if (ImGui::BeginCombo("##language", curr.c_str()))
						{
							if (ImGui::Selectable(getString("auto"), config.language == "auto"))
								config.language = "auto";

							for (const auto& [code, language] : Localization::languages)
							{
								const bool selected = curr == code;
								std::string str = language->getDisplayName();

								if (ImGui::Selectable(str.c_str(), selected))
									config.language = code;
							}

							ImGui::EndCombo();
						}

						UI::endPropertyColumns();
					}

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
							customColor.x = col[0]; customColor.y = col[1]; customColor.z = col[2];

						UI::endPropertyColumns();
						ImGui::PopStyleVar();
					}

					if (ImGui::CollapsingHeader(getString("video"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						bool vsync = Application::windowState.vsync;
						UI::beginPropertyColumns();
						UI::addCheckboxProperty(getString("vsync"), Application::windowState.vsync);
						UI::endPropertyColumns();

						if (vsync != Application::windowState.vsync)
							glfwSwapInterval(Application::windowState.vsync ? 1 : 0);
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(IMGUI_TITLE("", "timeline")))
				{
					if (ImGui::CollapsingHeader(getString("timeline"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::addCheckboxProperty(getString("match_timeline_size_to_window"), config.matchTimelineSizeToScreen);
						UI::addCheckboxProperty(getString("match_notes_size_to_timeline"), config.matchNotesSizeToTimeline);
						UI::addIntProperty(getString("lane_width"), config.timelineWidth, "%dpx", MIN_LANE_WIDTH, MAX_LANE_WIDTH);
						
						UI::addIntProperty(getString("notes_height"), config.notesHeight, "%dpx", MIN_NOTES_HEIGHT, MAX_NOTES_HEIGHT);
						ImGui::Separator();

						UI::addCheckboxProperty(getString("draw_waveform"), config.drawWaveform);
						UI::addCheckboxProperty(getString("return_to_last_tick"), config.returnToLastSelectedTickOnPause);
						UI::addCheckboxProperty(getString("cursor_auto_scroll"), config.followCursorInPlayback);
						UI::addPercentSliderProperty(getString("cursor_auto_scroll_amount"), config.cursorPositionThreshold);
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("scrolling"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::addFloatProperty(getString("scroll_speed_normal"), config.scrollSpeedNormal, "%.1fx");
						UI::addFloatProperty(getString("scroll_speed_shift"), config.scrollSpeedShift, "%.1fx");
						config.scrollSpeedNormal = std::max(0.1f, config.scrollSpeedNormal);
						config.scrollSpeedShift = std::max(0.1f, config.scrollSpeedShift);
						ImGui::Separator();

						UI::addCheckboxProperty(getString("use_smooth_scroll"), config.useSmoothScrolling);
						UI::addSliderProperty(getString("smooth_scroll_time"), config.smoothScrollingTime, 10.0f, 150.0f, "%.2fms");
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("audio"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::addSelectProperty(getString("notes_se"), config.seProfileIndex, Audio::soundEffectsProfileNames, Audio::soundEffectsProfileCount);
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("background"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::addCheckboxProperty(getString("draw_background"), config.drawBackground);

						std::string backgroundFile = config.backgroundImage;
						int result = UI::addFileProperty(getString("background_image"), backgroundFile);
						if (result == 1)
						{
							config.backgroundImage = backgroundFile;
							isBackgroundChangePending = true;
						}
						else if (result == 2)
						{
							IO::FileDialog fileDialog{};
							fileDialog.title = "Open Image File";
							fileDialog.filters = { IO::imageFilter, IO::allFilter };
							fileDialog.parentWindowHandle = Application::windowState.windowHandle;

							if (fileDialog.openFile() == IO::FileDialogResult::OK)
							{
								config.backgroundImage = fileDialog.outputFilename;
								isBackgroundChangePending = true;
							}
						}

						UI::addPercentSliderProperty(getString("background_brightnes"), config.backgroundBrightness);
						ImGui::Separator();

						UI::addPercentSliderProperty(getString("lanes_opacity"), config.laneOpacity);
						UI::endPropertyColumns();
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(IMGUI_TITLE("", "preview")))
				{
					if (ImGui::CollapsingHeader(getString("general"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::addSliderProperty(getString("notes_speed"), config.pvNoteSpeed, 1, 12, "%.2f");
						UI::addCheckboxProperty(getString("mirror_score"), config.pvMirrorScore);
						UI::addCheckboxProperty(getString("preview_draw_toolbar"), config.pvDrawToolbar);
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("background"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::addCheckboxProperty(getString("draw_background"), config.drawBackground);

						std::string backgroundFile = config.backgroundImage;
						int result = UI::addFileProperty(getString("background_image"), backgroundFile);
						if (result == 1)
						{
							config.backgroundImage = backgroundFile;
							isBackgroundChangePending = true;
						}
						else if (result == 2)
						{
							IO::FileDialog fileDialog{};
							fileDialog.title = "Open Image File";
							fileDialog.filters = { IO::imageFilter, IO::allFilter };
							fileDialog.parentWindowHandle = Application::windowState.windowHandle;

							if (fileDialog.openFile() == IO::FileDialogResult::OK)
							{
								config.backgroundImage = fileDialog.outputFilename;
								isBackgroundChangePending = true;
							}
						}

						UI::addPercentSliderProperty(getString("background_brightnes"), config.pvBackgroundBrightness);
						ImGui::Separator();

						UI::addPercentSliderProperty(getString("stage_opacity"), config.pvStageOpacity);
						UI::addPercentSliderProperty(getString("stage_cover"), config.pvStageCover);
						UI::addCheckboxProperty(getString("stage_lock_ratio"), config.pvLockAspectRatio);
						UI::endPropertyColumns();
					}
					
					if (ImGui::CollapsingHeader(getString("visuals"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						const std::vector<std::string>& noteSkinNames = noteSkins.getSkinNames();

						UI::addSelectProperty(getString("notes_skin"), config.notesSkin, noteSkinNames, noteSkins.count());

						UI::addCheckboxProperty(getString("flicks_animation"), config.pvFlickAnimation);
						UI::addCheckboxProperty(getString("holds_animation"), config.pvHoldAnimation);
						UI::addCheckboxProperty(getString("simultaneous_lines"), config.pvSimultaneousLine);
						ImGui::Separator();

						float hold_alpha = config.pvHoldAlpha * 100.f;
						UI::addSliderProperty(getString("holds_alpha"), hold_alpha, 10, 100, "%.0f%%");
						config.pvHoldAlpha = hold_alpha / 100.f;

						float guide_alpha = config.pvGuideAlpha * 100.f;
						UI::addSliderProperty(getString("guides_alpha"), guide_alpha, 10, 100, "%.0f%%");
						config.pvGuideAlpha = guide_alpha / 100.f;
						
						UI::endPropertyColumns();
					}
					
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(IMGUI_TITLE("", "key_config")))
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
