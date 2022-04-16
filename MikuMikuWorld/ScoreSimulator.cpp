#include "ScoreSimulator.h"
#include "ResourceManager.h"
#include "Rendering/Framebuffer.h"
#include "Constants.h"
#include "UI.h"
#include <algorithm>

namespace MikuMikuWorld
{
	ScoreSimulator::ScoreSimulator()
	{
		framebuffer = new Framebuffer(previewWidth, previewHeight);
		backgorundBrightness = 1.0f;
		laneTransparency = 1.0f;
		noteSpeed = 6.0f;

		camera.yaw = 90;
		camera.pitch = 24;
		camera.position.m128_f32[2] = -2.0f;

		transformMatrix = DirectX::XMMatrixIdentity();
		transformMatrix *= DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(transformAngle));
	}

	float ScoreSimulator::getNoteOnScreenTime()
	{
		float unlerp = (noteSpeed - 12) / (1 - 12);
		float ratio = powf(unlerp, 1.31);
		return lerp(0.25, 10, ratio);
	}

	float ScoreSimulator::laneToPosition(float lane)
	{
		return convertRange(lane, MIN_LANE, MAX_LANE, -1.0f, 1.0f);
	}

	float ScoreSimulator::tickToPosition(int ticks, float time, const std::vector<Tempo>& tempos)
	{
		float noteScreenTime = getNoteOnScreenTime();
		float noteTime = accumulateDuration(ticks, TICKS_PER_BEAT, tempos);

		float percent = ((noteTime - time) / noteScreenTime);
		return lerp(0, 10, percent) - 0.21;
		//return powf(1.06f, (percent + 0.5f) * 45) - 3.9f;

		/*
		float div = abs(noteScreenTime - (noteTime - time));
		if (abs((noteTime - time) - noteScreenTime) < 0.001)
		{
			printf("!\n");
			div = 0.001;
		}
		float percent = (noteTime - time) / div;
		if (noteTime - time > noteScreenTime)
		{
			//percent *= -1.0f;
		}
		*/

		//float percent = (noteTime - time) / ((noteTime - time ) + noteScreenTime);
		//return std::clamp(lerp(0, 10, percent) - 0.21f, -5.0f, 20.0f);
		return lerp(0, 10, percent) -0.21f;
		
	}

	void ScoreSimulator::drawBackground(Renderer* renderer)
	{
		const Texture& bg = ResourceManager::textures[ResourceManager::getTexture("default")];
		renderer->beginBatch();

		Vector2 pos{};
		Vector2 sz{ (float)bg.getWidth(), (float)bg.getHeight() };
		Color tint{ backgorundBrightness, backgorundBrightness, backgorundBrightness, 1.0f };

		renderer->drawSprite(pos, 0.0f, sz, AnchorType::MiddleCenter, bg, 0, tint, 0);
		renderer->endBatch();
	}

	void ScoreSimulator::drawJacket(Renderer* renderer)
	{

	}

	void ScoreSimulator::drawLanes(Renderer* renderer)
	{
		const Texture& laneBase = ResourceManager::textures[ResourceManager::getTexture("lane_base")];
		const Texture& laneLine = ResourceManager::textures[ResourceManager::getTexture("lane_line")];
		const Texture& judgeLine = ResourceManager::textures[ResourceManager::getTexture("judgeLine")];

		Vector2 basePos{ 0, 0 };
		Vector2 baseSz{ (float)laneBase.getWidth(), (float)laneBase.getHeight() };
		Vector2 lineSz{ (float)judgeLine.getWidth(), (float)judgeLine.getHeight() };
		Color baseTint{ 1.0f, 1.0f, 1.0f, laneTransparency };
		Color tint{ 1.0f, 1.0f, 1.0f, 1.0f };

		renderer->beginBatch();
		renderer->drawSprite(basePos, 0.0f, baseSz, AnchorType::MiddleCenter, laneBase, 0, baseTint, 0);
		renderer->drawSprite(basePos, 0.0f, baseSz, AnchorType::MiddleCenter, laneLine, 0, tint, 0);
		renderer->drawSprite(judgeLinePos, 0.0f, lineSz, AnchorType::MiddleCenter, judgeLine, 0, tint, 0);
		renderer->endBatch();
	}

	void ScoreSimulator::drawNotes(float time, const Score& score, Renderer* renderer)
	{
		Shader* shader = ResourceManager::shaders[1];
		shader->use();

		camera.updateCameraVectors();
		shader->setMatrix4("view", camera.getViewMatrix());
		shader->setMatrix4("projection", camera.getPerspectiveProjection(debugLaneFov, previewWidth / previewHeight, 0.01f, 200.0f));

		renderer->beginBatch();
		float onScreenTime = getNoteOnScreenTime();
		for (const auto& nIt : score.notes)
		{
			const Note& note = nIt.second;
			if (note.getType() != NoteType::Tap)
				continue;

			float noteTime = accumulateDuration(note.tick, TICKS_PER_BEAT, score.tempoChanges);
			if (noteTime - time > onScreenTime || noteTime < time)
				continue;

			drawNote(note, time, score.tempoChanges, renderer);
		}

		for (const auto& hIt : score.holdNotes)
		{
			drawHoldNote(hIt.second, score, time, renderer);
		}
		renderer->endBatch();
	}

	void ScoreSimulator::drawNote(const Note& note, float time, const std::vector<Tempo>& tempos, Renderer* renderer)
	{
		Texture& noteTex = ResourceManager::textures[0];
		Color tint{ 1.0f, 1.0f, 1.0f, 1.0f };

		float x = convertRange(note.lane, MIN_LANE, MAX_LANE, -1.0f, 1.0f);
		float y = tickToPosition(note.tick, time, tempos);

		Vector3 notePos{ x, y, 0.0f };
		transformMatrix = DirectX::XMMatrixIdentity();
		transformMatrix *= DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(transformAngle));
		notePos = notePos.transform(transformMatrix);

		Vector3 noteRot{ noteRotX };
		Vector3 noteScale{ noteScaleX, noteSclaeY, 1.0f };
		Vector3 noteMidScale{ (0.1725f * (note.width)) - 0.05f, 0.28f };

		const Sprite& spr = noteTex.sprites[getNoteSpriteIndex(note)];

		// left
		notePos.x -= 0.0725f + (note.lane * 0.0065f);
		float leftX = notePos.x;
		renderer->drawQuad(notePos, noteRot, noteScale, AnchorType::MiddleCenter, noteTex,
			spr.getX(), spr.getX() + NOTES_X_SLICE, spr.getY(), spr.getY() + spr.getHeight(), tint, 1);

		// mid
		notePos.x += 0.05f;
		renderer->drawQuad(notePos, noteRot, noteMidScale, AnchorType::MiddleLeft, noteTex,
			spr.getX() + NOTES_X_SLICE, spr.getX() + spr.getWidth() - NOTES_X_SLICE, spr.getY(), spr.getY() + spr.getHeight(), tint, 1);

		// right
		notePos.x += (note.width * 0.1725f);
		float rightX = notePos.x;
		renderer->drawQuad(notePos, noteRot, noteScale, AnchorType::MiddleCenter, noteTex,
			spr.getX() + spr.getWidth() - NOTES_X_SLICE, spr.getX() + spr.getWidth(), spr.getY(), spr.getY() + spr.getHeight(), tint, 1);

		if (note.isFlick())
		{
			Vector3 arrowPos{ (leftX + rightX) * 0.5f, y, flickArrowYOffset + (std::min(6, note.width) * flickArrowYOffsetFactor)};
			arrowPos = arrowPos.transform(transformMatrix);
			arrowPos.x -= x * 0.02f;
			
			Vector3 arrowSz{ flickArrowBaseWidth + (std::min(note.width, 6) * flickArrowWidthFactor), flickArrowBaseHeight + (std::min(note.width, 6) * flickArrowHeightFactor) };

			const Sprite& aSpr = noteTex.sprites[getFlickArrowSpriteIndex(note)];

			float x1 = aSpr.getX();
			float x2 = aSpr.getX() + aSpr.getWidth();

			if (note.flick == FlickType::Right)
			{
				x1 = aSpr.getX() + aSpr.getWidth();
				x2 = aSpr.getX();
			}

			float y1 = aSpr.getY();
			float y2 = aSpr.getY() + aSpr.getHeight();

			renderer->drawQuad(arrowPos, Vector3{ -10 }, arrowSz, AnchorType::MiddleCenter, noteTex, x1, x2, y1, y2, tint, 2);
		}
	}

	void ScoreSimulator::drawHoldCurve(const Note& n1, const Note& n2, EaseType ease, float time, const std::vector<Tempo>& tempos, Renderer* renderer)
	{
		float startX1 = laneToPosition(n1.lane);
		float startX2 = laneToPosition(n1.lane + n1.width);
		float startY = tickToPosition(n1.tick, time, tempos);

		float endX1 = laneToPosition(n2.lane);
		float endX2 = laneToPosition(n2.lane + n2.width);
		float endY = tickToPosition(n2.tick, time, tempos);

		//if (startY < -5 && endY > 20)
			//return;

		startX1 += holdXOffset + (holdXOffsetFactor * startX1);
		startX2 += holdXOffset + (holdXOffsetFactor * startX2);
		endX1 += holdXOffset + (holdXOffsetFactor * endX1);
		endX2 += holdXOffset + (holdXOffsetFactor * endX2);

		int texIndex = ResourceManager::getTexture(n1.critical ? HOLD_PATH_CRTCL_TEX : HOLD_PATH_TEX);
		if (texIndex == -1)
			return;

		Texture& pathTex = ResourceManager::textures[texIndex];
		Color tint{ 1.0f, 1.0f, 1.0f, 1.0f };

		float steps = std::min(std::ceilf(abs(endY - startY) * 10), 50.0f);
		for (int y = 0; y < steps; ++y)
		{
			const float percent1 = y / steps;
			const float percent2 = (y + 1) / steps;
			float i1 = ease == EaseType::None ? percent1 : ease == EaseType::EaseIn ? easeIn(percent1) : easeOut(percent1);
			float i2 = ease == EaseType::None ? percent2 : ease == EaseType::EaseIn ? easeIn(percent2) : easeOut(percent2);

			float xl1 = lerp(startX1, endX1, i1) - holdSideScale;
			float xr1 = lerp(startX2, endX2, i1) + holdSideScale;
			float y1 = lerp(startY, endY, percent1);
			float y2 = lerp(startY, endY, percent2);
			float xl2 = lerp(startX1, endX1, i2) - holdSideScale;
			float xr2 = lerp(startX2, endX2, i2) + holdSideScale;

			Vector3 p1{ xl1, y1 };
			Vector3 p2{ xl1 + holdSideScale, y1 };
			Vector3 p3{ xl2, y2 };
			Vector3 p4{ xl2 + holdSideScale, y2 };
			renderer->drawRectangle(p1, p2, p3, p4, pathTex, 0, HOLD_X_SLICE, 0, pathTex.getHeight(), tint);
			
			p1.x = xl1 + holdSideScale;
			p2.x = xr1 - holdSideScale;
			p3.x = xl2 + holdSideScale;
			p4.x = xr2 - holdSideScale;
			renderer->drawRectangle(p1, p2, p3, p4, pathTex, HOLD_X_SLICE, pathTex.getWidth() - HOLD_X_SLICE, 0, pathTex.getHeight(), tint);

			p1.x = xr1 - holdSideScale;
			p2.x = xr1;
			p3.x = xr2 - holdSideScale;
			p4.x = xr2;
			renderer->drawRectangle(p1, p2, p3, p4, pathTex, pathTex.getWidth() - HOLD_X_SLICE, pathTex.getWidth(), 0, pathTex.getHeight(), tint);
			
		}
	}

	void ScoreSimulator::drawHoldNote(const HoldNote& hold, const Score& score, float time, Renderer* renderer)
	{
		const Note& start = score.notes.at(hold.start.ID);
		const Note& end = score.notes.at(hold.end);
		Color tint{ 1.0f, 1.0f, 1.0f, 1.0f };

		float onScreenTime = getNoteOnScreenTime();
		float startTime = accumulateDuration(start.tick, TICKS_PER_BEAT, score.tempoChanges);
		if (startTime - time > onScreenTime)
			return;

		if (hold.steps.size())
		{
			int s1 = -1;
			int s2 = -1;

			for (int i = 0; i < hold.steps.size(); ++i)
			{
				HoldStepType type = hold.steps[i].type;
				if (type == HoldStepType::Ignored)
					continue;

				s2 = i;
				const Note& n1 = s1 == -1 ? start : score.notes.at(hold.steps[s1].ID);
				const Note& n2 = s2 == -1 ? end : score.notes.at(hold.steps[s2].ID);
				const EaseType ease = s1 == -1 ? hold.start.ease : hold.steps[s1].ease;
				drawHoldCurve(n1, n2, ease, time, score.tempoChanges, renderer);

				s1 = s2;
			}

			const Note& n1 = s1 == -1 ? start : score.notes.at(hold.steps[s1].ID);
			const EaseType ease = s1 == -1 ? hold.start.ease : hold.steps[s1].ease;
			drawHoldCurve(n1, end, ease, time, score.tempoChanges, renderer);

			s1 = -1;
			s2 = 1;

			const Texture& tex = ResourceManager::textures[0];
			const Vector3 nodeSz{ noteScaleX * 2, noteScaleX * 2, 1.0f };
			for (int i = 0; i < hold.steps.size(); ++i)
			{
				const Note& n3 = score.notes.at(hold.steps[i].ID);
				// find first non-ignored step
				if (s2 < i)
					s2 = i;

				while (s2 < hold.steps.size())
				{
					if (hold.steps[s2].type != HoldStepType::Ignored)
						break;
					++s2;
				}

				if (hold.steps[i].type != HoldStepType::Invisible)
				{
					float midTime = accumulateDuration(n3.tick, TICKS_PER_BEAT, score.tempoChanges);
					if (midTime - time < onScreenTime)
					{
						float y = tickToPosition(n3.tick, time, score.tempoChanges);

						const Sprite& s = tex.sprites[getNoteSpriteIndex(n3)];
						float lane = n3.lane + ((float)(n3.width - 1.0f) / 2.0f);
						Vector3 pos{ laneToPosition(lane), y };

						if (hold.steps[i].type == HoldStepType::Ignored)
						{
							const Note& n1 = s1 == -1 ? start : score.notes.at(hold.steps[s1].ID);
							const Note& n2 = s2 >= hold.steps.size() ? end : score.notes.at(hold.steps[s2].ID);

							float ratio = (float)(n3.tick - n1.tick) / (float)(n2.tick - n1.tick);
							const EaseType rEase = s1 == -1 ? hold.start.ease : hold.steps[s1].ease;
							float i1 = rEase == EaseType::None ? ratio : rEase == EaseType::EaseIn ? easeIn(ratio) : easeOut(ratio);

							float x1 = lerp(laneToPosition(n1.lane), laneToPosition(n2.lane), i1);
							float x2 = lerp(laneToPosition(n1.lane + n1.width), laneToPosition(n2.lane + n2.width), i1);
							pos.x = (x1 + x2) / 2.0f;
						}

						pos = pos.transform(transformMatrix);
						renderer->drawQuad(pos, 0.0f, nodeSz, AnchorType::MiddleCenter, tex, s.getX(), s.getX() + s.getWidth(),
							s.getY(), s.getHeight(), tint, 1);
					}
				}
				if (hold.steps[i].type != HoldStepType::Ignored)
					s1 = i;
			}
		}
		else
		{
			drawHoldCurve(start, end, hold.start.ease, time, score.tempoChanges, renderer);
		}


		float endTime = accumulateDuration(end.tick, TICKS_PER_BEAT, score.tempoChanges);

		if (startTime - time <= onScreenTime)
		{
			drawNote(start, time, score.tempoChanges, renderer);
		}

		if (endTime - time <= onScreenTime)
			drawNote(end, time, score.tempoChanges, renderer);
	}

	ImVec2 ScoreSimulator::getPreviewDimensions(const ImVec2& windowSize)
	{
		float width = previewWidth;
		float height = previewHeight;
		float hRatio = height / width;
		float wRatio = width / height;
		if (width > windowSize.x - 10)
			width = windowSize.x - 10;

		height = hRatio * width;
		if (height > windowSize.y - 15)
			height = windowSize.y - 15;

		width = wRatio * height;

		return ImVec2(width, height);
	}

	void ScoreSimulator::updateSettings()
	{
		if (ImGui::Begin("Preview Settings"))
		{
			ImGui::InputFloat("Background Brightness", &backgorundBrightness, 0, 0, "%.2f");
			ImGui::InputFloat("Lane Transparency", &laneTransparency, 0, 0, "%.2f");
		}

		ImGui::End();
	}

	void ScoreSimulator::update(float time, const Score& score, Renderer* renderer)
	{
		if (ImGui::Begin(previewWindow))
		{
			Shader* shader = ResourceManager::shaders[0];

			shader->use();
			shader->setMatrix4("projection", camera.getOrthographicProjection(previewWidth, previewHeight));

			framebuffer->bind();
			framebuffer->clear();

			drawBackground(renderer);
			drawJacket(renderer);
			drawLanes(renderer);
			drawNotes(time, score, renderer);

			glDisable(GL_DEPTH_TEST);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// flip image to become upright
			unsigned int fbTex = framebuffer->getTexture();
			ImVec2 previewSize = getPreviewDimensions(ImGui::GetWindowSize());
			ImVec2 previewPos = ImGui::GetWindowPos() + ((ImGui::GetWindowSize() - previewSize) * 0.5f);
			
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddImage((void*)fbTex, previewPos, previewPos + previewSize, ImVec2(0, 1), ImVec2(1, 0));
		}

		ImGui::End();

		updateSettings();
		debug();
	}

	void ScoreSimulator::debug()
	{
		if (ImGui::Begin("Simulator Debug"))
		{
			float camPos[]{camera.position.m128_f32[0], camera.position.m128_f32[1], camera.position.m128_f32[2]};
			if (ImGui::DragFloat3("position", camPos, 0.01))
			{
				camera.position.m128_f32[0] = camPos[0];
				camera.position.m128_f32[1] = camPos[1];
				camera.position.m128_f32[2] = camPos[2];
			}

;			ImGui::DragFloat("yaw", &camera.yaw, 0.1);
			ImGui::DragFloat("pitch", &camera.pitch, 0.1);

			ImGui::Separator();

			// lane transform debug
			ImGui::DragFloat("transform angle", &transformAngle, 0.01f);
			ImGui::DragFloat("lane fov", &debugLaneFov, 0.01f);

			// note debug
			ImGui::DragFloat("time adjustment", &timeAdjustment, 0.01f);
			ImGui::DragFloat("debug note pos X", &debugNoteXPos, 0.01f);
			ImGui::SliderFloat("note speed", &noteSpeed, 1.0f, 12.0f, "%.1f");
			ImGui::Text("note on-screen time %f", getNoteOnScreenTime());

			// flick debug
			ImGui::DragFloat("flick arrow base width", &flickArrowBaseWidth, 0.01);
			ImGui::DragFloat("flick arrow base height", &flickArrowBaseHeight, 0.01);
			ImGui::DragFloat("flick arrow width factor", &flickArrowWidthFactor, 0.01);
			ImGui::DragFloat("flick arrow height factor", &flickArrowHeightFactor, 0.01);
			ImGui::DragFloat("flick arrow y offset", &flickArrowYOffset, 0.01);
			ImGui::DragFloat("flick arrow y offset factor", &flickArrowYOffsetFactor, 0.01);

			// hold debug
			ImGui::DragFloat("hold x offset", &holdXOffset, 0.01);
			ImGui::DragFloat("hold x offset factor", &holdXOffsetFactor, 0.01);
			ImGui::DragFloat("hold side scale", &holdSideScale, 0.01);
		}

		ImGui::End();
	}
}