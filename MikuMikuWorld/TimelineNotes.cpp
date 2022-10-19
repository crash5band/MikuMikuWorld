#include "ScoreEditor.h"
#include "Rendering/Renderer.h"
#include "ResourceManager.h"
#include "InputListener.h"
#include "Score.h"
#include "Colors.h"
#include "ZIndex.h"
#include "Math.h"
#include "UI.h"
#include "StringOperations.h"
#include <algorithm>

namespace MikuMikuWorld
{
	void ScoreEditor::updateNote(Note& note)
	{
		const float btnPosY = canvas.getPosition().y - canvas.tickToPosition(note.tick) + canvas.getVisualOffset() - (canvas.getNotesHeight() * 0.5f);
		float btnPosX = canvas.laneToPosition(note.lane) + canvas.getPosition().x;

		ImVec2 pos{ btnPosX, btnPosY };
		ImVec2 sz{ noteCtrlWidth, noteCtrlHeight };

		if (mousePos.y >= (btnPosY - canvas.getVisualOffset() - canvas.getPosition().y - 2.0f) && mousePos.y <= (btnPosY + noteCtrlHeight - canvas.getVisualOffset() - canvas.getPosition().y + 2.0f)
			&& hoverLane >= note.lane && hoverLane <= note.lane + note.width - 1)
		{
			isHoveringNote = true;
			float noteYDistance = std::abs((btnPosY + noteCtrlHeight / 2 - canvas.getVisualOffset() - canvas.getPosition().y) - mousePos.y);
			if (InputListener::isCtrlDown() || noteYDistance < minNoteYDistance)
			{
				minNoteYDistance = noteYDistance;
				hoveringNote = note.ID;

				if (ImGui::IsMouseClicked(0) && !UI::isAnyPopupOpen())
				{
					if (!InputListener::isCtrlDown() && !InputListener::isAltDown() && !selection.hasNote(note.ID))
						selection.clear();

					selection.append(note.ID);

					if (InputListener::isAltDown() && selection.hasNote(note.ID))
						selection.remove(note.ID);
				}
			}
		}

		// left resize
		ImGui::PushID(note.ID);
		if (noteControl(pos, sz, "L", ImGuiMouseCursor_ResizeEW))
		{
			int curLane = canvas.positionToLane(mousePos.x);
			int grabLane = std::clamp(canvas.positionToLane(ctrlMousePos.x), MIN_LANE, MAX_LANE);
			int diff = curLane - grabLane;

			if (abs(diff) > 0 && curLane >= MIN_LANE && curLane <= MAX_LANE)
			{
				ctrlMousePos.x = mousePos.x;
				hasEdit = true;

				for (int id : selection.getSelection())
				{
					Note& n = score.notes.at(id);
					int newLane = n.lane + diff;
					n.lane = std::clamp(n.lane + (n.width < 2 && diff > 0 ? 0 : diff), MIN_LANE, MAX_LANE);
					if (newLane >= 0)
						n.setWidth(n.width - diff);
				}
			}
		}

		pos.x += noteCtrlWidth;
		sz.x = (canvas.getLaneWidth() * note.width) - (noteCtrlWidth * 2.0f);

		// move
		if (noteControl(pos, sz, "M", ImGuiMouseCursor_ResizeAll))
		{
			int curLane = std::clamp(canvas.positionToLane(mousePos.x), MIN_LANE, MAX_LANE);
			int grabLane = std::clamp(canvas.positionToLane(ctrlMousePos.x), MIN_LANE, MAX_LANE);
			int grabTick = snapTickFromPos(-ctrlMousePos.y);

			int diff = curLane - grabLane;
			if (abs(diff) > 0 && curLane >= MIN_LANE && curLane <= MAX_LANE)
			{
				isMovingNote = true;
				hasEdit = true;
				ctrlMousePos.x = mousePos.x;

				for (int id : selection.getSelection())
				{
					Note& n = score.notes.at(id);
					n.setLane(n.lane + diff);
				}
			}

			diff = hoverTick - grabTick;
			if (abs(diff) > 0)
			{
				isMovingNote = true;
				hasEdit = true;
				ctrlMousePos.y = mousePos.y;

				for (int id : selection.getSelection())
				{
					Note& n = score.notes.at(id);
					n.tick = std::max(n.tick + diff, 0);
				}
			}
		}

		// per note options here
		if (ImGui::IsItemDeactivated())
		{
			if (!isMovingNote && selection.hasSelection())
			{
				switch (currentMode)
				{
				case TimelineMode::InsertFlick:
					cycleFlicks();
					break;

				case TimelineMode::MakeCritical:
					toggleCriticals();
					break;

				case TimelineMode::InsertLong:
				case TimelineMode::InsertLongMid:
					if (InputListener::isAltDown())
						cycleStepType();
					else
						cycleEase();
					break;

				default:
					break;
				}
			}

			isMovingNote = false;
		}

		pos.x += sz.x;
		sz.x = noteCtrlWidth;

		// right resize
		if (noteControl(pos, sz, "R", ImGuiMouseCursor_ResizeEW))
		{
			int grabLane = std::clamp(canvas.positionToLane(ctrlMousePos.x), MIN_LANE, MAX_LANE);
			int curLane = canvas.positionToLane(mousePos.x);

			int diff = curLane - grabLane;
			if (abs(diff) > 0 && curLane >= MIN_LANE && curLane <= MAX_LANE)
			{
				ctrlMousePos.x = mousePos.x;
				isMovingNote = true;
				hasEdit = true;

				for (int id : selection.getSelection())
				{
					Note& n = score.notes.at(id);
					int numLanes = curLane - n.width - n.lane + 1;
					n.setWidth(n.width + diff);
				}
			}
		}

		ImGui::PopID();
	}

	void ScoreEditor::drawHoldCurve(const Note& n1, const Note& n2, EaseType ease, Renderer* renderer, const Color& tint, const int offsetTick, const int offsetLane)
	{
		float startX1 = canvas.laneToPosition(n1.lane + offsetLane);
		float startX2 = canvas.laneToPosition(n1.lane + n1.width + offsetLane);
		float startY = canvas.getNoteYPosFromTick(n1.tick + offsetTick);

		float endX1 = canvas.laneToPosition(n2.lane + offsetLane);
		float endX2 = canvas.laneToPosition(n2.lane + n2.width + offsetLane);
		float endY = canvas.getNoteYPosFromTick(n2.tick + offsetTick);

		int texIndex = ResourceManager::getTexture(n1.critical ? HOLD_PATH_CRTCL_TEX : HOLD_PATH_TEX);
		if (texIndex == -1)
			return;

		Texture& pathTex = ResourceManager::textures[texIndex];

		float steps = std::ceilf(abs((endY - startY)) / 10);
		for (int y = 0; y < steps; ++y)
		{
			const float percent1 = y / steps;
			const float percent2 = (y + 1) / steps;
			float i1 = ease == EaseType::None ? percent1 : ease == EaseType::EaseIn ? easeIn(percent1) : easeOut(percent1);
			float i2 = ease == EaseType::None ? percent2 : ease == EaseType::EaseIn ? easeIn(percent2) : easeOut(percent2);

			float xl1 = lerp(startX1, endX1, i1) - NOTES_SLICE_WIDTH;
			float xr1 = lerp(startX2, endX2, i1) + NOTES_SLICE_WIDTH;
			float y1 = lerp(startY, endY, percent1);
			float y2 = lerp(startY, endY, percent2);
			float xl2 = lerp(startX1, endX1, i2) - NOTES_SLICE_WIDTH;
			float xr2 = lerp(startX2, endX2, i2) + NOTES_SLICE_WIDTH;

			if (y2 <= 0)
				continue;

			// rest of hold no longer visible
			if (y1 > canvas.getSize().y + canvas.getSize().y + canvas.getPosition().y + 100)
				break;

			Vector2 p1{ xl1, y1 };
			Vector2 p2{ xl1 + NOTES_SLICE_WIDTH, y1 };
			Vector2 p3{ xl2, y2 };
			Vector2 p4{ xl2 + NOTES_SLICE_WIDTH, y2 };
			renderer->drawQuad(p1, p2, p3, p4, pathTex, 0, HOLD_X_SLICE,
				0, pathTex.getHeight(), tint);

			p1.x = xl1 + NOTES_SLICE_WIDTH;
			p2.x = xr1 - NOTES_SLICE_WIDTH;
			p3.x = xl2 + NOTES_SLICE_WIDTH;
			p4.x = xr2 - NOTES_SLICE_WIDTH;
			renderer->drawQuad(p1, p2, p3, p4, pathTex, HOLD_X_SLICE,
				pathTex.getWidth() - HOLD_X_SLICE, 0, pathTex.getHeight(), tint);

			p1.x = xr1 - NOTES_SLICE_WIDTH;
			p2.x = xr1;
			p3.x = xr2 - NOTES_SLICE_WIDTH;
			p4.x = xr2;
			renderer->drawQuad(p1, p2, p3, p4, pathTex, pathTex.getWidth() - HOLD_X_SLICE,
				pathTex.getWidth(), 0, pathTex.getHeight(), tint);
		}
	}

	void ScoreEditor::drawDummyHold(Renderer* renderer)
	{
		if (insertingHold)
		{
			drawHoldCurve(dummyStart, dummyEnd, EaseType::None, renderer, noteTint);
			drawNote(dummyStart, renderer, noteTint);
			drawNote(dummyEnd, renderer, noteTint);
		}
		else
		{
			drawNote(dummyStart, renderer, hoverTint);
		}
	}

	void ScoreEditor::drawHoldNote(const std::unordered_map<int, Note>& notes, const HoldNote& note, Renderer* renderer,
		const Color& tint, const int offsetTicks, const int offsetLane)
	{
		const Note& start = notes.at(note.start.ID);
		const Note& end = notes.at(note.end);
		if (note.steps.size())
		{
			int s1 = -1;
			int s2 = -1;

			for (int i = 0; i < note.steps.size(); ++i)
			{
				HoldStepType type = note.steps[i].type;
				if (type == HoldStepType::Ignored)
					continue;

				s2 = i;
				const Note& n1 = s1 == -1 ? start : notes.at(note.steps[s1].ID);
				const Note& n2 = s2 == -1 ? end : notes.at(note.steps[s2].ID);
				const EaseType ease = s1 == -1 ? note.start.ease : note.steps[s1].ease;
				drawHoldCurve(n1, n2, ease, renderer, tint, offsetTicks, offsetLane);

				s1 = s2;
			}

			const Note& n1 = s1 == -1 ? start : notes.at(note.steps[s1].ID);
			const EaseType ease = s1 == -1 ? note.start.ease : note.steps[s1].ease;
			drawHoldCurve(n1, end, ease, renderer, tint, offsetTicks, offsetLane);

			s1 = -1;
			s2 = 1;

			int texIndex = ResourceManager::getTexture(NOTES_TEX);
			if (texIndex != -1)
			{
				const Texture& tex = ResourceManager::textures[texIndex];
				const Vector2 nodeSz{ canvas.getLaneWidth(), canvas.getLaneWidth() };
				for (int i = 0; i < note.steps.size(); ++i)
				{
					const Note& n3 = notes.at(note.steps[i].ID);
					// find first non-ignored step
					if (s2 < i)
						s2 = i;

					while (s2 < note.steps.size())
					{
						if (note.steps[s2].type != HoldStepType::Ignored)
							break;
						++s2;
					}

					if (canvas.isNoteInCanvas(n3.tick + offsetTicks))
					{
						if (drawHoldStepOutline)
						{
							drawSteps.emplace_back(StepDrawData{n3.tick + offsetTicks, n3.lane + offsetLane, n3.width, note.steps[i].type});
						}

						if (note.steps[i].type != HoldStepType::Invisible)
						{
							int sprIndex = getNoteSpriteIndex(n3);
							if (sprIndex > -1 && sprIndex < tex.sprites.size())
							{
								const Sprite& s = tex.sprites[sprIndex];
								Vector2 pos{
									canvas.getNotePos(n3.tick + offsetTicks, n3.lane + offsetLane + ((n3.width) / 2.0f))
								};

								if (note.steps[i].type == HoldStepType::Ignored)
								{
									const Note& n1 = s1 == -1 ? start : notes.at(note.steps[s1].ID);
									const Note& n2 = s2 >= note.steps.size() ? end : notes.at(note.steps[s2].ID);

									float ratio = (float)(n3.tick - n1.tick) / (float)(n2.tick - n1.tick);
									const EaseType rEase = s1 == -1 ? note.start.ease : note.steps[s1].ease;
									float i1 = rEase == EaseType::None ? ratio : rEase == EaseType::EaseIn ? easeIn(ratio) : easeOut(ratio);

									float x1 = lerp(canvas.laneToPosition(n1.lane + offsetLane), canvas.laneToPosition(n2.lane + offsetLane), i1);
									float x2 = lerp(canvas.laneToPosition(n1.lane + offsetLane + n1.width), canvas.laneToPosition(n2.lane + offsetLane + n2.width), i1);
									pos.x = (x1 + x2) / 2.0f;
								}

								renderer->drawSprite(pos, 0.0f, nodeSz, AnchorType::MiddleCenter, tex, s.getX(), s.getX() + s.getWidth(),
									s.getY(), s.getHeight(), tint, 1);
							}
						}
					}

					if (note.steps[i].type != HoldStepType::Ignored)
						s1 = i;
				}
			}
		}
		else
		{
			drawHoldCurve(start, end, note.start.ease, renderer, tint, offsetTicks, offsetLane);
		}

		if (canvas.isNoteInCanvas(start.tick + offsetTicks))
			drawNote(start, renderer, tint, offsetTicks, offsetLane);

		if (canvas.isNoteInCanvas(end.tick + offsetTicks))
			drawNote(end, renderer, tint, offsetTicks, offsetLane);
	}

	void ScoreEditor::drawHoldMid(Note& note, HoldStepType type, Renderer* renderer, const Color& tint)
	{
		if (type == HoldStepType::Invisible)
			return;

		const int texIndex = ResourceManager::getTexture(NOTES_TEX);
		if (texIndex == -1)
			return;

		const Texture& tex = ResourceManager::textures[texIndex];
		int sprIndex = getNoteSpriteIndex(note);
		if (sprIndex > -1 && sprIndex < tex.sprites.size())
		{
			const Sprite& s = tex.sprites[sprIndex];
			// center diamond if width is even
			Vector2 pos {
				canvas.getNotePos(note.tick, note.lane + ((note.width) / 2.0f))
			};

			Vector2 nodeSz {
				canvas.getLaneWidth(), canvas.getLaneWidth()
			};

			renderer->drawSprite(pos, 0.0f, nodeSz, AnchorType::MiddleCenter,
				tex, s.getX(), s.getX() + s.getWidth(), s.getY(), s.getHeight(), tint, 1);
		}
	}

	void ScoreEditor::drawOutline(const StepDrawData& data)
	{
		float x = canvas.getPosition().x;
		float y = canvas.getPosition().y - canvas.tickToPosition(data.tick) + canvas.getVisualOffset();

		ImVec2 p1 {
			x + canvas.laneToPosition(data.lane),
			y - (canvas.getNotesHeight() * 0.15f)
		};

		ImVec2 p2 {
			x + canvas.laneToPosition(data.lane + data.width),
			y + (canvas.getNotesHeight() * 0.15f)
		};

		ImU32 fill = data.type == HoldStepType::Ignored ? 0x55ffffaa : 0x55aaffaa;
		ImU32 outline = data.type == HoldStepType::Ignored ? 0xffffffaa : 0xffccffaa;
		ImGui::GetWindowDrawList()->AddRectFilled(p1, p2, fill, 3.0f, ImDrawFlags_RoundCornersAll);
		ImGui::GetWindowDrawList()->AddRect(p1, p2, outline, 3.0f, ImDrawFlags_RoundCornersAll, 3.0f);
	}

	void ScoreEditor::drawFlickArrow(const Note& note, Renderer* renderer, const Color& tint, const int offsetTick, const int offsetLane)
	{
		const int texIndex = ResourceManager::getTexture(NOTES_TEX);
		if (texIndex != -1)
		{
			const Texture& tex = ResourceManager::textures[texIndex];
			const int sprIndex = getFlickArrowSpriteIndex(note);
			if (sprIndex > -1 && sprIndex < tex.sprites.size())
			{
				const Sprite& arrowS = tex.sprites[sprIndex];

				Vector2 pos{ 0, canvas.getNoteYPosFromTick(note.tick + offsetTick) };
				const float x1 = canvas.laneToPosition(note.lane + offsetLane);
				const float x2 = pos.x + canvas.laneToPosition(note.lane + note.width + offsetLane);
				pos.x = (x1 + x2) * 0.5f;
				pos.y += (canvas.getNotesHeight() * 0.55f) + 10.0f;

				// notes wider than 6 lanes also use arrow size 6
				Vector2 midSz(canvas.getLaneWidth() * (note.flick == FlickType::Up ? 1.0f : 1.05f) * std::min(note.width, 6), canvas.getNotesHeight() - 5);
				midSz.x -= NOTES_SLICE_WIDTH * 1.28f * std::min(note.width - 1, 5);
				midSz.y += (note.flick == FlickType::Up ? 3.0f : 4.0f) * std::min(note.width, 6);

				float sx1 = arrowS.getX();
				float sx2 = arrowS.getX() + arrowS.getWidth();
				if (note.flick == FlickType::Right)
				{
					sx1 = arrowS.getX() + arrowS.getWidth();
					sx2 = arrowS.getX();
				}

				renderer->drawSprite(pos, 0.0f, midSz, AnchorType::MiddleCenter, tex,
					sx1, sx2, arrowS.getY(), arrowS.getY() + arrowS.getHeight(), tint, 2);
			}
		}
	}

	void ScoreEditor::drawNote(const Note& note, Renderer* renderer, const Color& tint, const int offsetTick, const int offsetLane)
	{
		Vector2 pos{ canvas.getNotePos(note.tick + offsetTick, note.lane + offsetLane) };
		const Vector2 sliceSz(NOTES_SLICE_WIDTH, canvas.getNotesHeight());
		const AnchorType anchor = AnchorType::MiddleLeft;

		const int texIndex = ResourceManager::getTexture(NOTES_TEX);
		if (texIndex != -1)
		{
			const Texture& tex = ResourceManager::textures[texIndex];
			const int sprIndex = getNoteSpriteIndex(note);
			if (sprIndex > -1 && sprIndex < tex.sprites.size())
			{
				const Sprite& s = tex.sprites[sprIndex];

				const float midLen = (canvas.getLaneWidth() * note.width) - (NOTES_SLICE_WIDTH * 2) + NOTES_X_ADJUST + 5;
				const Vector2 midSz(midLen, canvas.getNotesHeight());

				pos.x -= NOTES_X_ADJUST;

				// left slice
				renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex,
					s.getX() + NOTES_SIDE_WIDTH, s.getX() + NOTES_X_SLICE,
					s.getY(), s.getY() + s.getHeight(), tint, 1
				);
				pos.x += NOTES_SLICE_WIDTH;

				// middle
				renderer->drawSprite(pos, 0.0f, midSz, anchor, tex,
					s.getX() + NOTES_X_SLICE + 10, s.getX() + NOTES_X_SLICE + 20,
					s.getY(), s.getY() + s.getHeight(), tint, 1
				);
				pos.x += midLen;

				// right slice
				renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex,
					s.getX() + s.getWidth() - NOTES_X_SLICE, s.getX() + s.getWidth() - NOTES_SIDE_WIDTH,
					s.getY(), s.getY() + s.getHeight(), tint, 1
				);
			}
		}

		if (note.isFlick())
			drawFlickArrow(note, renderer, tint, offsetTick, offsetLane);
	}

	bool ScoreEditor::bpmControl(const Tempo& tempo)
	{
		return bpmControl(tempo.bpm, tempo.tick, true);
	}

	bool ScoreEditor::bpmControl(float bpm, int tick, bool enabled)
	{
		return eventControl(tick, false, true, tempoColor, formatString("%g BPM", bpm).c_str(), enabled);
	}

	bool ScoreEditor::timeSignatureControl(const TimeSignature& ts)
	{
		int tick = measureToTicks(ts.measure, TICKS_PER_BEAT, score.timeSignatures);
		return timeSignatureControl(ts.numerator, ts.denominator, tick, true);
	}

	bool ScoreEditor::timeSignatureControl(int numerator, int denominator, int tick, bool enabled)
	{
		return eventControl(tick, true, true, timeColor, formatString("%d/%d", numerator, denominator).c_str(), enabled);
	}

	bool ScoreEditor::skillControl(const SkillTrigger& skill)
	{
		return skillControl(skill.tick, true);
	}

	bool ScoreEditor::skillControl(int tick, bool enabled)
	{
		return eventControl(tick, true, false, skillColor, "Skill", enabled);
	}

	bool ScoreEditor::feverControl(const Fever& fever)
	{
		return feverControl(fever.startTick, true, true) || feverControl(fever.endTick, false, true);
	}

	bool ScoreEditor::feverControl(int tick, bool start, bool enabled)
	{
		if (tick < 0)
			return false;

		std::string txt = "FEVER";
		txt.append(start ? ICON_FA_CHEVRON_UP : ICON_FA_CHEVRON_DOWN);

		return eventControl(tick, false, false, feverColor, txt.c_str(), enabled);
	}

	bool ScoreEditor::eventControl(int tick, bool left, bool up, ImU32 color, const char* txt, bool enabled)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return false;

		float x1 = canvas.getTimelineStartX() - MEASURE_WIDTH;
		float x2 = canvas.getTimelineStartX();
		if (!left)
		{
			x1 = canvas.getTimelineEndX();
			x2 = canvas.getTimelineEndX() + MEASURE_WIDTH;
		}

		float y = canvas.getPosition().y - canvas.tickToPosition(tick) + canvas.getVisualOffset();
		float yOffset = 32.0f * (up ? -1 : -2);
		drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y), color, 2.0f);

		float txtSize = std::max(ImGui::CalcTextSize(txt).x + 8.0f, 50.0f);
		float x = (left ? x1 - txtSize + 10.0f : x2 - 15.0f);

		ImGui::PushID(tick);
		bool pressed = UI::coloredButton(txt, ImVec2{ x, y + yOffset }, ImVec2{ -1, -1 }, color, enabled);
		ImGui::PopID();
		return pressed;
	}
}
