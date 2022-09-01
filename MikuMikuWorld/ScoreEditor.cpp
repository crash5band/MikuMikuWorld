#include "ScoreEditor.h"
#include "ResourceManager.h"
#include "HistoryManager.h"
#include "Rendering/Renderer.h"
#include "Score.h"
#include "SUSIO.h"
#include "Utilities.h"
#include "IconsFontAwesome5.h"
#include "Colors.h"
#include "InputListener.h"
#include "UI.h"
#include "FileDialog.h"
#include "StringOperations.h"
#include <string>
#include <math.h>
#include <map>
#include <unordered_set>
#include <algorithm>
#include <filesystem>
#include <Windows.h>

#undef min
#undef max

namespace MikuMikuWorld
{
	ScoreEditor::ScoreEditor() : score{}, prevUpdateScore{},
		isHoveringNote{ false }, isHoldingNote{ false }, isMovingNote{ false }, currentMode{ 0 },
		dummy{ NoteType::Tap }, dummyStart{ NoteType::Hold }, dummyMid{ NoteType::HoldMid }, dummyEnd{ NoteType::HoldEnd }
	{
		drawHoldStepOutline = true;
		showRenderStats = true;

		defaultNoteWidth = 3;
		defaultStepType = HoldStepType::Visible;
		defaultBPM = 160.0f;
		defaultTimeSignN = defaultTimeSignD = 4;

		skipUpdateAfterSortingSteps = false;
		framebuffer = new Framebuffer(1080, 1920);

		time = 0;
		masterVolume = 0.8f;
		bgmVolume = 1.0f;
		seVolume = 1.0f;
		playing = false;
		dragging = false;
		hasEdit = false;
		playStartTime = 0;

		pasting = flipPasting = insertingPreset = false;

		audio.initAudio();
		audio.setMasterVolume(masterVolume);
		audio.setBGMVolume(bgmVolume);
		audio.setSEVolume(seVolume);

		uptoDate = true;
	}

	ScoreEditor::~ScoreEditor()
	{
		audio.uninitAudio();
	}

	void ScoreEditor::readScoreMetadata()
	{
		workingData.title = score.metadata.title;
		workingData.designer = score.metadata.author;
		workingData.artist = score.metadata.artist;
		workingData.musicOffset = score.metadata.musicOffset;

		loadMusic(score.metadata.musicFile);
		audio.setBGMOffset(time, workingData.musicOffset);
	}

	void ScoreEditor::writeScoreMetadata()
	{
		score.metadata.title = workingData.title;
		score.metadata.author = workingData.designer;
		score.metadata.artist = workingData.artist;
		score.metadata.musicFile = workingData.musicFilename;
		score.metadata.musicOffset = workingData.musicOffset;
	}

	void ScoreEditor::loadScore(const std::string& filename)
	{
		resetEditor();

		std::string extension = File::getFileExtension(filename);
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
		std::string title = windowUntitled;

		if (extension == SUS_EXTENSION)
		{
			SUSIO susIO;
			score = susIO.importSUS(filename);
			workingData.filename = "";

			// project not saved
			uptoDate = false;
		}
		else if (extension == MMWS_EXTENSION)
		{
			Score backup = score;
			try
			{
				score = deserializeScore(filename);
				workingData.filename = filename;
				title = File::getFilenameWithoutExtension(filename);
			}
			catch (std::runtime_error& err)
			{
				MessageBox(NULL, err.what(), APP_NAME, MB_OK | MB_ICONERROR);
				score = Score{};
			}
		}

		readScoreMetadata();
		stats.calculateStats(score);
		UI::setWindowTitle(title);
	}

	void ScoreEditor::loadMusic(const std::string& filename)
	{
		audio.changeBGM(filename);
		workingData.musicFilename = filename;
	}

	void ScoreEditor::open()
	{
		std::string filename;
		if (FileDialog::openFile(filename, FileType::ScoreFile))
			loadScore(filename);
	}

	void ScoreEditor::save()
	{
		if (workingData.filename.size())
		{
			writeScoreMetadata();
			serializeScore(score, workingData.filename);
			uptoDate = true;
			UI::setWindowTitle(File::getFilenameWithoutExtension(workingData.filename));
		}
		else
		{
			saveAs();
		}
	}

	void ScoreEditor::save(const std::string& filename)
	{
		writeScoreMetadata();
		serializeScore(score, filename);
	}

	void ScoreEditor::saveAs()
	{
		std::string filename;
		if (FileDialog::saveFile(filename, FileType::MMWSFile))
		{
			workingData.filename = filename;
			writeScoreMetadata();
			serializeScore(score, filename);
			uptoDate = true;

			UI::setWindowTitle(File::getFilenameWithoutExtension(workingData.filename));
		}
	}

	void ScoreEditor::exportSUS()
	{
		std::string filename;
		if (FileDialog::saveFile(filename, FileType::SUSFile))
		{
			writeScoreMetadata();
			SUSIO susIO;
			susIO.exportSUS(score, filename);
		}
	}

	void ScoreEditor::reset()
	{
		resetEditor();
		UI::setWindowTitle(windowUntitled);
	}

	void ScoreEditor::resetEditor()
	{
		playing = false;
		audio.stopSounds(false);
		audio.stopBGM();

		selection.clear();
		history.clear();
		resetNextID();

		workingData = EditorScoreData{};
		score = Score{};
		stats.reset();

		hasEdit = false;
		uptoDate = true;
	}

	bool ScoreEditor::isUptoDate() const
	{
		return uptoDate;
	}

	int ScoreEditor::snapTickFromPos(float posY)
	{
		return snapTick(canvas.positionToTick(posY), division);
	}

	int ScoreEditor::snapTick(int tick, int div)
	{
		int half = (TICKS_PER_BEAT / (div / 4)) / 2;
		int remaining = tick % (TICKS_PER_BEAT / (div / 4));

		// round to closest division
		tick -= remaining;
		if (remaining >= half)
			tick += half * 2;

		return std::max(tick, 0);
	}

	int ScoreEditor::roundTickDown(int tick, int div)
	{
		return std::max(tick - (tick % (TICKS_PER_BEAT / (div / 4))), 0);
	}

	int ScoreEditor::laneFromCenterPos(int lane, int width)
	{
		return std::clamp(lane - (width / 2), MIN_LANE, MAX_LANE - width + 1);
	}

	void ScoreEditor::togglePlaying()
	{
		playing ^= true;
		if (playing)
		{
			audio.seekBGM(time);
			audio.reSync();
			audio.playBGM(time);
			playStartTime = time;
		}
		else
		{
			audio.stopSounds(false);
			audio.stopBGM();
		}
	}

	void ScoreEditor::stop()
	{
		playing = false;
		time = currentTick = 0;

		canvas.scrollToBeginning();
		audio.stopSounds(false);
		audio.stopBGM();
	}

	void ScoreEditor::restart()
	{
		stop();
		togglePlaying();
	}

	bool ScoreEditor::isPlaying()
	{
		return playing;
	}

	void ScoreEditor::nextTick()
	{
		currentTick = roundTickDown(currentTick, division);
		currentTick += TICKS_PER_BEAT / (division / 4);
		canvas.centerCursor(currentTick, playing, 1);
	}

	void ScoreEditor::previousTick()
	{
		currentTick = roundTickDown(currentTick, division);
		currentTick = std::max(currentTick - (TICKS_PER_BEAT / (division / 4)), 0);
		canvas.centerCursor(currentTick, playing, 2);
	}

	void ScoreEditor::setDivision(int div)
	{
		const int length = sizeof(divisions) / sizeof(int);
		for (int i = 0; i < length - 1; ++i)
		{
			if (divisions[i] == div)
			{
				selectedDivision = i;
				division = divisions[i];
			}
		}
	}

	void ScoreEditor::pushHistory(const std::string& description, const Score& prev, const Score& curr)
	{
		history.pushHistory(description, prev, curr);

		if (uptoDate)
			UI::setWindowTitle("*" + (workingData.filename.size() ? File::getFilenameWithoutExtension(workingData.filename) : windowUntitled));

		uptoDate = false;
		stats.calculateStats(score);
	}

	void ScoreEditor::undo()
	{
		if (history.hasUndo())
		{
			score = history.undo();
			clearSelection();
			uptoDate = false;

			stats.calculateStats(score);
		}
	}

	void ScoreEditor::redo()
	{
		if (history.hasRedo())
		{
			score = history.redo();
			clearSelection();
			uptoDate = false;

			stats.calculateStats(score);
		}
	}

	void ScoreEditor::gotoMeasure(int measure)
	{
		if (measure < 0 || measure > 999)
			return;

		currentTick = measureToTicks(measure, TICKS_PER_BEAT, score.timeSignatures);
		canvas.centerCursor(currentTick, playing, 0);
	}

	void ScoreEditor::updateNoteSE()
	{
		songPosLastFrame = songPos;

		if (audio.isMusicInitialized() && playing && audio.getAudioPosition() >= workingData.musicOffset && !audio.isMusicAtEnd())
			songPos = audio.getAudioPosition();
		else
			songPos = time;

		if (!playing)
			return;

		tickSEMap.clear();
		for (const auto& it : score.notes)
		{
			const Note& note = it.second;
			float noteTime = accumulateDuration(note.tick, TICKS_PER_BEAT, score.tempoChanges);
			float offsetNoteTime = noteTime - audioLookAhead;

			if (offsetNoteTime >= songPosLastFrame && offsetNoteTime < songPos)
			{
				std::string se = getNoteSE(note, score);
				std::string key = std::to_string(note.tick) + "-" + se;
				if (se.size());
				{
					if (tickSEMap.find(key) == tickSEMap.end())
					{
						audio.playSound(se.c_str(), noteTime - playStartTime, -1);
						tickSEMap[key] = note.tick;
					}
				}

				if (note.getType() == NoteType::Hold)
				{
					float endTime = accumulateDuration(score.notes.at(score.holdNotes.at(note.ID).end).tick, TICKS_PER_BEAT, score.tempoChanges);
					audio.playSound(note.critical ? SE_CRITICAL_CONNECT : SE_CONNECT, noteTime - playStartTime, endTime - playStartTime + 0.02f);
				}
			}
			else if (time == playStartTime)
			{
				// playback just started
				if (noteTime >= time && offsetNoteTime < time)
				{
					std::string se = getNoteSE(note, score);
					std::string key = std::to_string(note.tick) + "-" + se;
					if (se.size())
					{
						if (tickSEMap.find(key) == tickSEMap.end())
						{
							audio.playSound(se.c_str(), noteTime - playStartTime, -1);
							tickSEMap[key] = note.tick;
						}
					}
				}

				// playback started mid-hold
				if (note.getType() == NoteType::Hold)
				{
					int endTick = score.notes.at(score.holdNotes.at(note.ID).end).tick;
					float endTime = accumulateDuration(endTick, TICKS_PER_BEAT, score.tempoChanges);

					if ((noteTime - time) <= audioLookAhead && endTime > time)
						audio.playSound(note.critical ? SE_CRITICAL_CONNECT : SE_CONNECT, std::max(0.0f, noteTime - playStartTime), endTime - playStartTime + 0.02f);
				}
			}
		}
	}

	void ScoreEditor::drawMeasures()
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		const float x1 = canvas.getTimelineStartX();
		const float x2 = canvas.getTimelineEndX();

		int firstTick = std::max(0, canvas.positionToTick(canvas.getVisualOffset() - canvas.getSize().y));
		int lastTick = canvas.positionToTick(canvas.getVisualOffset());
		int measure = accumulateMeasures(firstTick, TICKS_PER_BEAT, score.timeSignatures);
		firstTick = measureToTicks(measure, TICKS_PER_BEAT, score.timeSignatures);

		int tsIndex = findTimeSignature(measure, score.timeSignatures);
		int subDiv = TICKS_PER_BEAT / (division < 192 ? (division / 4) : 1);
		int div = TICKS_PER_BEAT;

		for (int tick = firstTick; tick <= lastTick; tick += subDiv)
		{
			const float y = canvas.getPosition().y - canvas.tickToPosition(tick) + canvas.getVisualOffset();
			int measure = accumulateMeasures(tick, TICKS_PER_BEAT, score.timeSignatures);

			// time signature changes on current measure
			if (score.timeSignatures.find(measure) != score.timeSignatures.end())
				tsIndex = measure;

			if (!(tick % div))
				drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y), divColor1, primaryLineThickness);
			else if (division < 192)
				drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y), divColor2, secondaryLineThickness);
		}

		tsIndex = findTimeSignature(measure, score.timeSignatures);
		int ticksPerMeasure = beatsPerMeasure(score.timeSignatures[tsIndex]) * TICKS_PER_BEAT;

		for (int tick = firstTick; tick < lastTick; tick += ticksPerMeasure)
		{
			if (score.timeSignatures.find(measure) != score.timeSignatures.end())
			{
				tsIndex = measure;
				ticksPerMeasure = beatsPerMeasure(score.timeSignatures[tsIndex]) * TICKS_PER_BEAT;
			}

			std::string measureStr = "#" + std::to_string(measure);
			const float txtPos = x1 - MEASURE_WIDTH - (ImGui::CalcTextSize(measureStr.c_str()).x * 0.5f);
			const float y = canvas.getPosition().y - canvas.tickToPosition(tick) + canvas.getVisualOffset();

			drawList->AddLine(ImVec2(x1 - MEASURE_WIDTH, y), ImVec2(x2 + MEASURE_WIDTH, y), measureColor, 1.5f);
			drawList->AddText(ImGui::GetFont(), 24.0f, ImVec2(txtPos, y), measureColor, measureStr.c_str());

			++measure;
		}
	}

	void ScoreEditor::drawLanes()
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		for (int l = 0; l <= NUM_LANES; ++l)
		{
			const float x = canvas.getPosition().x + canvas.laneToPosition(l);
			const bool boldLane = !(l & 1);
			const ImU32 color = boldLane ? divColor1 : divColor2;
			const float thickness = boldLane ? primaryLineThickness : secondaryLineThickness;
			drawList->AddLine(ImVec2(x, canvas.getPosition().y), ImVec2(x, canvas.getPosition().y + canvas.getSize().y), color, secondaryLineThickness);
		}
	}

	void ScoreEditor::updateCursor()
	{
		hoverTick = snapTickFromPos(-mousePos.y);
		hoverLane = canvas.positionToLane(mousePos.x);
		if (ImGui::IsMouseClicked(0) && !isHoveringNote && canvas.isMouseInCanvas() && !playing &&
			!UI::isAnyPopupOpen() && currentMode == TimelineMode::Select && ImGui::IsWindowFocused())
		{
			currentTick = hoverTick;
		}

		const float x1 = canvas.getTimelineStartX();
		const float x2 = canvas.getTimelineEndX();
		const float y = canvas.getPosition().y - canvas.tickToPosition(currentTick) + canvas.getVisualOffset();
		const float triPtOffset = 8.0f;
		const float triXPos = x1 - (triPtOffset * 2);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddTriangleFilled(ImVec2(triXPos, y - triPtOffset), ImVec2(triXPos, y + triPtOffset), ImVec2(triXPos + (triPtOffset * 2), y), cursorColor);
		drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y), cursorColor, primaryLineThickness + 1.0f);
	}

	void ScoreEditor::updateTempoChanges()
	{
		static float editBPM = 0;
		int removeBPM = -1;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		for (int index = 0; index < score.tempoChanges.size(); ++index)
		{
			Tempo& tempo = score.tempoChanges[index];
			drawBPM(tempo);

			const float y = canvas.getPosition().y - canvas.tickToPosition(tempo.tick) + canvas.getVisualOffset();
			std::string id = "bpm" + std::to_string(index);
			if (UI::transparentButton2(id.c_str(), ImVec2(canvas.getTimelineEndX(), y - 30.0f), ImVec2(100.0f, 30.0f)))
				editBPM = tempo.bpm;

			ImGui::SetNextWindowSize(ImVec2(250, -1), ImGuiCond_Always);
			if (ImGui::BeginPopupContextItem(std::to_string(index).c_str(), ImGuiPopupFlags_MouseButtonLeft | ImGuiPopupFlags_NoOpenOverExistingPopup))
			{
				ImGui::Text("Edit Tempo");
				ImGui::Separator();

				UI::beginPropertyColumns();
				UI::addReadOnlyProperty("Tick", std::to_string(tempo.tick));
				UI::addFloatProperty("BPM", editBPM, "%g");
				UI::endPropertyColumns();

				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					Score prev = score;
					tempo.bpm = std::clamp(editBPM, MIN_BPM, MAX_BPM);

					pushHistory("Change tempo", prev, score);
				}

				// cannot remove the first tempo change
				if (tempo.tick != 0)
				{
					if (ImGui::Button("Remove", ImVec2(-1, UI::btnNormal.y)))
					{
						ImGui::CloseCurrentPopup();
						removeBPM = index;
					}
				}

				ImGui::EndPopup();
			}
		}

		if (removeBPM != -1)
		{
			Score prev = score;
			score.tempoChanges.erase(score.tempoChanges.begin() + removeBPM);
			pushHistory("Remove tempo change", prev, score);
		}
	}

	void ScoreEditor::updateTimeSignatures()
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		static int editTsNum = 0;
		static int editTsDen = 0;
		int removeTS = -1;

		for (auto& [measure, ts] : score.timeSignatures)
		{
			const int ticks = measureToTicks(measure, TICKS_PER_BEAT, score.timeSignatures);
			drawTimeSignature(ts);
			const float y = canvas.getPosition().y - canvas.tickToPosition(ticks) + canvas.getVisualOffset();

			std::string id = "ts-" + std::to_string(measure);
			if (UI::transparentButton2(id.c_str(), ImVec2(canvas.getTimelineStartX() - 60.0f, y - 30.0f), ImVec2(60.0f, 30.0f)))
			{
				// save current time signature
				editTsNum = ts.numerator;
				editTsDen = ts.denominator;
			}

			ImGui::SetNextWindowSize(ImVec2(250, -1), ImGuiCond_Always);
			std::string wId = "TS-" + std::to_string(measure);
			if (ImGui::BeginPopupContextItem(wId.c_str(), ImGuiPopupFlags_MouseButtonLeft | ImGuiPopupFlags_NoOpenOverExistingPopup))
			{
				ImGui::Text("Edit Time Signature");
				ImGui::Separator();

				UI::beginPropertyColumns();
				UI::addReadOnlyProperty("Measure", std::to_string(measure));
				if (UI::addFractionProperty("Time Signature", editTsNum, editTsDen))
				{
					Score prev = score;
					ts.numerator = std::clamp(abs(editTsNum), MIN_TIME_SIGN, MAX_TIME_SIGN);
					ts.denominator = std::clamp(abs(editTsDen), MIN_TIME_SIGN, MAX_TIME_SIGN);

					pushHistory("Change time signature", prev, score);
				}
				UI::endPropertyColumns();

				// cannot remove the first time signature
				if (measure != 0)
				{
					if (ImGui::Button("Remove", ImVec2(-1, UI::btnNormal.y)))
					{
						ImGui::CloseCurrentPopup();
						removeTS = measure;
					}
				}

				ImGui::EndPopup();
			}
		}

		if (score.timeSignatures.find(removeTS) != score.timeSignatures.end())
		{
			Score prev = score;
			score.timeSignatures.erase(removeTS);
			pushHistory("Remove time signature", prev, score);
		}
	}

	void ScoreEditor::updateSkills()
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		int removeSkill = -1;

		for (const auto& skill : score.skills)
		{
			drawSkill(skill);

			const float y = canvas.getPosition().y - canvas.tickToPosition(skill.tick) + canvas.getVisualOffset();
			std::string id = "skill-" + std::to_string(skill.ID);
			UI::transparentButton2(id.c_str(), ImVec2(canvas.getTimelineStartX() - 60.0f, y - 30.0f), ImVec2(60.0f, 30.0f));

			ImGui::SetNextWindowSize(ImVec2(250, -1), ImGuiCond_Always);
			if (ImGui::BeginPopupContextItem(id.c_str(), ImGuiPopupFlags_MouseButtonLeft | ImGuiPopupFlags_NoOpenOverExistingPopup))
			{
				ImGui::Text("Edit Skill");
				ImGui::Separator();

				UI::beginPropertyColumns();
				UI::addReadOnlyProperty("Tick", std::to_string(skill.tick));
				UI::endPropertyColumns();

				if (ImGui::Button("Remove", ImVec2(-1, UI::btnNormal.y)))
				{
					removeSkill = skill.ID;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}

		if (removeSkill != -1)
		{
			Score prev = score;
			for (auto it = score.skills.begin(); it != score.skills.end(); ++it)
			{
				if (it->ID == removeSkill)
				{
					score.skills.erase(it);
					break;
				}
			}

			history.pushHistory("Remove skill", prev, score);
		}
	}

	bool ScoreEditor::noteControl(const ImVec2& pos, const ImVec2& sz, const char* id, ImGuiMouseCursor cursor)
	{
		ImGui::SetCursorScreenPos(pos);
		ImGui::InvisibleButton(id, sz);
		if (ImGui::IsItemHovered())
			ImGui::SetMouseCursor(cursor);

		// note clicked
		if (ImGui::IsItemActivated())
		{
			prevUpdateScore = score;
			ctrlMousePos = mousePos;
			holdLane = hoverLane;
			holdTick = hoverTick;
		}

		// holding note
		if (ImGui::IsItemActive())
		{
			ImGui::SetMouseCursor(cursor);
			isHoldingNote = true;
			return true;
		}

		// note released
		if (ImGui::IsItemDeactivated())
		{
			isHoldingNote = false;
			if (hasEdit)
			{
				// only need to sort if the notes are moved
				if (!strcmp(id, "M") && holdTick != hoverTick)
				{
					std::unordered_set<int> sortHolds = selection.getHolds(score);
					for (int id : sortHolds)
					{
						HoldNote& hold = score.holdNotes.at(id);
						Note& start = score.notes.at(id);
						Note& end = score.notes.at(hold.end);

						if (start.tick > end.tick)
							std::swap(start.tick, end.tick);

						sortHoldSteps(score, hold);
						skipUpdateAfterSortingSteps = true;
					}
				}

				pushHistory("Update notes", prevUpdateScore, score);
				hasEdit = false;
			}
		}

		return false;
	}

	int ScoreEditor::findClosestHold()
	{
		float xt = canvas.laneToPosition(hoverLane);
		float yt = canvas.getNoteYPosFromTick(hoverTick);

		for (auto it = score.holdNotes.begin(); it != score.holdNotes.end(); ++it)
		{
			const HoldNote& hold = it->second;
			const Note& start = score.notes.at(hold.start.ID);
			const Note& end = score.notes.at(hold.end);
			const int ID = hold.start.ID;

			if (hold.steps.size())
			{
				const HoldStep& mid1 = hold.steps[0];
				if (isHoldPathInTick(start, score.notes.at(mid1.ID), hold.start.ease, xt, yt))
					return ID;

				for (int step = 0; step < hold.steps.size() - 1; ++step)
				{
					const Note& m1 = score.notes.at(hold.steps[step].ID);
					const Note& m2 = score.notes.at(hold.steps[step + 1].ID);
					if (isHoldPathInTick(m1, m2, hold.steps[step].ease, xt, yt))
						return ID;
				}

				const Note& lastMid = score.notes.at(hold.steps[hold.steps.size() - 1].ID);
				if (isHoldPathInTick(lastMid, end, hold.steps[hold.steps.size() - 1].ease, xt, yt))
					return ID;
			}
			else
			{
				if (isHoldPathInTick(start, end, hold.start.ease, xt, yt))
					return ID;
			}
		}

		return -1;
	}

	void ScoreEditor::calcDragSelection()
	{
		float left = std::min(dragStart.x, mousePos.x);
		float right = std::max(dragStart.x, mousePos.x);
		float top = std::min(dragStart.y, mousePos.y);
		float bottom = std::max(dragStart.y, mousePos.y);

		if (!InputListener::isAltDown() && !InputListener::isCtrlDown())
			selection.clear();

		for (const auto& n : score.notes)
		{
			const Note& note = n.second;
			float x1 = canvas.laneToPosition(note.lane);
			float x2 = canvas.laneToPosition(note.lane + note.width);
			float y = -canvas.tickToPosition(note.tick);

			if (right > x1 && left < x2 && isWithinRange(y, top - 10.0f, bottom + 10.0f))
			{
				if (InputListener::isAltDown())
					selection.remove(note.ID);
				else
					selection.append(note.ID);
			}
		}
	}

	void ScoreEditor::updateNotes(Renderer* renderer)
	{
		// directxmath dies
		if (canvas.getSize().y < 10 || canvas.getSize().x < 10)
			return;

		Shader* shader = ResourceManager::shaders[0];
		shader->use();
		shader->setMatrix4("projection", camera.getOffCenterOrthographicProjection(0, canvas.getSize().x, canvas.getPosition().y, canvas.getPosition().y + canvas.getSize().y));

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		framebuffer->bind();
		framebuffer->clear();
		renderer->beginBatch();

		for (auto& [id, note] : score.notes)
		{
			if (canvas.isNoteInCanvas(note.tick) && note.getType() == NoteType::Tap)
			{
				updateNote(note);
				drawNote(note, renderer, noteTint);
			}
		}

		for (auto& [id, hold] : score.holdNotes)
		{
			Note& start = score.notes.at(hold.start.ID);
			Note& end = score.notes.at(hold.end);

			if (canvas.isNoteInCanvas(start.tick)) updateNote(start);
			if (canvas.isNoteInCanvas(end.tick)) updateNote(end);

			for (const auto& step : hold.steps)
			{
				Note& mid = score.notes.at(step.ID);
				if (canvas.isNoteInCanvas(mid.tick)) updateNote(mid);

				if (skipUpdateAfterSortingSteps)
				{
					skipUpdateAfterSortingSteps = false;
					break;
				}
			}

			drawHoldNote(score.notes, hold, renderer, noteTint);
		}

		renderer->endBatch();
		renderer->beginBatch();

		if (isPasting() || insertingPreset && canvas.isMouseInCanvas())
			previewPaste(renderer);

		if (canvas.isMouseInCanvas() && !isHoldingNote && currentMode != TimelineMode::Select &&
			!isPasting() && !insertingPreset && !UI::isAnyPopupOpen())
		{
			updateDummyNotes();
			previewInput(renderer);

			if (ImGui::IsMouseClicked(0) && hoverTick >= 0 && !isHoveringNote)
			{
				executeInput();
			}

			if (insertingHold)
			{
				if (ImGui::IsMouseDown(0))
					updateDummyHold();
				else
				{
					insertHoldNote();
					insertingHold = false;
				}
			}
		}
		else
		{
			insertingHold = false;
		}

		renderer->endBatch();
		drawSelectionBoxes(renderer);

		glDisable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		unsigned int fbTex = framebuffer->getTexture();
		drawList->AddImage((void*)fbTex, canvas.getPosition(), canvas.getPosition() + canvas.getSize());
	}

	void ScoreEditor::previewInput(Renderer* renderer)
	{
		if (currentMode == TimelineMode::InsertLong)
		{
			drawDummyHold(renderer);
		}
		else if (currentMode == TimelineMode::InsertLongMid)
		{
			drawHoldMid(dummyMid, defaultStepType, renderer, hoverTint);
		}
		else if (currentMode == TimelineMode::InsertBPM)
		{
			drawBPM(defaultBPM, hoverTick);
		}
		else if (currentMode == TimelineMode::InsertTimeSign)
		{
			drawTimeSignature(defaultTimeSignN, defaultTimeSignD, hoverTick);
		}
		else if (currentMode == TimelineMode::InsertSkill)
		{
			drawSkill(hoverTick);
		}
		else
		{
			drawNote(dummy, renderer, hoverTint);
		}
	}

	void ScoreEditor::executeInput()
	{
		if (currentMode == TimelineMode::InsertLong)
		{
			insertingHold = true;
		}
		else if (currentMode == TimelineMode::InsertLongMid)
		{
			int id = findClosestHold();
			if (id != -1)
				insertHoldStep(score.holdNotes.at(id));
		}
		else if (currentMode == TimelineMode::InsertBPM)
		{
			insertTempo();
		}
		else if (currentMode == TimelineMode::InsertTimeSign)
		{
			insertTimeSignature();
		}
		else if (currentMode == TimelineMode::InsertSkill)
		{
			insertSkill();
		}
		else
		{
			insertNote(currentMode == TimelineMode::MakeCritical);
		}
	}

	void ScoreEditor::updateDummyNotes()
	{
		dummy.lane = laneFromCenterPos(hoverLane, defaultNoteWidth);
		dummy.tick = hoverTick;
		dummy.width = defaultNoteWidth;
		dummyMid.lane = dummy.lane;
		dummyMid.tick = dummy.tick;
		dummyMid.width = dummy.width;
		if (!insertingHold)
		{
			dummyStart.lane = dummyEnd.lane = dummy.lane;
			dummyStart.width = dummyEnd.width = dummy.width;
			dummyStart.tick = dummyEnd.tick = dummy.tick;
		}
	}

	void ScoreEditor::updateDummyHold()
	{
		dummyEnd.lane = laneFromCenterPos(hoverLane, dummyEnd.width);
		dummyEnd.tick = hoverTick;
	}

	void ScoreEditor::changeMode(TimelineMode mode)
	{
		switch (mode)
		{
		case TimelineMode::InsertTap:
			dummy.flick = FlickType::None;
			dummy.critical = false;
			break;

		case TimelineMode::InsertFlick:
			dummy.flick = FlickType::Up;
			dummy.critical = false;
			break;

		case TimelineMode::MakeCritical:
			dummy.flick = FlickType::None;
			dummy.critical = true;
			break;

			// #C02: Save selected hold step type
		case TimelineMode::InsertLongMid:
			if (currentMode == TimelineMode::InsertLongMid)
				defaultStepType = (HoldStepType)(((int)defaultStepType + 1) % 3);
			break;

		default:
			break;
		}
		currentMode = mode;
	}

	void ScoreEditor::drawSelectionRectangle()
	{
		float startX = std::min(canvas.getPosition().x + dragStart.x, canvas.getPosition().x + mousePos.x);
		float endX = std::max(canvas.getPosition().x + dragStart.x, canvas.getPosition().x + mousePos.x);
		float startY = std::min(canvas.getPosition().y + dragStart.y, canvas.getPosition().y + mousePos.y) + canvas.getVisualOffset();
		float endY = std::max(canvas.getPosition().y + dragStart.y, canvas.getPosition().y + mousePos.y) + canvas.getVisualOffset();

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(ImVec2(startX, startY), ImVec2(endX, endY), selectionColor1);

		ImVec2 iconPos = ImVec2(canvas.getPosition() + dragStart);
		iconPos.y += canvas.getVisualOffset();
		if (InputListener::isCtrlDown())
		{
			drawList->AddText(ImGui::GetFont(), 12, iconPos, 0xdddddddd, ICON_FA_PLUS_CIRCLE);
		}
		else if (InputListener::isAltDown())
		{
			drawList->AddText(ImGui::GetFont(), 12, iconPos, 0xdddddddd, ICON_FA_MINUS_CIRCLE);
		}
	}

	void ScoreEditor::drawSelectionBoxes(Renderer* renderer)
	{
		renderer->beginBatch();
		for (const int& id : selection.getSelection())
		{
			if (canvas.isNoteInCanvas(score.notes.at(id).tick))
				drawHighlight(score.notes.at(id), renderer, noteTint, false);
		}
		renderer->endBatch();
	}

	bool ScoreEditor::isHoldPathInTick(const Note& n1, const Note& n2, EaseType ease, float x, float y)
	{
		float xStart1 = canvas.laneToPosition(n1.lane);
		float xStart2 = canvas.laneToPosition(n1.lane + n1.width);
		float xEnd1 = canvas.laneToPosition(n2.lane);
		float xEnd2 = canvas.laneToPosition(n2.lane + n2.width);
		float y1 = canvas.getNoteYPosFromTick(n1.tick);
		float y2 = canvas.getNoteYPosFromTick(n2.tick);

		if (y > y2 || y < y1)
			return false;

		float percent = (y - y1) / (y2 - y1);
		float iPercent = ease == EaseType::None ? percent : ease == EaseType::EaseIn ? easeIn(percent) : easeOut(percent);
		float xl = lerp(xStart1, xEnd1, iPercent);
		float xr = lerp(xStart2, xEnd2, iPercent);

		return isWithinRange(x, xl < xr ? xl : xr, xr > xl ? xr : xl);
	}

	std::string ScoreEditor::getWorkingFilename() const
	{
		return workingData.filename;
	}
}
