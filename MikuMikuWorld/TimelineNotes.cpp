#include "ScoreEditor.h"
#include "Rendering/Renderer.h"
#include "ResourceManager.h"
#include "InputListener.h"
#include "Score.h"
#include "Colors.h"
#include "ZIndex.h"
#include "Math.h"
#include <algorithm>

namespace MikuMikuWorld
{
	/// <summary>
	/// TODO: improve note selection
	/// </summary>
	/// <param name="note"></param>
	void ScoreEditor::updateNote(Note& note)
	{
		const float btnPosY = canvasPos.y - tickToPosition(note.tick) + timelineOffset - (notesHeight * 0.25f);
		float btnPosX = laneToPosition(note.lane) + canvasPos.x;

		ImVec2 pos{ btnPosX, btnPosY };
		ImVec2 sz{ noteCtrlWidth, noteCtrlHeight };

		if (mousePos.y >= (btnPosY - timelineOffset - canvasPos.y - 2.0f) && mousePos.y <= (btnPosY + noteCtrlHeight - timelineOffset - canvasPos.y + 2.0f)
			&& hoverLane >= note.lane && hoverLane <= note.lane + note.width - 1)
		{
			isHoveringNote = true;
			hoveringNote = note.ID;
			if (ImGui::IsMouseClicked(0))
			{
				if (!InputListener::isCtrlDown())
					selectedNotes.clear();
				
				selectedNotes.insert(note.ID);
			}
		}

		// left resize
		ImGui::PushID(note.ID);
		if (noteControl(pos, sz, "L", ImGuiMouseCursor_ResizeEW))
		{
			int curLane = positionToLane(mousePos.x);
			int grabLane = std::clamp(positionToLane(ctrlMousePos.x), MIN_LANE, MAX_LANE);
			int diff = curLane - grabLane;

			if (abs(diff) > 0 && curLane >= MIN_LANE && curLane <= MAX_LANE)
			{
				ctrlMousePos.x = mousePos.x;
				hasEdit = true;

				for (int id : selectedNotes)
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
		sz.x = (laneWidth * note.width) - (noteCtrlWidth * 2.0f);

		// move
		if (noteControl(pos, sz, "M", ImGuiMouseCursor_ResizeAll))
		{
			int curLane = std::clamp(positionToLane(mousePos.x), MIN_LANE, MAX_LANE);
			int grabLane = std::clamp(positionToLane(ctrlMousePos.x), MIN_LANE, MAX_LANE);
			int grabTick = snapTickFromPos(-ctrlMousePos.y);

			int diff = curLane - grabLane;
			if (abs(diff) > 0 && curLane >= MIN_LANE && curLane <= MAX_LANE)
			{
				isMovingNote = true;
				hasEdit = true;
				ctrlMousePos.x = mousePos.x;

				for (int id : selectedNotes)
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

				// TODO: sort hold note steps
				for (int id : selectedNotes)
				{
					Note& n = score.notes.at(id);
					n.tick = std::max(n.tick + diff, 0);
				}
			}
		}

		// per note options here
		if (ImGui::IsItemDeactivated())
		{
			if (!isMovingNote && selectedNotes.size())
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
						toggleStepVisibility();
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
			int grabLane = std::clamp(positionToLane(ctrlMousePos.x), MIN_LANE, MAX_LANE);
			int curLane = positionToLane(mousePos.x);

			int diff = curLane - grabLane;
			if (abs(diff) > 0 && curLane >= MIN_LANE && curLane <= MAX_LANE)
			{
				ctrlMousePos.x = mousePos.x;
				isMovingNote = true;
				hasEdit = true;

				for (int id : selectedNotes)
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
		float startX1 = laneToPosition(n1.lane + offsetLane);
		float startX2 = laneToPosition(n1.lane + n1.width + offsetLane);
		float startY = getNoteYPosFromTick(n1.tick + offsetTick);

		float endX1 = laneToPosition(n2.lane + offsetLane);
		float endX2 = laneToPosition(n2.lane + n2.width + offsetLane);
		float endY = getNoteYPosFromTick(n2.tick + offsetTick);

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
			if (y1 > canvasSize.y + canvasSize.y + canvasPos.y + 100)
				break;

			Vector2 p1{ xl1, y1 };
			Vector2 p2{ xl1 + NOTES_SLICE_WIDTH, y1 };
			Vector2 p3{ xl2, y2 };
			Vector2 p4{ xl2 + NOTES_SLICE_WIDTH, y2 };
			renderer->drawRectangle(p1, p2, p3, p4, pathTex, 0, HOLD_X_SLICE, 0, pathTex.getHeight(), tint);

			p1.x = xl1 + NOTES_SLICE_WIDTH;
			p2.x = xr1 - NOTES_SLICE_WIDTH;
			p3.x = xl2 + NOTES_SLICE_WIDTH;
			p4.x = xr2 - NOTES_SLICE_WIDTH;
			renderer->drawRectangle(p1, p2, p3, p4, pathTex, HOLD_X_SLICE, pathTex.getWidth() - HOLD_X_SLICE, 0, pathTex.getHeight(), tint);

			p1.x = xr1 - NOTES_SLICE_WIDTH;
			p2.x = xr1;
			p3.x = xr2 - NOTES_SLICE_WIDTH;
			p4.x = xr2;
			renderer->drawRectangle(p1, p2, p3, p4, pathTex, pathTex.getWidth() - HOLD_X_SLICE, pathTex.getWidth(), 0, pathTex.getHeight(), tint);
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
				const Vector2 nodeSz{ laneWidth, laneWidth };
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

					if (isNoteInCanvas(n3.tick))
					{
						if (drawHoldStepOutline)
							drawHighlight(n3, renderer, tint, true, offsetTicks, offsetLane);

						if (note.steps[i].type != HoldStepType::Invisible)
						{
							const Sprite& s = tex.sprites[getNoteSpriteIndex(n3)];
							Vector2 pos{
								laneToPosition(n3.lane + offsetLane + ((n3.width) / 2.0f)),
								getNoteYPosFromTick(n3.tick + offsetTicks)
							};

							if (note.steps[i].type == HoldStepType::Ignored)
							{
								const Note& n1 = s1 == -1 ? start : notes.at(note.steps[s1].ID);
								const Note& n2 = s2 >= note.steps.size() ? end : notes.at(note.steps[s2].ID);

								float ratio = (float)(n3.tick - n1.tick + offsetTicks) / (float)(n2.tick - n1.tick + offsetTicks);
								const EaseType rEase = s1 == -1 ? note.start.ease : note.steps[s1].ease;
								float i1 = rEase == EaseType::None ? ratio : rEase == EaseType::EaseIn ? easeIn(ratio) : easeOut(ratio);

								float x1 = lerp(laneToPosition(n1.lane + offsetLane), laneToPosition(n2.lane + offsetLane), i1);
								float x2 = lerp(laneToPosition(n1.lane + offsetLane + n1.width), laneToPosition(n2.lane + offsetLane + n2.width), i1);
								pos.x = (x1 + x2) / 2.0f;
							}

							renderer->drawSprite(pos, 0.0f, nodeSz, AnchorType::MiddleCenter, tex, s.getX(), s.getX() + s.getWidth(),
								s.getY(), s.getHeight(), tint, 1);
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

		if (isNoteInCanvas(start.tick + offsetTicks)) drawNote(start, renderer, tint, offsetTicks, offsetLane);
		if (isNoteInCanvas(end.tick + offsetTicks)) drawNote(end, renderer, tint, offsetTicks, offsetLane);
	}

	void ScoreEditor::drawHighlight(const Note& note, Renderer* renderer, const Color& tint, bool mid, const int offsetTick, const int offsetLane)
	{
		Vector2 pos(laneToPosition(note.lane + offsetLane), getNoteYPosFromTick(note.tick + offsetTick));
		const Vector2 sliceSz(NOTES_SLICE_WIDTH, mid ? HIGHLIGHT_HEIGHT : notesHeight + 5);
		const AnchorType anchor = AnchorType::MiddleLeft;

		const int texIndex = ResourceManager::getTexture(HIGHLIGHT_TEX);
		if (texIndex != -1)
		{
			const Texture& tex = ResourceManager::textures[texIndex];
			const Sprite& s = tex.sprites[mid ? 1 : 0];

			const float midLen = (laneWidth * note.width) - (NOTES_SLICE_WIDTH * 2) + NOTES_X_ADJUST + 5;
			const Vector2 midSz(midLen, mid ? HIGHLIGHT_HEIGHT : notesHeight + 5);

			pos.x -= NOTES_X_ADJUST;

			// left slice
			renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex,
				s.getX(), s.getX() + HIGHLIGHT_SLICE,
				s.getY(), s.getY() + s.getHeight(), tint
			);
			pos.x += NOTES_SLICE_WIDTH;

			// middle
			renderer->drawSprite(pos, 0.0f, midSz, anchor, tex,
				s.getX() + HIGHLIGHT_SLICE, s.getX() + s.getWidth() - HIGHLIGHT_SLICE,
				s.getY(), s.getY() + s.getHeight(), tint
			);
			pos.x += midLen;

			// right slice
			renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex,
				s.getX() + s.getWidth() - HIGHLIGHT_SLICE, s.getX() + s.getWidth(),
				s.getY(), s.getY() + s.getHeight(), tint
			);
		}
	}

	void ScoreEditor::drawHoldMid(const Note& note, HoldStepType type, Renderer* renderer, const Color& tint,
		const int offsetTick, const int offsetLane)
	{
		int texIndex = ResourceManager::getTexture(NOTES_TEX);
		if (texIndex == -1)
			return;

		const Texture& tex = ResourceManager::textures[texIndex];
		const Sprite& s = tex.sprites[getNoteSpriteIndex(note)];
		const Vector2 nodeSz{ laneWidth, laneWidth };

		if (drawHoldStepOutline)
			drawHighlight(note, renderer, tint, true, offsetTick, offsetLane);

		if (type != HoldStepType::Invisible)
		{
			Vector2 pos{ laneToPosition(note.lane + offsetLane + ((note.width - 1) / 2.0f)), getNoteYPosFromTick(note.tick + offsetTick) };
			renderer->drawSprite(pos, 0.0f, nodeSz, AnchorType::MiddleLeft, tex, s.getX(), s.getX() + s.getWidth(),
				s.getY(), s.getHeight(), tint, 1);
		}
	}

	void ScoreEditor::drawFlickArrow(const Note& note, Renderer* renderer, const Color& tint, const int offsetTick, const int offsetLane)
	{
		const int texIndex = ResourceManager::getTexture(NOTES_TEX);
		if (texIndex != -1)
		{
			const Texture& tex = ResourceManager::textures[texIndex];
			const Sprite& arrowS = tex.sprites[getFlickArrowSpriteIndex(note)];

			Vector2 pos{ 0, getNoteYPosFromTick(note.tick + offsetTick)};
			const float x1 = laneToPosition(note.lane + offsetLane);
			const float x2 = pos.x + laneToPosition(note.lane + note.width + offsetLane);
			pos.x = (x1 + x2) * 0.5f;
			pos.y += (notesHeight * 0.55f) + 10.0f;

			// notes wider than 6 lanes also use arrow size 6
			Vector2 midSz(laneWidth * (note.flick == FlickType::Up ? 1.0f : 1.05f) * std::min(note.width, 6), notesHeight - 5);
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

	void ScoreEditor::drawNote(const Note& note, Renderer* renderer, const Color& tint, const int offsetTick, const int offsetLane)
	{
		Vector2 pos(laneToPosition(note.lane + offsetLane), getNoteYPosFromTick(note.tick + offsetTick));
		const Vector2 sliceSz(NOTES_SLICE_WIDTH, notesHeight);
		const AnchorType anchor = AnchorType::MiddleLeft;

		const int texIndex = ResourceManager::getTexture(NOTES_TEX);
		if (texIndex != -1)
		{
			const Texture& tex = ResourceManager::textures[texIndex];
			const Sprite& s = tex.sprites[getNoteSpriteIndex(note)];

			const float midLen = (laneWidth * note.width) - (NOTES_SLICE_WIDTH * 2) + NOTES_X_ADJUST + 5;
			const Vector2 midSz(midLen, notesHeight);

			pos.x -= NOTES_X_ADJUST;

			// left slice
			renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex,
				s.getX() + NOTES_SIDE_WIDTH, s.getX() + NOTES_X_SLICE,
				s.getY(), s.getY() + s.getHeight(), tint, 1
			);
			pos.x += NOTES_SLICE_WIDTH;

			// middle
			renderer->drawSprite(pos, 0.0f, midSz, anchor, tex,
				s.getX() + NOTES_X_SLICE, s.getX() + s.getWidth() - NOTES_X_SLICE,
				s.getY(), s.getY() + s.getHeight(), tint, 1
			);
			pos.x += midLen;

			// right slice
			renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex,
				s.getX() + s.getWidth() - NOTES_X_SLICE, s.getX() + s.getWidth() - NOTES_SIDE_WIDTH,
				s.getY(), s.getY() + s.getHeight(), tint, 1
			);
		}
		
		if (note.isFlick())
			drawFlickArrow(note, renderer, tint, offsetTick, offsetLane);
	}
}
