#include "ScorePreview.h"
#include "ResourceManager.h"
#include "Colors.h"
#include "ImageCrop.h"
#include "GLFW/glfw3.h"
#include "Rendering/Quad.h"

namespace MikuMikuWorld
{
	namespace PrvConfig {
		static bool lockAspectRatio = true;
		static float noteSpeed = 1;
		float ref_alpha = 0.4f;

		static void drawMenu() {
			if(!ImGui::Begin("Config###config_window")) {
				ImGui::End();
				return;
			}
			ImGui::Checkbox("Lock Aspect Ratio", &lockAspectRatio);
			ImGui::SliderFloat("Note Speed", &noteSpeed, 1, 12, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			
			ImGui::SliderFloat("Reference", &ref_alpha, 0, 1.f);
			ImGui::End();
		}
		
	};

	namespace Utils {
		inline float accumulateScaledDuration(int tick, int ticksPerBeat, const std::vector<Tempo>& bpms, const std::vector<HiSpeedChange>& hispeeds) {
			int prvBpm = 0, prvSpd = -1;
			int accTicks = 0;
			float totalDuration = 0;

			while (accTicks < tick) {
				int nxtBpmTick = prvBpm + 1 < bpms.size() ? bpms[prvBpm + 1].tick : INT_MAX;
				int nxtSpdTick = prvSpd + 1 < hispeeds.size() ? hispeeds[prvSpd + 1].tick : INT_MAX;
				int nxtTick = std::min({nxtBpmTick, nxtSpdTick, tick});

				float currentBpm = bpms.at(prvBpm).bpm;
				float currentSpd = prvSpd >= 0 ? hispeeds[prvSpd].speed : 1.0f;

				totalDuration += ticksToSec(nxtTick - accTicks, ticksPerBeat, currentBpm) * currentSpd;

				if (nxtTick == nxtBpmTick)
					prvBpm++;
				if (nxtTick == nxtSpdTick)
					prvSpd++;
				accTicks = nxtTick;
			}
			return totalDuration;
		}

		// Scale a rectangle with specified aspect ratio to be visible inside the target rectangle
		inline void fitRect(float target_width, float target_height, long double source_aspect_ratio, float& width, float& height) {
			const float target_aspect_ratio = target_width / target_height;
			width  = target_aspect_ratio > source_aspect_ratio ? source_aspect_ratio * target_height : target_width;
			height = target_aspect_ratio < source_aspect_ratio ? target_width / source_aspect_ratio : target_height;
			return;
		}

		// Scale a rectangle with specified aspect ratio so it fill the area of the target rectangle
		inline void fillRect(float target_width, float target_height, long double source_aspect_ratio, float& width, float& height) {
			const float target_aspect_ratio = target_width / target_height;
			width  = target_aspect_ratio < source_aspect_ratio ? source_aspect_ratio * target_height : target_width;
			height = target_aspect_ratio > source_aspect_ratio ? target_width / source_aspect_ratio : target_height;
			return;
		}

		inline void drawUnscaledTexture(unsigned int texId, float texWidth, float texHeight, ImVec2 const& pos, ImVec2 const& size, ImU32 const& col) {
			const float source_ratio = texWidth / texHeight;
			const float target_ratio = size.x / size.y;
			ImVec2 target_size = {
				target_ratio >  source_ratio ? source_ratio * size.y : size.x,
				target_ratio >= source_ratio ? size.y : size.x / source_ratio
			};
			ImVec2 target_pos = pos + (size - target_size) / 2;
			ImGui::GetWindowDrawList()->AddImage((ImTextureID)(size_t)texId, target_pos, target_pos + target_size, {0, 0}, {1, 1}, col);
		}

	};

	namespace Engine {
		enum class SpriteType {
			NoteLeft,
			NoteRight,
			NoteMiddle
		};
		struct Range {
			float min;
			float max;
		};
		// Contains hardcode value to the sprite texture
		namespace Transform {
			static const float NOTE_SIDE_WIDTH = 93;
			static constexpr float getStageStart(float height) { return height * (0.5 + 1.15875 * (47.f / 1176)); }
			static constexpr float getStageEnd(float height) { return height * (0.5 - 1.15875 * (803. / 1176)); }
			static constexpr float getLaneWidth(float width) { return width * ((1.15875 * (1420. / 1176)) / (16. / 9) / 12.); }

			// Map the vertices of a quad to the sprite
			static std::array<DirectX::XMVECTOR, 4> spriteTransform(std::array<DirectX::XMVECTOR, 4> const& vPos, SpriteType type) {
				const float 
					&x1 = vPos[2].vector4_f32[0], &y1 = vPos[2].vector4_f32[1],
				  	&x2 = vPos[3].vector4_f32[0], &y2 = vPos[3].vector4_f32[1],
					&x3 = vPos[0].vector4_f32[0], &y3 = vPos[0].vector4_f32[1],
					&x4 = vPos[1].vector4_f32[0], &y4 = vPos[1].vector4_f32[1];
				std::array<DirectX::XMVECTOR, 4> sPos;
				const auto s_side = [](float v1, float v2, float v3, float v4) -> float { return 4.294791666666667 * v1 - 1.7114583333333337 * v2 + 1.0489583333333337 * v3 - 2.632291666666667 * v4; };
				const auto s_mid = [](float v1, float v2, float v3, float v4) -> float { return 1.6625 * v3 -0.6625000000000001 * v4; };

				switch (type) {
					case SpriteType::NoteLeft:
						sPos[0] = {  s_mid(x1, x2, x3, x4),  s_mid(y1, y2, y3, y4), vPos[0].vector4_f32[2], vPos[0].vector4_f32[3] };
						sPos[1] = {  s_mid(x2, x1, x4, x3),  s_mid(y2, y1, y4, y3), vPos[1].vector4_f32[2], vPos[1].vector4_f32[3] };
						sPos[2] = { s_side(x1, x2, x3, x4), s_side(y1, y2, y3, y4), vPos[2].vector4_f32[2], vPos[2].vector4_f32[3] };
						sPos[3] = { s_side(x2, x1, x4, x3), s_side(y2, y1, y4, y3), vPos[3].vector4_f32[2], vPos[3].vector4_f32[3] };
						break;
					case SpriteType::NoteRight:
						sPos[0] = { s_side(x3, x4, x1, x2), s_side(y3, y4, y1, y2), vPos[0].vector4_f32[2], vPos[0].vector4_f32[3] };
						sPos[1] = { s_side(x4, x3, x2, x1), s_side(y4, y3, y2, y1), vPos[1].vector4_f32[2], vPos[1].vector4_f32[3] };
						sPos[2] = {  s_mid(x3, x4, x1, x2),  s_mid(y3, y4, y1, y2), vPos[2].vector4_f32[2], vPos[2].vector4_f32[3] };
						sPos[3] = {  s_mid(x4, x3, x2, x1),  s_mid(y4, y3, y2, y1), vPos[3].vector4_f32[2], vPos[3].vector4_f32[3] };
						break;
					case SpriteType::NoteMiddle:
						sPos[0] = {  s_mid(x1, x2, x3, x4),  s_mid(y1, y2, y3, y4), vPos[0].vector4_f32[2], vPos[0].vector4_f32[3] };
						sPos[1] = {  s_mid(x2, x1, x4, x3),  s_mid(y2, y1, y4, y3), vPos[1].vector4_f32[2], vPos[1].vector4_f32[3] };
						sPos[2] = {  s_mid(x3, x4, x1, x2),  s_mid(y3, y4, y1, y2), vPos[2].vector4_f32[2], vPos[2].vector4_f32[3] };
						sPos[3] = {  s_mid(x4, x3, x2, x1),  s_mid(y4, y3, y2, y1), vPos[3].vector4_f32[2], vPos[3].vector4_f32[3] };
						break;
				}
				return sPos;
			}
		}
		static Range getNoteVisualTime(Note const& note, Score const& score) {
			float targetTime = Utils::accumulateScaledDuration(note.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
			float duration = lerp(0.35, 4.f, std::pow(unlerp(12, 1, PrvConfig::noteSpeed), 1.31f));
			return {targetTime - duration, targetTime};
		}
		static inline float approach(float start_time, float end_time, float current_time) {
			return std::pow(1.06, 45 * lerp(-1, 0, unlerp(start_time, end_time, current_time)));
		}
		static inline std::array<DirectX::XMVECTOR, 4> perspectiveQuad(float left, float right, float top, float bottom) {
			float x1 = right * top,    y1 = top,
				  x2 = right * bottom, y2 = bottom,
				  x3 = left * bottom,  y3 = bottom,
				  x4 = left * top,     y4 = top;
			// Quad order, right->left, bottom->top
			return {{
				{ x1, y1, 1.0f, 1.0f },
				{ x2, y2, 1.0f, 1.0f },
				{ x3, y3, 1.0f, 1.0f },
				{ x4, y4, 1.0f, 1.0f },
			}};
		}
		static inline float laneToLeft(float lane) { return lane - 6; }
	};

	ScorePreviewWindow::ScorePreviewWindow() : previewBuffer{1920, 1080}
	{
		
	}

	void ScorePreviewWindow::update(ScoreContext& context, Renderer* renderer) {
		
		PrvConfig::drawMenu();
		bool isWindowActive =  !ImGui::IsWindowDocked() || ImGui::GetCurrentWindow()->TabId == ImGui::GetWindowDockNode()->SelectedTabId;
		if (!isWindowActive)
			// Don't draw anything if the window is not active.
			// SFX and music updates are handled in the timeline
			return;
		ImVec2 size = ImGui::GetContentRegionAvail();
		ImVec2 position = ImGui::GetCursorScreenPos();
		ImRect boundaries = ImRect(position, position + size);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(boundaries.Min, boundaries.Max, 0xff202020);
		
		int shaderId;
		if ((shaderId = ResourceManager::getShader("basic2d")) == -1) return;
		Shader* shader = ResourceManager::shaders[shaderId];
		shader->use();

		// Create a projection to the engine coordinate system
		// 				 0 note spawn
		// left lane -6     6 right lane
		// 				 1 judgement line
		float width  = size.x, height = size.y;
		float right = Engine::Transform::getLaneWidth(PrvConfig::lockAspectRatio ? 1920 : size.x);
		float top  = Engine::Transform::getStageStart(PrvConfig::lockAspectRatio ? 1080 : size.y);
		float bottom = Engine::Transform::getStageEnd(PrvConfig::lockAspectRatio ? 1080 : size.y);
		if (PrvConfig::lockAspectRatio) Utils::fillRect(1920, 1080, size.x / size.y, width, height);

		auto view = DirectX::XMMatrixScaling(right, bottom - top, 1.f) * DirectX::XMMatrixTranslation(0.f, top, 0.f);
		auto projection = Camera::getOffCenterOrthographicProjection(-width / 2, width / 2, -height / 2, height / 2);
		shader->setMatrix4("projection", view * projection);

		if (previewBuffer.getWidth() != size.x || previewBuffer.getHeight() != size.y)
			previewBuffer.resize(size.x, size.y);
		previewBuffer.bind();
		previewBuffer.clear();

		drawStage(renderer);
		drawNotes(context, renderer);

		previewBuffer.unblind();
		drawList->AddImage((ImTextureID)(size_t)previewBuffer.getTexture(), position, position + size);
	}

    void ScorePreviewWindow::drawNotes(ScoreContext& context, Renderer *renderer)
    {
		const float TOP_CUTOFF = 0.f;
		const float BOTTOM_CUTOFF = 1.f;
		std::multimap<float, int> drawingNotes;
		float scaled_tm = Utils::accumulateScaledDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);

		ImGui::Begin("Config###config_window");
		for (auto& [id, note] : context.score.notes) {
			if (note.getType() == NoteType::Tap) {
				auto visual_tm = Engine::getNoteVisualTime(note, context.score);
				float y = INFINITY;
				if (visual_tm.min <= scaled_tm && scaled_tm <= visual_tm.max) {
					y = Engine::approach(visual_tm.min, visual_tm.max, scaled_tm);
					drawingNotes.insert({y, id});
				}
				ImGui::Text("Visual (%.4f, %.4f)\tY: %.4f", visual_tm.min, visual_tm.max, y);
			}
		}
		renderer->beginBatch();
		ImGui::SliderInt("Current Tick", &context.currentTick, 0, TICKS_PER_BEAT * 20);
		for (auto& [y, id] : drawingNotes) {
			drawNote(context.score.notes[id], y, noteTint, renderer);
		}
		ImGui::End();
		renderer->endBatch();
    }

    void ScorePreviewWindow::drawNote(const Note& note, float y, const Color& tint, Renderer* renderer)
    {
		if (noteTextures.notes == -1)
			return;

		const Texture& texture = ResourceManager::textures[noteTextures.notes];
		const int sprIndex = getNoteSpriteIndex(note);
		if (!isArrayIndexInBounds(sprIndex, texture.sprites))
			return;

		const Sprite& s = texture.sprites[sprIndex];

		const float noteHeight = 75. / 850. / 2.;
		float noteLeft = Engine::laneToLeft(note.lane), noteRight = Engine::laneToLeft(note.lane) + note.width;

		// Uniform value between notes slices
		auto color = DirectX::XMVECTOR{ tint.r, tint.g, tint.b, tint.a };
		std::array<DirectX::XMVECTOR, 4> vColor = { color, color, color, color };
		auto model = DirectX::XMMatrixScaling(y, y, y);
		// Left slice
		auto vPos = Engine::Transform::spriteTransform(Engine::perspectiveQuad(noteLeft, noteLeft + 0.3f, 1 - noteHeight, 1 + noteHeight), Engine::SpriteType::NoteLeft);
		renderer->drawQuad(vPos, model, texture, s.getX1(), s.getX1() + Engine::Transform::NOTE_SIDE_WIDTH, s.getY1(), s.getY2(), tint, (int)ZIndex::Note);
		
		// Right slice
		vPos = Engine::Transform::spriteTransform(Engine::perspectiveQuad(noteRight - 0.3f, noteRight, 1 - noteHeight, 1 + noteHeight), Engine::SpriteType::NoteRight);
		renderer->drawQuad(vPos, model, texture, s.getX2() - Engine::Transform::NOTE_SIDE_WIDTH, s.getX2(), s.getY1(), s.getY2(), tint, (int)ZIndex::Note);

		// Middle
		vPos = Engine::Transform::spriteTransform(Engine::perspectiveQuad(noteLeft + 0.3f, noteRight - 0.3f, 1 - noteHeight, 1 + noteHeight), Engine::SpriteType::NoteMiddle);
		renderer->drawQuad(vPos, model, texture, s.getX1() + Engine::Transform::NOTE_SIDE_WIDTH, s.getX2() - Engine::Transform::NOTE_SIDE_WIDTH, s.getY1(), s.getY2(), tint, (int)ZIndex::Note);
    }

    void ScorePreviewWindow::drawStage(Renderer* renderer)
    {
		int index = ResourceManager::getTexture("stage");
		if (index == -1)
			return;
		Texture& stage = ResourceManager::textures[index];

		const float left = ((2048. / 1420.) * 12) / 2;
		const float top = 47 / 850.;
		const float width = left * 2;
		const float height = 1176 / 850.;
		
		renderer->beginBatch();
		renderer->drawRectangle({-left, top}, {width, height}, stage, 0, 2048, 0, 1176, Color{1, 1, 1}, 0);
		renderer->endBatch();
    }
}