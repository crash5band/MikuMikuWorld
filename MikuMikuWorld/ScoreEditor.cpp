#include "ScoreEditor.h"
#include "ResourceManager.h"
#include "HistoryManager.h"
#include "Rendering/Renderer.h"
#include "Score.h"
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

namespace MikuMikuWorld
{
	ScoreEditor::ScoreEditor() : score{}, prevUpdateScore{},
		isHoveringNote{ false }, isHoldingNote{ false }, isMovingNote{ false }, currentMode{ 0 },
		dummy{ NoteType::Tap }, dummyStart{ NoteType::Hold }, dummyMid{ NoteType::HoldMid }, dummyEnd{ NoteType::HoldEnd }
	{
		drawHoldStepOutline = true;
		showRenderStats = true;
		defaultNoteWidth = 3;
		framebuffer = new Framebuffer(1080, 1920);

		time			= 0;
		songStart		= 0;
		songEnd			= 0;
		masterVolume	= 0.8f;
		bgmVolume		= 1.0f;
		seVolume		= 1.0f;
		playing			= false;
		playSE			= true;
		dragging		= false;
		hasEdit			= false;
		playStartTime	= 0;

		audio.initAudio();
		audio.setMasterVolume(masterVolume);
		audio.setBGMVolume(bgmVolume);
		audio.setSEVolume(seVolume);
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
		musicOffset = score.metadata.offset;
		audio.setBGMOffset(time, musicOffset);
	}

	void ScoreEditor::writeScoreMetadata()
	{
		score.metadata.title = workingData.title;
		score.metadata.author = workingData.designer;
		score.metadata.artist = workingData.artist;
		score.metadata.difficulty = workingData.difficulty;
		score.metadata.musicFile = File::getFilenameWithoutExtension(musicFile);
		score.metadata.offset = musicOffset;
	}

	void ScoreEditor::open()
	{
		std::string filename;
		if (FileDialog::openFile(filename, FileType::ScoreFile))
		{
			reset();
			score = loadScore(filename);
			readScoreMetadata();

			std::string title = windowTitle + File::getFilenameWithoutExtension(score.metadata.filename);
			setWindowTitle(title.c_str());
		}
	}

	void ScoreEditor::save()
	{
		if (score.metadata.filename.size())
		{
			writeScoreMetadata();
			saveScore(score, score.metadata.filename);
		}
		else
		{
			saveAs();
		}

		std::string title = windowTitle + File::getFilenameWithoutExtension(score.metadata.filename);
		setWindowTitle(title.c_str());
	}

	void ScoreEditor::saveAs()
	{
		std::string filename;
		if (FileDialog::saveFile(filename, FileType::ScoreFile))
		{
			if (!endsWith(filename, ".sus"))
				filename.append(".sus");

			score.metadata.filename = filename;
			writeScoreMetadata();
			saveScore(score, filename);
		}

		std::string title = windowTitle + File::getFilenameWithoutExtension(score.metadata.filename);
		setWindowTitle(title.c_str());
	}

	void ScoreEditor::reset()
	{
		int tick = currentTick;
		stop();

		currentTick = tick;
		selectedNotes.clear();
		history.clear();

		resetNextID();
		hasEdit = false;
		score = Score();

		setWindowTitle(windowTitleNew);
	}

	float ScoreEditor::getNoteYPosFromTick(int tick)
	{
		return canvasPos.y + tickToPosition(tick) - timelineOffset + canvasSize.y;
	}

	int ScoreEditor::snapTickFromPos(float posY)
	{
		return snapTick(positionToTick(posY), division);
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

	int ScoreEditor::positionToTick(float pos, int div)
	{
		return roundf(pos / (TICK_HEIGHT * zoom) / div);
	}

	float ScoreEditor::tickToPosition(int tick, int div)
	{
		return tick * div * TICK_HEIGHT * zoom;
	}

	int ScoreEditor::positionToLane(float pos)
	{
		return (pos - laneOffset) / laneWidth;
	}

	float ScoreEditor::laneToPosition(float lane)
	{
		return laneOffset + (lane * laneWidth);
	}

	void ScoreEditor::togglePlaying()
	{
		playing ^= true;
		if (playing)
		{
			playStartTime = time;
			audio.seekBGM(time);
			audio.reSync();
			audio.playBGM(time);
		}
		else
		{
			audio.clearEvents();
			audio.stopBGM();
		}
	}

	void ScoreEditor::stop()
	{
		playing = false;
		time = currentTick = 0;
		timelineOffset = timelineMinOffset;

		audio.clearEvents();
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
		centerCursor(1);
	}

	void ScoreEditor::previousTick()
	{
		currentTick = roundTickDown(currentTick, division);
		currentTick = std::max(currentTick - (TICKS_PER_BEAT / (division / 4)), 0);
		centerCursor(2);
	}

	void ScoreEditor::setLaneWidth(float width)
	{
		laneWidth = std::clamp((int)width, MIN_LANE_WIDTH, MAX_LANE_WIDTH);
	}

	void ScoreEditor::setNotesHeight(float height)
	{
		notesHeight = std::clamp((int)height, MIN_NOTES_HEIGHT, MAX_NOTES_HEIGHT);
	}

	void ScoreEditor::setZoom(float val)
	{
		int tick = positionToTick(timelineOffset - canvasSize.y);
		float x1 = canvasPos.y - tickToPosition(tick) + timelineOffset;

		zoom = std::clamp(val, MIN_ZOOM, MAX_ZOOM);

		float x2 = canvasPos.y - tickToPosition(tick) + timelineOffset;
		timelineOffset += x1 - x2;
	}

	void ScoreEditor::undo()
	{
		if (history.hasUndo())
		{
			score = history.undo();
			clearSelection();
		}
	}

	void ScoreEditor::redo()
	{
		if (history.hasRedo())
		{
			score = history.redo();
			clearSelection();
		}
	}

	void ScoreEditor::centerCursor(int mode)
	{
		cursorPos = tickToPosition(currentTick);
		float offset = canvasSize.y * 0.5f;

		bool exec = false;
		switch (mode)
		{
		case 0:
			exec = true;
			break;
		case 1:
			if (cursorPos >= timelineOffset - offset)
				exec = true;
			break;
		case 2:
			if (cursorPos <= timelineOffset - offset)
				exec = true;
			break;
		default:
			break;
		}

		if (exec)
			timelineOffset = cursorPos + offset;
	}

	void ScoreEditor::gotoMeasure(int measure)
	{
		if (measure < 0 || measure > 999)
			return;

		currentTick = measureToTicks(measure, TICKS_PER_BEAT, score.timeSignatures);
		centerCursor();
	}

	void ScoreEditor::updateNoteSE()
	{
		songPosLastFrame = songPos;

		if (audio.isMusicInitialized() && playing && audio.getAudioPosition() >= musicOffset && !audio.isMusicAtEnd())
			songPos = audio.getAudioPosition();
		else
			songPos = time;

		if (!playing)
			return;

		std::unordered_map<std::string, int> tickSEMap;

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
						audio.pushAudioEvent(se.c_str(), noteTime - playStartTime, -1, false);
						tickSEMap[key] = note.tick;
					}
				}

				if (note.getType() == NoteType::Hold)
				{
					float endTime = accumulateDuration(score.notes.at(score.holdNotes.at(note.ID).end).tick, TICKS_PER_BEAT, score.tempoChanges);
					audio.pushAudioEvent(note.critical ? "critical_connect" : "connect", noteTime - playStartTime, endTime - playStartTime + 0.02f, true);
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
							audio.pushAudioEvent(se.c_str(), noteTime - playStartTime, -1, false);
							tickSEMap[key] = note.tick;
						}
					}
				}

				// playback started mid-hold
				if (note.getType() == NoteType::Hold)
				{
					int endTick = score.notes.at(score.holdNotes.at(note.ID).end).tick;
					float endTime = accumulateDuration(endTick, TICKS_PER_BEAT, score.tempoChanges);

					if (noteTime <= time && endTime > time)
						audio.pushAudioEvent(note.critical ? "critical_connect" : "connect", std::max(0.0f, noteTime - playStartTime), endTime - playStartTime + 0.02f, true);
				}
			}
		}
	}

	void ScoreEditor::drawMeasures()
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		ImU32 bgColor = ImGui::GetColorU32(ImGuiCol_FrameBg);
		int texIndex = ResourceManager::getTexture("default");
		drawList->AddImage((void*)ResourceManager::textures[texIndex].getID(), canvasPos, canvasPos + canvasSize,
			ImVec2(0, 0), ImVec2(1, 1), 0xFF656565);
		//ImGui::RenderFrame(boundaries.Min, boundaries.Max, 0xffffffff, true, 1.0f);

		const float x1 = canvasPos.x + laneOffset;
		const float x2 = x1 + timelineWidth;

		int firstTick = std::max(0, positionToTick(timelineOffset - canvasSize.y));
		int lastTick = positionToTick(timelineOffset);
		int measure = accumulateMeasures(firstTick, TICKS_PER_BEAT, score.timeSignatures);
		firstTick = measureToTicks(measure, TICKS_PER_BEAT, score.timeSignatures);

		int subDiv = TICKS_PER_BEAT / (division < 192 ? (division / 4) : 1);
		int tsIndex = findTimeSignature(measure, score.timeSignatures);

		for (int tick = firstTick; tick <= lastTick; tick += subDiv)
		{
			const float y = canvasPos.y - tickToPosition(tick) + timelineOffset;
			
			int numBeats = beatsPerMeasure(score.timeSignatures[tsIndex]);
			int ticksPerMeasure = numBeats * TICKS_PER_BEAT;
			int measure = accumulateMeasures(tick, TICKS_PER_BEAT, score.timeSignatures);
			int measureTick = measureToTicks(measure, TICKS_PER_BEAT, score.timeSignatures);

			// time signature changes on current measure
			if (score.timeSignatures.find(measure) != score.timeSignatures.end())
				tsIndex = measure;

			int div = TICKS_PER_BEAT * numBeats / score.timeSignatures[tsIndex].numerator;

			if (tick == measureTick)
			{
				// new measure
				std::string measureStr = "#" + std::to_string(measure);
				const float txtPos = x1 - MEASURE_WIDTH - (ImGui::CalcTextSize(measureStr.c_str()).x * 0.5f);

				drawList->AddLine(ImVec2(x1 - MEASURE_WIDTH, y), ImVec2(x2 + MEASURE_WIDTH, y), measureColor, 1.5f);
				drawList->AddText(ImGui::GetFont(), 24.0f, ImVec2(txtPos, y), measureColor, measureStr.c_str());
			}
			else if (tick % div == 0)
			{
				drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y), divColor1, 1.0f);
			}
			else if (division < 192 && tick % subDiv == 0)
			{
				drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y), divColor2, 1.0f);
			}
		}
	}

	void ScoreEditor::drawLanes()
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		for (int l = 0; l <= NUM_LANES; ++l)
		{
			const float x = canvasPos.x + laneToPosition(l);
			const bool boldLane = !(l & 1);
			drawList->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasPos.y + canvasSize.y), boldLane ? measureColor : divColor2, boldLane ? 1.0f : 0.5f);
		}
	}

	void ScoreEditor::updateCursor()
	{
		hoverTick = snapTickFromPos(-mousePos.y);
		hoverLane = positionToLane(mousePos.x);
		if (ImGui::IsMouseClicked(0) && !isHoveringNote && mouseInTimeline && !playing &&
			!isAnyPopupOpen() && currentMode == TimelineMode::Select && ImGui::IsWindowFocused())
		{
			currentTick = hoverTick;
		}

		const float x1 = canvasPos.x + laneOffset;
		const float x2 = x1 + timelineWidth;
		const float y = canvasPos.y - tickToPosition(currentTick) + timelineOffset;
		const float triPtOffset = 10.0f;
		const float triXPos = x1 - 30.0f + triPtOffset + 2.0f;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddTriangleFilled(ImVec2(triXPos, y - triPtOffset), ImVec2(triXPos, y + triPtOffset), ImVec2(triXPos + (triPtOffset * 2), y), cursorColor);
		drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y), cursorColor, 3.0f);
	}

	void ScoreEditor::updateTempoChanges()
	{
		static float editBPM = 0;
		const float x1 = canvasPos.x + laneOffset;
		const float x2 = x1 + timelineWidth + MEASURE_WIDTH;
		
		ImVec2 removeBtnSz{ -1, btnSmall.y + 2 };
		int removeBPM = -1;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		for (int index = 0; index < score.tempoChanges.size(); ++index)
		{
			Tempo& tempo = score.tempoChanges[index];

			const float y = canvasPos.y - tickToPosition(tempo.tick) + timelineOffset;

			std::string bpmStr = formatString("%g", tempo.bpm) + " BPM";
			drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y), tempoColor, 2.0f);
			drawList->AddText(ImGui::GetFont(), 24.0f, ImVec2(x2 - MEASURE_WIDTH + 20.0f, y - 25.0f), tempoColor, bpmStr.c_str());

			std::string id = "bpm" + std::to_string(index);
			if (transparentButton2(id.c_str(), ImVec2(x2 - MEASURE_WIDTH, y - 50), ImVec2(100, 30)))
				editBPM = tempo.bpm;

			ImGui::SetNextWindowSize(ImVec2(250, -1), ImGuiCond_Always);
			if (ImGui::BeginPopupContextItem(std::to_string(index).c_str(), ImGuiPopupFlags_MouseButtonLeft | ImGuiPopupFlags_NoOpenOverExistingPopup))
			{
				ImGui::Text("Edit Tempo");
				ImGui::Separator();

				beginPropertyColumns();
				addReadOnlyProperty("Tick", std::to_string(tempo.tick));
				addFloatProperty("BPM", editBPM, "%g");
				endPropertyColumns();

				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					Score prev = score;
					tempo.bpm = std::max(abs(editBPM), 1.0f);

					history.pushHistory(History{ "Change tempo", prev, score });
				}

				// cannot remove the first tempo change
				if (tempo.tick != 0)
				{
					if (ImGui::Button("Remove", removeBtnSz))
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
			history.pushHistory("Remove tempo change", prev, score);
		}
	}

	void ScoreEditor::updateTimeSignatures()
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const float x1 = canvasPos.x + laneOffset - MEASURE_WIDTH;
		const float x2 = canvasPos.x + laneOffset + timelineWidth;
		const float btnX = x1 - MEASURE_WIDTH;

		ImVec2 removeBtnSz{ -1, btnSmall.y + 2 };
		int removeTS = -1;

		for (auto& it : score.timeSignatures)
		{
			const int ticks = measureToTicks(it.second.measure, TICKS_PER_BEAT, score.timeSignatures);
			const float y = canvasPos.y - tickToPosition(ticks) + timelineOffset;

			std::string tStr = std::to_string(it.second.numerator) + "/" + std::to_string(it.second.denominator);
			drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y), timeColor, 2.0f);
			drawList->AddText(ImGui::GetFont(), 24.0f, ImVec2(x1 - 20.0f, y - 25.0f), timeColor, tStr.c_str());

			static int editTsNum = 0;
			static int editTsDen = 0;
			std::string id = "ts-" + std::to_string(it.second.measure);
			if (transparentButton2(id.c_str(), ImVec2(btnX, y - 50), ImVec2(x1 - btnX + MEASURE_WIDTH, 30)))
			{
				editTsNum = it.second.numerator;
				editTsDen = it.second.denominator;
			}

			ImGui::SetNextWindowSize(ImVec2(250, -1), ImGuiCond_Always);
			std::string wId = "TS-" + std::to_string(it.second.measure);
			if (ImGui::BeginPopupContextItem(wId.c_str(), ImGuiPopupFlags_MouseButtonLeft | ImGuiPopupFlags_NoOpenOverExistingPopup))
			{
				ImGui::Text("Edit Time Signature");
				ImGui::Separator();

				beginPropertyColumns();
				addReadOnlyProperty("Measure", std::to_string(it.second.measure));
				if (addFractionProperty("Time Signature", editTsNum, editTsDen))
				{
					Score prev = score;
					it.second.numerator = std::clamp(abs(editTsNum), 1, 64);
					it.second.denominator = std::clamp(abs(editTsDen), 1, 64);

					history.pushHistory(History{ "Change time signature", prev, score });
				}
				endPropertyColumns();

				// cannot remove the first time signature
				if (it.second.measure != 0)
				{
					if (ImGui::Button("Remove", removeBtnSz))
					{
						ImGui::CloseCurrentPopup();
						removeTS = it.second.measure;
					}
				}

				ImGui::EndPopup();
			}
		}

		if (score.timeSignatures.find(removeTS) != score.timeSignatures.end())
		{
			Score prev = score;
			score.timeSignatures.erase(removeTS);
			history.pushHistory("Remove time signature", prev, score);
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
				std::unordered_set<int> sortHolds;
				for (auto& id : selectedNotes)
				{
					const Note& note = score.notes.at(id);
					if (note.getType() == NoteType::Hold)
						sortHolds.insert(note.ID);
					else if (note.getType() == NoteType::HoldMid || note.getType() == NoteType::HoldEnd)
						sortHolds.insert(note.parentID);
				}

				for (int id : sortHolds)
				{
					HoldNote& hold = score.holdNotes.at(id);
					Note& start = score.notes.at(id);
					Note& end = score.notes.at(hold.end);

					if (start.tick > end.tick)
					{
						// swap
						start.tick = start.tick ^ end.tick;
						end.tick = start.tick ^ end.tick;
						start.tick = start.tick ^ end.tick;
					}

					sortHoldSteps(score, score.holdNotes.at(id));
				}

				//if (holdLane != hoverLane || (!strcmp(id, "M") && (holdTick != hoverTick)))
				history.pushHistory(History{ "Update notes", prevUpdateScore, score });
				
				hasEdit = false;
			}
		}

		return false;
	}

	int ScoreEditor::findClosestHold()
	{
		float xt = laneToPosition(hoverLane);
		float yt = getNoteYPosFromTick(hoverTick);

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
			selectedNotes.clear();

		for (const auto& n : score.notes)
		{
			const Note& note = n.second;
			float x1 = laneToPosition(note.lane);
			float x2 = laneToPosition(note.lane + note.width);
			float y = -tickToPosition(note.tick);

			if (right > x1 && left < x2 && isWithinRange(y, top - 10.0f, bottom + 10.0f))
			{
				if (InputListener::isAltDown())
					selectedNotes.erase(note.ID);
				else
					selectedNotes.insert(note.ID);
			}
		}
	}

	bool ScoreEditor::isNoteInCanvas(const int tick)
	{
		const float y = getNoteYPosFromTick(tick);
		return y >= 0 && y <= canvasSize.y + canvasPos.y + 100;
	}

	void ScoreEditor::updateNotes(Renderer* renderer)
	{
		// directxmath dies
		if (canvasSize.y < 10 || canvasSize.x < 10)
			return;

		Shader* shader = ResourceManager::shaders[0];
		shader->use();
		shader->setMatrix4("projection", camera.getOffCenterOrthographicProjection(0, canvasSize.x, canvasPos.y, canvasPos.y + canvasSize.y));

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		framebuffer->bind();
		framebuffer->clear();
		renderer->beginBatch();

		for (auto it = score.notes.begin(); it != score.notes.end(); ++it)
		{
			Note& note = it->second;
			if (note.getType() == NoteType::Tap)
			{
				if (isNoteInCanvas(note.tick))
				{
					updateNote(note);
					drawNote(note, renderer, noteTint);
				}
			}
		}

		for (auto it = score.holdNotes.begin(); it != score.holdNotes.end(); ++it)
		{
			Note& start = score.notes.at(it->first);
			Note& end = score.notes.at(it->second.end);

			if (isNoteInCanvas(start.tick)) updateNote(start);
			if (isNoteInCanvas(end.tick)) updateNote(end);

			for (const auto& step : it->second.steps)
			{
				Note& mid = score.notes.at(step.ID);
				if (isNoteInCanvas(mid.tick)) updateNote(mid);
			}

			drawHoldNote(score.notes, it->second, renderer, noteTint);
		}

		renderer->endBatch();
#ifdef _DEBUG
		if (showRenderStats)
		{
			ImVec2 statPos = canvasPos;
			statPos.x += 10;
			statPos.y += 5;

			std::string vertexCount = "Vertices: " + std::to_string(renderer->getNumVertices());
			drawList->AddText(statPos, 0xffffffff, vertexCount.c_str());

			statPos.y += ImGui::GetFontSize();

			std::string QuadCount = "Quads: " + std::to_string(renderer->getNumQuads());
			drawList->AddText(statPos, 0xffffffff, QuadCount.c_str());

		}
#endif // DEBUG

		renderer->beginBatch();

		if (isPasting())
			previewPaste(renderer);
		
		// input note preview
		if (mouseInTimeline && !isHoldingNote && currentMode != TimelineMode::Select && !isPasting() && !isAnyPopupOpen())
		{
			// preview note
			updateDummyNotes();
			if (currentMode == TimelineMode::InsertLong)
			{
				drawDummyHold(renderer);
			}
			else if (currentMode == TimelineMode::InsertLongMid)
			{
				drawHoldMid(dummyMid, HoldStepType::Visible, renderer, hoverTint);
			}
			else if (currentMode == TimelineMode::InsertBPM)
			{
				const float x1 = canvasPos.x + laneOffset;
				const float x2 = x1 + timelineWidth;
				const float y = canvasPos.y - tickToPosition(hoverTick) + timelineOffset;
				drawList->AddLine(ImVec2(x1, y), ImVec2(x2 + MEASURE_WIDTH, y), tempoColor, 2.0f);
				drawList->AddText(ImGui::GetFont(), 24.0f, ImVec2(x2 + 20.0f, y), tempoColor, "BPM");
			}
			else if (currentMode == TimelineMode::InsertTimeSign)
			{
				const float x1 = canvasPos.x + laneOffset;
				const float x2 = x1 + timelineWidth;
				const float y = canvasPos.y - tickToPosition(hoverTick) + timelineOffset;
				drawList->AddLine(ImVec2(x1 - MEASURE_WIDTH - (ImGui::CalcTextSize("4/4").x * 0.5f), y), ImVec2(x2, y), timeColor, 2.0f);
				drawList->AddText(ImGui::GetFont(), 24.0f, ImVec2(x1 - 40.0f, y), timeColor, "4/4");
			}
			else
			{
				drawNote(dummy, renderer, hoverTint);
			}

			// input note
			if (ImGui::IsMouseClicked(0) && hoverTick >= 0 && !isHoveringNote)
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
				else
				{
					insertNote(currentMode == TimelineMode::MakeCritical);
				}
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
		drawList->AddImage((void*)fbTex, canvasPos, canvasPos + canvasSize);
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
		currentMode = mode;
		switch (currentMode)
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

		default:
			break;
		}
	}

	void ScoreEditor::drawSelectionRectangle()
	{
		float startX = std::min(canvasPos.x + dragStart.x, canvasPos.x + mousePos.x);
		float endX = std::max(canvasPos.x + dragStart.x, canvasPos.x + mousePos.x);
		float startY = std::min(canvasPos.y + dragStart.y, canvasPos.y + mousePos.y) + timelineOffset;
		float endY = std::max(canvasPos.y + dragStart.y, canvasPos.y + mousePos.y) + timelineOffset;
		
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(ImVec2(startX, startY), ImVec2(endX, endY), selectionColor1);

		ImVec2 iconPos = ImVec2(canvasPos + dragStart);
		iconPos.y += timelineOffset;
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
		for (const int& id : selectedNotes)
		{
			if (isNoteInCanvas(score.notes.at(id).tick))
				drawHighlight(score.notes.at(id), renderer, noteTint, false);
		}
		renderer->endBatch();
	}

	bool ScoreEditor::isHoldPathInTick(const Note& n1, const Note& n2, EaseType ease, float x, float y)
	{
		float xStart1 = laneToPosition(n1.lane);
		float xStart2 = laneToPosition(n1.lane + n1.width);
		float xEnd1 = laneToPosition(n2.lane);
		float xEnd2 = laneToPosition(n2.lane + n2.width);
		float y1 = getNoteYPosFromTick(n1.tick);
		float y2 = getNoteYPosFromTick(n2.tick);

		if (y > y2 || y < y1)
			return false;

		float percent = (y - y1) / (y2 - y1);
		float iPercent = ease == EaseType::None ? percent : ease == EaseType::EaseIn ? easeIn(percent) : easeOut(percent);
		float xl = lerp(xStart1, xEnd1, iPercent);
		float xr = lerp(xStart2, xEnd2, iPercent);

		return isWithinRange(x, xl < xr ? xl : xr, xr > xl ? xr : xl);
	}

	bool ScoreEditor::selectionHasEase()
	{
		for (const int id : selectedNotes)
			if (score.notes[id].hasEase())
				return true;

		return false;
	}

	bool ScoreEditor::selectionHasHoldStep()
	{
		for (const int id : selectedNotes)
			if (score.notes[id].getType() == NoteType::HoldMid)
				return true;

		return false;
	}
}