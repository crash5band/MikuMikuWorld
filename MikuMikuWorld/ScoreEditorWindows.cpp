#include "ScoreEditorWindows.h"
#include "Application.h"
#include "ApplicationConfiguration.h"
#include "Constants.h"
#include "File.h"
#include "ScoreContext.h"
#include "UI.h"
#include "Utilities.h"

namespace MikuMikuWorld
{
	void ScorePropertiesWindow::update(ScoreContext& context)
	{
		if (ImGui::CollapsingHeader(
		        IO::concat(ICON_FA_ALIGN_LEFT, getString("metadata"), " ").c_str(),
		        ImGuiTreeNodeFlags_DefaultOpen))
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
				IO::FileDialog fileDialog{};
				fileDialog.title = "Open Image File";
				fileDialog.filters = { { "Image Files", "*.jpg;*.jpeg;*.png" },
					                   { IO::allFilesName, IO::allFilesFilter } };
				fileDialog.parentWindowHandle = Application::windowState.windowHandle;

				if (fileDialog.openFile() == IO::FileDialogResult::OK)
					context.workingData.jacket.load(fileDialog.outputFilename);
			}
			context.workingData.jacket.draw();

			UI::addIntProperty(getString("lane_extension"), context.score.metadata.laneExtension, 0,
			                   100);
			UI::endPropertyColumns();
		}

		if (ImGui::CollapsingHeader(IO::concat(ICON_FA_VOLUME_UP, getString("audio"), " ").c_str(),
		                            ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::beginPropertyColumns();

			bool changeMusic = false;
			std::string filename = context.workingData.musicFilename;
			int filePickResult = UI::addFileProperty(getString("music_file"), filename);
			if (filePickResult == 1 && filename != context.workingData.musicFilename)
			{
				isPendingLoadMusic = true;
				pendingLoadMusicFilename = filename;
			}
			else if (filePickResult == 2)
			{
				IO::FileDialog fileDialog{};
				fileDialog.title = "Open Audio File";
				fileDialog.filters = { { "Audio Files", "*.mp3;*.wav;*.flac;*.ogg" },
					                   { IO::allFilesName, IO::allFilesFilter } };
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

		if (ImGui::CollapsingHeader(
		        IO::concat(ICON_FA_CHART_BAR, getString("statistics"), " ").c_str(),
		        ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::beginPropertyColumns();
			UI::addReadOnlyProperty(getString("taps"), context.scoreStats.getTaps());
			UI::addReadOnlyProperty(getString("flicks"), context.scoreStats.getFlicks());
			UI::addReadOnlyProperty(getString("holds"), context.scoreStats.getHolds());
			UI::addReadOnlyProperty(getString("steps"), context.scoreStats.getSteps());
			UI::addReadOnlyProperty(getString("traces"), context.scoreStats.getTraces());
			UI::addReadOnlyProperty(getString("total"), context.scoreStats.getTotal());
			UI::addReadOnlyProperty(getString("combo"), context.scoreStats.getCombo());
			UI::endPropertyColumns();
		}
	}

	void ScoreNotePropertiesWindow::update(ScoreContext& context)
	{
		auto numSelected = context.selectedNotes.size() + context.selectedHiSpeedChanges.size();
		if (numSelected == 0)
		{
			ImGui::Text("%s", getString("note_properties_not_selected"));
			return;
		}

		Score prev = context.score;
		bool edited = false;

		int selectedTick;
		int selectedLayer;
		try
		{
			selectedTick =
			    context.selectedNotes.size() >= 1
			        ? context.score.notes.at(*context.selectedNotes.begin()).tick
			        : context.score.hiSpeedChanges.at(*context.selectedHiSpeedChanges.begin()).tick;
			selectedLayer =
			    context.selectedNotes.size() >= 1
			        ? context.score.notes.at(*context.selectedNotes.begin()).layer
			        : context.score.hiSpeedChanges.at(*context.selectedHiSpeedChanges.begin())
			              .layer;
		}
		catch (const std::out_of_range& e)
		{
			ImGui::Text("%s", getString("note_properties_not_selected"));
			return;
		}

		if (ImGui::CollapsingHeader(IO::concat(ICON_FA_COG, getString("general"), " ").c_str(),
		                            ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::beginPropertyColumns();

			double beat = selectedTick / static_cast<float>(TICKS_PER_BEAT);
			if (UI::addDoubleProperty(getString("beat"), beat, "%.3f"))
			{
				auto newTick = std::floor(beat * TICKS_PER_BEAT);
				for (auto& id : context.selectedNotes)
				{
					context.score.notes.at(id).tick = newTick;
				}
				for (auto& id : context.selectedHiSpeedChanges)
				{
					context.score.hiSpeedChanges.at(id).tick = newTick;
				}
				edited = true;
			}

			if (config.showTickInProperties)
			{
				if (UI::addIntProperty(getString("tick"), selectedTick))
				{
					for (auto& id : context.selectedNotes)
					{
						context.score.notes.at(id).tick = selectedTick;
					}
					for (auto& id : context.selectedHiSpeedChanges)
					{
						context.score.hiSpeedChanges.at(id).tick = selectedTick;
					}
					edited = true;
				}
			}

			UI::propertyLabel(getString("layer"));
			const std::string layer_name = context.score.layers[selectedLayer].name;
			if (ImGui::BeginCombo(IO::concat("##", getString("layer")).c_str(), layer_name.c_str()))
			{
				for (int i = 0; i < context.score.layers.size(); i++)
				{
					auto& layer = context.score.layers[i];
					bool selected = selectedLayer == i;
					if (ImGui::Selectable(layer.name.c_str(), selected))
					{
						for (auto& id : context.selectedNotes)
						{
							context.score.notes.at(id).layer = i;
						}
						for (auto& id : context.selectedHiSpeedChanges)
						{
							context.score.hiSpeedChanges.at(id).layer = i;
						}
						edited = true;
					}
				}
				ImGui::EndCombo();
			}

			UI::endPropertyColumns();
		}

		if (context.selectedNotes.size() >= 1)
		{
			bool isGuide = false;

			bool multipleHold = false;
			int holdIndex = -1;
			for (int id : context.selectedNotes)
			{
				const Note& n = context.score.notes.at(id);
				if (n.isHold())
				{
					auto prevHoldIndex = holdIndex;
					if (n.getType() == NoteType::Hold)
					{
						holdIndex = id;
					}
					else
					{
						holdIndex = n.parentID;
					}
					if (prevHoldIndex != -1 && prevHoldIndex != holdIndex)
					{
						multipleHold = true;
						break;
					}
				}
			}

			Note& note = context.score.notes.at(*context.selectedNotes.begin());
			if (ImGui::CollapsingHeader(
			        IO::concat(ICON_FA_COG, getString("note_properties_note"), " ").c_str(),
			        ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();

				if (UI::addFloatProperty(getString("lane"), note.lane, "%.2f"))
				{
					for (auto& id : context.selectedNotes)
					{
						context.score.notes.at(id).lane = note.lane;
					}
					edited = true;
				}

				if (UI::addFloatProperty(getString("width"), note.width, "%.2f"))
				{
					for (auto& id : context.selectedNotes)
					{
						auto& localNote = context.score.notes.at(id);
						if (localNote.isHold())
						{
							auto& hold = context.score.holdNotes.at(
							    localNote.parentID == -1 ? id : localNote.parentID);
							if (hold.isGuide())
							{
								localNote.width = std::max(0.0f, note.width);
								continue;
							}
						}

						localNote.width = std::max(0.5f, note.width);
					}
					edited = true;
				}
			}

			if (context.hasHoldInSelection())
			{
				if (!multipleHold)
				{
					auto& hold = context.score.holdNotes.at(holdIndex);
					auto& holdStart = context.score.notes.at(hold.start.ID);

					isGuide = hold.isGuide();
					if (note.width == 0 && !isGuide)
					{
						if (note.getType() == NoteType::Hold)
						{
							hold.startType = HoldNoteType::Hidden;
						}
						else if (note.getType() == NoteType::HoldEnd)
						{
							hold.endType = HoldNoteType::Hidden;
						}
					}

					if (!isGuide)
					{
						if (note.getType() == NoteType::Hold || note.getType() == NoteType::HoldEnd)
						{
							if (UI::addCheckboxProperty(getString("trace"), note.friction))
							{
								for (auto& id : context.selectedNotes)
								{
									auto& n = context.score.notes.at(id);
									if (n.canTrace())
									{
										n.friction = note.friction;
									}
								}
								edited = true;
							}
						}
						if (UI::addCheckboxProperty(getString("critical"), note.critical))
						{
							context.score.notes.at(hold.start.ID).critical = note.critical;
							for (auto& step : hold.steps)
							{
								context.score.notes.at(step.ID).critical = note.critical;
							}
							context.score.notes.at(hold.end).critical = note.critical;
							for (auto& id : context.selectedNotes)
							{
								context.score.notes.at(id).critical = note.critical;
							}
							edited = true;
						}
					}

					if (note.getType() == NoteType::HoldEnd && !isGuide)
					{
						if (UI::addFlickSelectPropertyWithNone(getString("flick"), note.flick,
						                                       flickTypes, arrayLength(flickTypes)))
						{
							context.score.notes.at(hold.start.ID).flick = note.flick;
							for (auto& id : context.selectedNotes)
							{
								auto& n = context.score.notes.at(id);
								if (!n.isHold())
								{
									n.flick = note.flick;
								}
							}
						}
					}
				}
			}
			else
			{
				if (UI::addCheckboxProperty(getString("trace"), note.friction))
				{
					for (auto& id : context.selectedNotes)
					{
						auto& n = context.score.notes.at(id);
						if (n.canTrace())
						{
							n.friction = note.friction;
						}
					}
					edited = true;
				}
				if (UI::addCheckboxProperty(getString("critical"), note.critical))
				{
					for (auto& id : context.selectedNotes)
					{
						context.score.notes.at(id).critical = note.critical;
					}
					edited = true;
				}
				if (UI::addFlickSelectPropertyWithNone(getString("flick"), note.flick, flickTypes,
				                                       arrayLength(flickTypes)))
				{
					for (auto& id : context.selectedNotes)
					{
						auto& n = context.score.notes.at(id);
						if (n.canFlick())
						{
							n.flick = note.flick;
						}
					}
					edited = true;
				}
			}

			UI::endPropertyColumns();
			if (context.hasHoldInSelection())
			{
				if (ImGui::CollapsingHeader(IO::concat(ICON_FA_COG,
				                                       isGuide
				                                           ? getString("note_properties_guide")
				                                           : getString("note_properties_hold_note"),
				                                       " ")
				                                .c_str(),
				                            ImGuiTreeNodeFlags_DefaultOpen))
				{

					if (multipleHold)
					{
						ImGui::Text("%s", getString("note_properties_many_hold_notes"));
					}
					else
					{
						auto& hold = context.score.holdNotes.at(holdIndex);
						auto& holdStart = context.score.notes.at(hold.start.ID);

						int stepIndex = findHoldStep(hold, note.ID);

						UI::beginPropertyColumns();

						bool hasEaseable = false;
						bool hasStepType = false;
						for (auto id : context.selectedNotes)
						{
							if (context.score.notes.at(id).hasEase())
							{
								hasEaseable = true;
							}
							if (context.score.notes.at(id).getType() == NoteType::HoldMid)
							{
								hasStepType = true;
							}
						}

						if (hasEaseable)
						{
							auto ease = note.getType() == NoteType::Hold
							                ? hold.start.ease
							                : hold.steps.at(stepIndex == -1 ? 0 : stepIndex).ease;
							if (UI::addSelectProperty(getString("ease_type"), ease, easeTypes,
							                          arrayLength(easeTypes)))
							{
								for (auto id : context.selectedNotes)
								{
									auto& note = context.score.notes.at(id);
									if (note.hasEase())
									{
										if (note.getType() == NoteType::Hold)
										{
											hold.start.ease = ease;
										}
										else
										{
											auto& step = hold.steps.at(findHoldStep(hold, note.ID));
											step.ease = ease;
										}
									}
								}
							}
						}

						if (hasStepType)
						{
							auto stepType = hold.steps.at(stepIndex == -1 ? 0 : stepIndex).type;

							if (UI::addSelectProperty(getString("step_type"), stepType, stepTypes,
							                          arrayLength(stepTypes)))
							{
								for (auto id : context.selectedNotes)
								{
									auto& note = context.score.notes.at(id);
									if (note.getType() == NoteType::HoldMid)
									{
										auto& step = hold.steps.at(findHoldStep(hold, note.ID));
										step.type = stepType;
									}
								}
							}
						}

						if (isGuide)
						{
							edited |= UI::addSelectProperty(getString("guide_color"),
							                                hold.guideColor, guideColorsForString,
							                                arrayLength(guideColors));
							edited |= UI::addSelectProperty(getString("fade_type"), hold.fadeType,
							                                fadeTypes, arrayLength(fadeTypes));
						}
						else
						{
							auto holdType =
							    note.getType() == NoteType::Hold ? hold.startType : hold.endType;
							if (UI::addSelectProperty(getString("hold_type"), holdType, holdTypes,
							                          2))
							{
								for (auto id : context.selectedNotes)
								{
									auto& note = context.score.notes.at(id);
									if (note.getType() == NoteType::Hold)
									{
										hold.startType = holdType;
									}
									else
									{
										hold.endType = holdType;
									}
								}
								edited = true;
							}
						}
					}
				}

				UI::endPropertyColumns();
			}
		}
		if (context.selectedHiSpeedChanges.size() >= 1)
		{
			HiSpeedChange& hiSpeed =
			    context.score.hiSpeedChanges.at(*context.selectedHiSpeedChanges.begin());
			if (ImGui::CollapsingHeader(
			        IO::concat(ICON_FA_FAST_FORWARD, getString("note_properties_hi_speed"), " ")
			            .c_str(),
			        ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();

				if (UI::addFloatProperty(getString("hi_speed"), hiSpeed.speed, "%.3f"))
				{
					hiSpeed.speed = hiSpeed.speed;
					edited = true;
				}

				UI::endPropertyColumns();
			}
		}

		if (edited)
			context.pushHistory("Edited object", prev, context.score);
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
			UI::addIntProperty(getString("note_width"), edit.noteWidth, MIN_NOTE_WIDTH,
			                   MAX_NOTE_WIDTH);
			UI::addSelectProperty(getString("step_type"), edit.stepType, stepTypes,
			                      arrayLength(stepTypes));
			UI::addSelectProperty(getString("ease_type"), edit.easeType, easeTypes,
			                      arrayLength(easeTypes));
			UI::addSelectProperty<FlickType>(getString("flick"), edit.flickType, flickTypes,
			                                 arrayLength(flickTypes));
			UI::addSelectProperty<GuideColor>(getString("guide_color"), edit.colorType,
			                                  guideColorsForString, arrayLength(guideColors));
			UI::addSelectProperty<FadeType>(getString("fade_type"), edit.fadeType, fadeTypes,
			                                arrayLength(fadeTypes));

			break;
		}
		UI::endPropertyColumns();
	}

	void PresetsWindow::update(ScoreContext& context, PresetManager& presetManager)
	{
		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_DRAFTING_COMPASS, "presets")))
		{
			int removePattern = -1;

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
			                    ImVec2(0, ImGui::GetStyle().ItemSpacing.y));
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			float filterWidth = ImGui::GetContentRegionAvail().x - UI::btnSmall.x - 2;

			presetFilter.Draw("##preset_filter",
			                  IO::concat(ICON_FA_SEARCH, getString("search"), " ").c_str(),
			                  filterWidth);
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_TIMES, UI::btnSmall))
				presetFilter.Clear();

			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			float presetButtonHeight = ImGui::GetFrameHeight();
			float windowHeight = ImGui::GetContentRegionAvail().y - presetButtonHeight -
			                     ImGui::GetStyle().WindowPadding.y;
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

						if (ImGui::Button(
						        preset.getName().c_str(),
						        ImVec2(ImGui::GetContentRegionAvail().x - UI::btnSmall.x - 2.0f,
						               presetButtonHeight)))
							presetManager.applyPreset(id, context);

						if (preset.description.size())
							UI::tooltip(preset.description.c_str());

						ImGui::SameLine();
						if (UI::transparentButton(ICON_FA_TRASH,
						                          ImVec2(UI::btnSmall.x, presetButtonHeight)))
							removePattern = id;

						ImGui::PopID();
					}
				}
			}
			ImGui::EndChild();
			ImGui::Separator();

			const bool disable =
			    !(context.selectedNotes.size() + context.selectedHiSpeedChanges.size());
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, disable);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1 - (0.5f * disable));
			if (ImGui::Button(getString("create_preset"), ImVec2(-1, presetButtonHeight)))
				dialogOpen = true;

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
			presetManager.createPreset(context.score, context.selectedNotes,
			                           context.selectedHiSpeedChanges, presetName, presetDesc);
			presetName.clear();
			presetDesc.clear();
		}
	}

	DialogResult PresetsWindow::updateCreationDialog()
	{
		DialogResult result = DialogResult::None;

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
		                        ImVec2(0.5f, 0.5f));
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
			ImGui::InputTextMultiline(
			    "##preset_desc", &presetDesc,
			    { -1, ImGui::GetContentRegionAvail().y - UI::btnSmall.y - 10.0f - padding.y });

			ImVec2 btnSz{ (ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) /
				              2.0f,
				          ImGui::GetFrameHeight() };
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

	DialogResult RecentFileNotFoundDialog::update()
	{
		DialogResult result{ DialogResult::None };
		if (open)
		{
			ImGui::OpenPopup(MODAL_TITLE("file_not_found"));
			open = false;
		}

		std::string dialogText = IO::formatString(
		    "%s \"%s\" %s. %s", getString("file_not_found_msg1"), removeFilename.c_str(),
		    getString("file_not_found_msg2"), getString("remove_recent_file_not_found"));

		float maxDialogSizeX{ ImGui::GetMainViewport()->WorkSize.x * 0.80f };
		ImVec2 padding = ImGui::GetStyle().WindowPadding;
		ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
		ImVec2 textSize = ImGui::CalcTextSize(dialogText.c_str());

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
		                        ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(
		    ImVec2(std::min(maxDialogSizeX, textSize.x + (padding.x * 2) + spacing.x), 0),
		    ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(MODAL_TITLE("file_not_found"), NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::TextWrapped(dialogText.c_str());

			// New line to move the buttons a bit down
			ImGui::Text("\n");

			ImVec2 btnSz{ (ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) /
				              2.0f,
				          ImGui::GetFrameHeight() };
			btnSz.x = std::min(btnSz.x, 150.0f);

			// Right align buttons
			ImGui::SetCursorPos(
			    ImVec2(ImGui::GetWindowSize().x - (btnSz.x * 2) - spacing.x - padding.x,
			           ImGui::GetCursorPosY()));

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
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
		                        ImVec2(0.5f, 0.5f));
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

			ImVec2 btnSz{ (ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) /
				              3.0f,
				          btnsHeight };

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

	DialogResult UpdateAvailableDialog::update()
	{
		if (open)
		{
			ImGui::OpenPopup(MODAL_TITLE("update_available"));
			open = false;
		}

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
		                        ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(450, 250), ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(MODAL_TITLE("update_available"), NULL,
		                           ImGuiWindowFlags_NoResize))
		{
			ImGui::Text(getString("update_available_description"), latestVersion.c_str());

			float okButtonHeight = ImGui::GetFrameHeight();

			ImGui::SetCursorPos(
			    { ImGui::GetStyle().WindowPadding.x,
			      ImGui::GetWindowSize().y - okButtonHeight - ImGui::GetStyle().WindowPadding.y });

			ImVec2 padding = ImGui::GetStyle().WindowPadding;
			ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
			ImVec2 btnSz{ (ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) /
				              2.0f,
				          okButtonHeight };

			if (ImGui::Button(getString("yes"), btnSz))
			{
				system("start https://github.com/sevenc-nanashi/MikuMikuWorld4CC/releases/latest");
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return DialogResult::Ok;
			}

			ImGui::SameLine();
			if (ImGui::Button(getString("no"), btnSz))
			{
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return DialogResult::Ok;
			}

			ImGui::EndPopup();
		}

		return DialogResult::None;
	}

	DialogResult AboutDialog::update()
	{
		if (open)
		{
			ImGui::OpenPopup(MODAL_TITLE("about"));
			open = false;
		}

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
		                        ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(450, 250), ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(MODAL_TITLE("about"), NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::Text("MikuMikuWorld for Chart Cyanvas\nCopyright (c) 2023 Nanashi. "
			            "(@sevenc-nanashi)\n\nThis application is based on "
			            "MikuMikuWorld.\nCopyright (C) 2022 Crash5b\n\n");
			ImGui::Separator();

			float okButtonHeight = ImGui::GetFrameHeight();

			ImGui::Text("Version %s", Application::getAppVersion().c_str());

			ImGui::Text(getString("translated_by"), getString("translator"));

			ImGui::SetCursorPos(
			    { ImGui::GetStyle().WindowPadding.x,
			      ImGui::GetWindowSize().y - okButtonHeight - ImGui::GetStyle().WindowPadding.y });

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
					UI::addReadOnlyProperty(
					    "Latency",
					    IO::formatString("%.2fms", context.audio.getDeviceLatency() * 1000));
					UI::endPropertyColumns();
				}

				if (ImGui::CollapsingHeader("Music", headerFlags))
				{
					UI::beginPropertyColumns();
					UI::addReadOnlyProperty("Music Initialized",
					                        boolToString(context.audio.isMusicInitialized()));
					UI::addReadOnlyProperty("Music Filename", context.audio.musicBuffer.name);

					float musicTime = context.audio.getMusicPosition(),
					      musicLength = context.audio.getMusicLength();
					int musicTimeSeconds = static_cast<int>(musicTime),
					    musicLengthSeconds = static_cast<int>(musicLength);
					UI::addReadOnlyProperty(
					    "Music Time",
					    IO::formatString(
					        "%02d:%02d/%02d:%02d", musicTimeSeconds,
					        static_cast<int>((musicTime - musicTimeSeconds) * 100),
					        musicLengthSeconds,
					        static_cast<int>((musicLength - musicLengthSeconds) * 100)));

					UI::addReadOnlyProperty("Sample Rate", context.audio.musicBuffer.sampleRate);
					UI::addReadOnlyProperty("Effective Sample Rate",
					                        context.audio.musicBuffer.effectiveSampleRate);
					UI::addReadOnlyProperty("Channel Count",
					                        context.audio.musicBuffer.channelCount);
					UI::endPropertyColumns();
				}

				if (ImGui::CollapsingHeader("Waveform", headerFlags))
				{
					UI::beginPropertyColumns();
					UI::addReadOnlyProperty("Waveform L",
					                        boolToString(!context.waveformL.isEmpty()));
					UI::addReadOnlyProperty("Waveform L Mip Count",
					                        context.waveformL.getUsedMipCount());
					UI::addReadOnlyProperty("Waveform L Samples",
					                        context.waveformL.mips->absoluteSamples.size());
					UI::addReadOnlyProperty("Waveform R",
					                        boolToString(!context.waveformR.isEmpty()));
					UI::addReadOnlyProperty("Waveform R Mip Count",
					                        context.waveformR.getUsedMipCount());
					UI::addReadOnlyProperty("Waveform R Samples",
					                        context.waveformL.mips->absoluteSamples.size());
					UI::endPropertyColumns();

					if (ImGui::Button("Re-Generate Waveform", { -1, UI::btnSmall.y }))
					{
						context.waveformL.generateMipChainsFromSampleBuffer(
						    context.audio.musicBuffer, 0);
						context.waveformR.generateMipChainsFromSampleBuffer(
						    context.audio.musicBuffer, 1);
					}
				}

				if (ImGui::CollapsingHeader("Sound Test", headerFlags))
				{
					constexpr ImGuiTableFlags tableFlags =
					    ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH |
					    ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY |
					    ImGuiTableFlags_RowBg;

					const int rowHeight = ImGui::GetFrameHeight() + 5;

					if (ImGui::BeginTable("##sound_test_table", 3, tableFlags, { -1, 200 }))
					{
						ImGui::TableSetupScrollFreeze(0, 1);
						ImGui::TableSetupColumn("Name");
						ImGui::TableSetupColumn("Duration (sec)", ImGuiTableColumnFlags_WidthFixed);
						ImGui::TableSetupColumn("Play/Stop", ImGuiTableColumnFlags_WidthFixed);
						ImGui::TableHeadersRow();

						for (size_t i = 0;
						     i < arrayLength(SE_NAMES) * Audio::soundEffectsProfileCount; i++)
						{
							Audio::SoundInstance& sound = context.audio.debugSounds[i];

							ImGui::PushID(i);
							ImGui::TableNextRow(0, rowHeight);
							ImGui::TableSetColumnIndex(0);

							float ratio = sound.getCurrentFrame() /
							              static_cast<float>(sound.getLengthInFrames());
							if (!sound.isPlaying())
								ratio = 0.0f;

							ImGui::ProgressBar(ratio, { -1, 0 }, sound.name.c_str());

							float duration = sound.getDuration();
							int durationSecondsOnly = duration;
							float time = sound.isPlaying() ? sound.getCurrentTime() : 0.0f;
							int timeSecondsOnly = time;
							ImGui::TableSetColumnIndex(1);
							ImGui::Text("%02d:%02d/%02d:%02d", timeSecondsOnly,
							            static_cast<int>((time - timeSecondsOnly) * 100),
							            durationSecondsOnly,
							            static_cast<int>((duration - durationSecondsOnly) * 100));

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
		const ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersOuter |
		                                   ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY |
		                                   ImGuiTableFlags_RowBg;

		const ImGuiSelectableFlags selectionFlags =
		    ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
		int rowHeight = ImGui::GetFrameHeight() + 5;

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
				if (ImGui::Selectable(getString(bindings[i]->name), i == selectedBindingIndex,
				                      selectionFlags))
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

			if (!canAdd)
				UI::beginNextItemDisabled();

			if (ImGui::Button(getString("add"), { -1, btnHeight }))
				bindings[selectedBindingIndex]->addBinding(InputBinding{});

			if (!canAdd)
				UI::endNextItemDisabled();

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

				std::string buttonText =
				    ToShortcutString(bindings[selectedBindingIndex]->bindings[b]);
				;
				if (!buttonText.size())
					buttonText = getString("none");

				if (listeningForInput && editBindingIndex == b)
					buttonText = getString("cmd_key_listen");

				if (ImGui::Button(buttonText.c_str(),
				                  ImVec2(ImGui::GetContentRegionAvail().x - btnWidth, btnHeight)))
				{
					listeningForInput = true;
					inputTimer.reset();
					editBindingIndex = b;
				}

				ImGui::SameLine();
				if (!canMoveUp)
					UI::beginNextItemDisabled();
				if (ImGui::Button(ICON_FA_CARET_UP, { btnHeight, btnHeight }))
				{
					moveIndex = b;
					moveDirection = -1;
				}
				if (!canMoveUp)
					UI::endNextItemDisabled();

				ImGui::SameLine();
				if (!canMoveDown)
					UI::beginNextItemDisabled();
				if (ImGui::Button(ICON_FA_CARET_DOWN, { btnHeight, btnHeight }))
				{
					moveIndex = b;
					moveDirection = 1;
				}
				if (!canMoveDown)
					UI::endNextItemDisabled();

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
				if (moveDirection == -1)
					bindings[selectedBindingIndex]->moveUp(moveIndex);
				if (moveDirection == 1)
					bindings[selectedBindingIndex]->moveDown(moveIndex);
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
					bool isCtrl = key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl ||
					              key == ImGuiKey_ModCtrl;
					bool isShift = key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift ||
					               key == ImGuiKey_ModShift;
					bool isAlt = key == ImGuiKey_LeftAlt || key == ImGuiKey_RightAlt ||
					             key == ImGuiKey_ModAlt;
					bool isSuper = key == ImGuiKey_LeftSuper || key == ImGuiKey_RightSuper ||
					               key == ImGuiKey_ModSuper;

					// execute if a non-modifier key is tapped
					if (ImGui::IsKeyPressed((ImGuiKey)key) && !isCtrl && !isShift && !isAlt &&
					    !isSuper)
					{
						bindings[selectedBindingIndex]->bindings[editBindingIndex] =
						    InputBinding((ImGuiKey)key, (ImGuiModFlags_)ImGui::GetIO().KeyMods);
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

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
		                        ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(750, 600), ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 10));

		if (ImGui::BeginPopupModal(MODAL_TITLE("settings"), NULL, ImGuiWindowFlags_NoResize))
		{
			ImVec2 padding = ImGui::GetStyle().WindowPadding;
			ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
			ImVec2 confirmBtnPos =
			    ImGui::GetWindowSize() + ImVec2(-100, -UI::btnNormal.y) - padding;
			ImGui::BeginChild("##settings_panel", ImGui::GetContentRegionAvail() -
			                                          ImVec2(0, UI::btnNormal.y + padding.y));

			if (ImGui::BeginTabBar("##settings_tabs"))
			{
				if (ImGui::BeginTabItem(IMGUI_TITLE("", "general")))
				{
					if (ImGui::CollapsingHeader(getString("language"),
					                            ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::propertyLabel(getString("language"));

						std::string curr = getString("auto");
						auto langIt = Localization::languages.find(config.language);
						if (langIt != Localization::languages.end())
							curr = langIt->second->getString("language_name");

						if (ImGui::BeginCombo("##language", curr.c_str()))
						{
							if (ImGui::Selectable(getString("auto"), config.language == "auto"))
								config.language = "auto";

							for (const auto& [code, language] : Localization::languages)
							{
								const bool selected = curr == code;
								std::string str = language->getString("language_name");

								if (ImGui::Selectable(str.c_str(), selected))
									config.language = code;
							}

							ImGui::EndCombo();
						}

						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("auto_save"),
					                            ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::addCheckboxProperty(getString("auto_save_enable"),
						                        config.autoSaveEnabled);
						UI::addIntProperty(getString("auto_save_interval"),
						                   config.autoSaveInterval);
						UI::addIntProperty(getString("auto_save_count"), config.autoSaveMaxCount);
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("theme"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::addSelectProperty(getString("base_theme"), config.baseTheme, baseThemes,
						                      (int)BaseTheme::BASE_THEME_MAX);
						UI::endPropertyColumns();

						ImGui::TextWrapped(getString("accent_color_help"));
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
						                    ImVec2(ImGui::GetStyle().ItemSpacing.x + 3, 15));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);

						for (int i = 0; i < UI::accentColors.size(); ++i)
						{
							bool apply = false;
							std::string id = i == config.accentColor ? ICON_FA_CHECK
							                 : i == 0                ? "C"
							                                         : "##" + std::to_string(i);
							ImGui::PushStyleColor(ImGuiCol_Button, UI::accentColors[i]);
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UI::accentColors[i]);
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, UI::accentColors[i]);

							apply = ImGui::Button(id.c_str(), UI::btnNormal);

							ImGui::PopStyleColor(3);

							if ((i < UI::accentColors.size() - 1) &&
							    ImGui::GetCursorPosX() <
							        ImGui::GetWindowSize().x - UI::btnNormal.x - 50.0f)
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
						if (ImGui::BeginCombo("##color_display_mode",
						                      colorDisplayStr[(int)displayMode]))
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

						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
						                    ImVec2(ImGui::GetStyle().ItemSpacing.x + 3, 15));
						ImGuiColorEditFlags flags = 1 << (20 + (int)displayMode);
						if (ImGui::ColorEdit3("##custom_accent_color", col, flags))
							customColor.x = col[0];
						customColor.y = col[1];
						customColor.z = col[2];

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
					if (ImGui::CollapsingHeader(getString("timeline"),
					                            ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::addCheckboxProperty(getString("match_notes_size_to_timeline"),
						                        config.matchNotesSizeToTimeline);
						UI::addIntProperty(getString("lane_width"), config.timelineWidth, "%dpx",
						                   MIN_LANE_WIDTH, MAX_LANE_WIDTH);

						if (config.matchNotesSizeToTimeline)
							UI::beginNextItemDisabled();
						UI::addIntProperty(getString("notes_height"), config.notesHeight, "%dpx",
						                   MIN_NOTES_HEIGHT, MAX_NOTES_HEIGHT);
						if (config.matchNotesSizeToTimeline)
							UI::endNextItemDisabled();
						ImGui::Separator();

						UI::addCheckboxProperty(getString("draw_waveform"), config.drawWaveform);
						UI::addCheckboxProperty(getString("return_to_last_tick"),
						                        config.returnToLastSelectedTickOnPause);
						UI::addCheckboxProperty(getString("cursor_auto_scroll"),
						                        config.followCursorInPlayback);
						UI::addPercentSliderProperty(getString("cursor_auto_scroll_amount"),
						                             config.cursorPositionThreshold);
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("scrolling"),
					                            ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::addFloatProperty(getString("scroll_speed_normal"),
						                     config.scrollSpeedNormal, "%.1fx");
						UI::addFloatProperty(getString("scroll_speed_shift"),
						                     config.scrollSpeedShift, "%.1fx");
						config.scrollSpeedNormal = std::max(0.1f, config.scrollSpeedNormal);
						config.scrollSpeedShift = std::max(0.1f, config.scrollSpeedShift);
						ImGui::Separator();

						UI::addCheckboxProperty(getString("use_smooth_scroll"),
						                        config.useSmoothScrolling);
						UI::addSliderProperty(getString("smooth_scroll_time"),
						                      config.smoothScrollingTime, 10.0f, 150.0f, "%.2fms");
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("audio"), ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::addSelectProperty(getString("notes_se"), config.seProfileIndex,
						                      Audio::soundEffectsProfileNames,
						                      Audio::soundEffectsProfileCount);
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("background"),
					                            ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::addCheckboxProperty(getString("draw_background"),
						                        config.drawBackground);

						std::string backgroundFile = config.backgroundImage;
						int result =
						    UI::addFileProperty(getString("background_image"), backgroundFile);
						if (result == 1)
						{
							config.backgroundImage = backgroundFile;
							isBackgroundChangePending = true;
						}
						else if (result == 2)
						{
							IO::FileDialog fileDialog{};
							fileDialog.title = "Open Image File";
							fileDialog.filters = { { "Image Files", "*.jpg;*.jpeg;*.png" },
								                   { IO::allFilesName, IO::allFilesFilter } };
							fileDialog.parentWindowHandle = Application::windowState.windowHandle;

							if (fileDialog.openFile() == IO::FileDialogResult::OK)
							{
								config.backgroundImage = fileDialog.outputFilename;
								isBackgroundChangePending = true;
							}
						}

						UI::addPercentSliderProperty(getString("background_brightnes"),
						                             config.backgroundBrightness);
						ImGui::Separator();

						UI::addPercentSliderProperty(getString("lanes_opacity"),
						                             config.laneOpacity);
						UI::endPropertyColumns();
					}

					if (ImGui::CollapsingHeader(getString("advanced"),
					                            ImGuiTreeNodeFlags_DefaultOpen))
					{
						UI::beginPropertyColumns();
						UI::addCheckboxProperty(getString("show_tick_in_properties"),
						                        config.showTickInProperties);
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

	void LayersWindow::update(ScoreContext& context)
	{
		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_LAYER_GROUP, "layers")))
		{
			int moveUpPattern = -1;
			int moveDownPattern = -1;
			int mergePattern = -1;
			int toggleHideIndex = -1;

			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			float layersButtonHeight = ImGui::GetFrameHeight();
			float windowHeight = ImGui::GetContentRegionAvail().y - layersButtonHeight * 2 -
			                     ImGui::GetStyle().WindowPadding.y * 2;

			bool showAllLayers = context.showAllLayers;

			if (showAllLayers)
			{
				ImGui::PushStyleColor(ImGuiCol_Button,
				                      ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
				                      ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			}
			if (ImGui::Button(getString("show_all_layers"), ImVec2(-1, layersButtonHeight)))
			{
				context.showAllLayers = !context.showAllLayers;
			}
			if (showAllLayers)
				ImGui::PopStyleColor(2);

			if (ImGui::BeginChild("layers_child_window", ImVec2(-1, windowHeight), true))
			{
				int index = -1;
				for (const auto& layer : context.score.layers)
				{
					++index;
					ImGui::PushID(index);

					int isSelected = index == context.selectedLayer;

					if (isSelected)
					{
						ImGui::PushStyleColor(ImGuiCol_Button,
						                      ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
						                      ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
					}
					if (ImGui::Button(layer.name.c_str(), ImVec2(ImGui::GetContentRegionAvail().x -
					                                                 UI::btnSmall.x * 4 - 2.0f * 5,
					                                             layersButtonHeight)))
						context.selectedLayer = index;
					if (isSelected)
						ImGui::PopStyleColor(2);

					ImGui::SameLine();

					if (UI::transparentButton(layer.hidden ? ICON_FA_EYE_SLASH : ICON_FA_EYE,
					                          ImVec2(UI::btnSmall.x, layersButtonHeight), false))
					{
						toggleHideIndex = index;
					}
					UI::tooltip(layer.hidden ? getString("layer_show") : getString("layer_hide"));

					ImGui::SameLine();
					if (UI::transparentButton(ICON_FA_PENCIL_ALT,
					                          ImVec2(UI::btnSmall.x, layersButtonHeight), false))
					{
						renameIndex = index;
						layerName = layer.name;
						dialogOpen = true;
					}
					UI::tooltip(getString("layer_rename"));

					bool isFirst = index == 0;
					ImGui::SameLine();
					if (UI::transparentButton(ICON_FA_CHEVRON_UP,
					                          ImVec2(UI::btnSmall.x, layersButtonHeight), false,
					                          !isFirst))
						moveUpPattern = index;
					UI::tooltip(getString("layer_up"));

					int isLast = index == context.score.layers.size() - 1;
					ImGui::SameLine();
					if (UI::transparentButton(ICON_FA_CHEVRON_DOWN,
					                          ImVec2(UI::btnSmall.x, layersButtonHeight), false,
					                          !isLast))
						moveDownPattern = index;
					UI::tooltip(getString("layer_down"));

					ImGui::SameLine();
					if (UI::transparentButton(ICON_FA_LEVEL_DOWN_ALT,
					                          ImVec2(UI::btnSmall.x, layersButtonHeight), false,
					                          !isLast))
						mergePattern = index;
					UI::tooltip(getString("layer_merge"));

					ImGui::PopID();
				}
			}
			ImGui::EndChild();
			ImGui::Separator();

			if (ImGui::Button(getString("create_layer"), ImVec2(-1, layersButtonHeight)))
			{
				dialogOpen = true;
				renameIndex = -1;
				layerName.clear();
			}

			ImGui::PopStyleColor();

			if (moveUpPattern != -1)
			{
				Score prev = context.score;
				std::swap(context.score.layers[moveUpPattern],
				          context.score.layers[moveUpPattern - 1]);
				for (auto& [_, note] : context.score.notes)
				{
					if (note.layer == moveUpPattern)
						note.layer = moveUpPattern - 1;
					else if (note.layer == moveUpPattern - 1)
						note.layer = moveUpPattern;
				}
				for (auto& [_, hiSpeed] : context.score.hiSpeedChanges)
				{
					if (hiSpeed.layer == moveUpPattern)
						hiSpeed.layer = moveUpPattern - 1;
					else if (hiSpeed.layer == moveUpPattern - 1)
						hiSpeed.layer = moveUpPattern;
				}
				context.pushHistory("Change Layer Order", prev, context.score);
			}

			if (moveDownPattern != -1)
			{
				Score prev = context.score;
				std::swap(context.score.layers[moveDownPattern],
				          context.score.layers[moveDownPattern + 1]);
				for (auto& [_, note] : context.score.notes)
				{
					if (note.layer == moveDownPattern)
						note.layer = moveDownPattern + 1;
					else if (note.layer == moveDownPattern + 1)
						note.layer = moveDownPattern;
				}
				for (auto& [_, hiSpeed] : context.score.hiSpeedChanges)
				{
					if (hiSpeed.layer == moveDownPattern)
						hiSpeed.layer = moveDownPattern + 1;
					else if (hiSpeed.layer == moveDownPattern + 1)
						hiSpeed.layer = moveDownPattern;
				}
				context.pushHistory("Change Layer Order", prev, context.score);
			}

			if (mergePattern != -1)
			{
				Score prev = context.score;
				context.score.layers.erase(context.score.layers.begin() + mergePattern);
				for (auto& [_, note] : context.score.notes)
				{
					if (note.layer > mergePattern)
						note.layer -= 1;
				}
				for (auto& [_, hiSpeed] : context.score.hiSpeedChanges)
				{
					if (hiSpeed.layer > mergePattern)
						hiSpeed.layer -= 1;
				}
				if (context.selectedLayer > mergePattern)
					context.selectedLayer -= 1;
				context.pushHistory("Merge Layer", prev, context.score);
			}

			if (toggleHideIndex != -1)
			{
				Score prev = context.score;
				auto& layer = context.score.layers.at(toggleHideIndex);
				layer.hidden = !layer.hidden;
				context.pushHistory("Toggle Hide Layer", prev, context.score);
			}
		}

		ImGui::End();

		if (dialogOpen)
		{
			ImGui::OpenPopup(renameIndex >= 0 ? MODAL_TITLE("layer_rename")
			                                  : MODAL_TITLE("create_layer"));
			dialogOpen = false;
		}

		if (updateCreationDialog() == DialogResult::Ok)
		{
			if (renameIndex >= 0)
			{
				context.score.layers[renameIndex].name = layerName;
				renameIndex = -1;
			}
			else
			{
				context.score.layers.push_back(Layer{ layerName });

				int id = nextHiSpeedID++;
				context.score.hiSpeedChanges[id] = {
					id, 0, 1, static_cast<int>(context.score.layers.size()) - 1
				};
				layerName.clear();
			}
		}
	}

	DialogResult LayersWindow::updateCreationDialog()
	{
		DialogResult result = DialogResult::None;

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
		                        ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(renameIndex >= 0 ? MODAL_TITLE("layer_rename")
		                                            : MODAL_TITLE("create_layer"),
		                           NULL, ImGuiWindowFlags_NoResize))
		{
			ImVec2 padding = ImGui::GetStyle().WindowPadding;
			ImVec2 spacing = ImGui::GetStyle().ItemSpacing;

			float xPos = padding.x;
			float yPos = ImGui::GetWindowSize().y - UI::btnSmall.y - 2.0f - (padding.y * 2);

			ImGui::Text("%s", getString("name"));
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##layer_name", &layerName);

			ImVec2 btnSz{ (ImGui::GetContentRegionAvail().x - spacing.x - (padding.x * 0.5f)) /
				              2.0f,
				          ImGui::GetFrameHeight() };
			ImGui::SetCursorPos(ImVec2(xPos, yPos));
			if (ImGui::Button(getString("cancel"), btnSz))
			{
				result = DialogResult::Cancel;
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !layerName.size());
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1 - (0.5f * !layerName.size()));
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

	void WaypointsWindow::update(ScoreContext& context)
	{
		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_LOCATION_ARROW, "waypoints")))
		{
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			float waypointButtonHeight = ImGui::GetFrameHeight();
			float windowHeight = ImGui::GetContentRegionAvail().y - waypointButtonHeight * 1 -
			                     ImGui::GetStyle().WindowPadding.y * 2;

			if (ImGui::BeginChild("waypoints_child_window", ImVec2(-1, windowHeight), true))
			{
				int index = -1;
				for (const auto& waypoint : context.score.waypoints)
				{
					++index;
					ImGui::PushID(index);

					if (ImGui::Button(
					        waypoint.name.c_str(),
					        ImVec2(ImGui::GetContentRegionAvail().x, waypointButtonHeight)))
					{
						context.currentTick = waypoint.tick;
						scrollTimeline(context, waypoint.tick);
					}
					ImGui::PopID();
				}
			}
			ImGui::EndChild();
			ImGui::Separator();

			if (ImGui::Button(getString("create_waypoint"), ImVec2(-1, waypointButtonHeight)))
			{
				context.score.waypoints.push_back(Waypoint{ "New Waypoint", context.currentTick });
				std::sort(context.score.waypoints.begin(), context.score.waypoints.end(),
				          [](const Waypoint& a, const Waypoint& b) { return a.tick < b.tick; });
			}

			ImGui::PopStyleColor();
		}

		ImGui::End();
	}
}
