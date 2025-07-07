#include "ScorePreview.h"
#include "PreviewEngine.h"
#include "Rendering/Camera.h"
#include "ResourceManager.h"
#include "Colors.h"

namespace MikuMikuWorld
{
	namespace PrvConfig {
		static bool lockAspectRatio = true;
		static float noteSpeed = 1;
		static bool markerAnimation = true;
		float ref_alpha = 0.4f;

		static void drawMenu() {
			if(!ImGui::Begin("Config###config_window")) {
				ImGui::End();
				return;
			}
			ImGui::Checkbox("Lock Aspect Ratio", &lockAspectRatio);
			ImGui::Checkbox("Flick Animation", &markerAnimation);
			ImGui::SliderFloat("Note Speed", &noteSpeed, 1, 12, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			
			ImGui::SliderFloat("Reference", &ref_alpha, 0, 1.f);
			ImGui::End();
		}
		
	};

	namespace Utils {

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
	};

	ScorePreviewWindow::ScorePreviewWindow() : previewBuffer{1920, 1080}, notesTex()
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
		float right = Engine::getLaneWidth(PrvConfig::lockAspectRatio ? 1920 : size.x);
		float top  = Engine::getStageStart(PrvConfig::lockAspectRatio ? 1080 : size.y);
		float bottom = Engine::getStageEnd(PrvConfig::lockAspectRatio ? 1080 : size.y);
		if (PrvConfig::lockAspectRatio) Utils::fillRect(1920, 1080, size.x / size.y, width, height);
		scaledAspectRatio = right / (top - bottom);

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

    ScorePreviewWindow::~ScorePreviewWindow()
    {
		if(notesTex) notesTex->dispose();
    }

    const Texture &ScorePreviewWindow::getNoteTexture()
    {
		// At smaller widths, due to mipmapping, the texture becomes blurry.
		// So we make a copy texture which have no mipmapping
		if (!notesTex) {
			const Texture& target_texture = ResourceManager::textures[noteTextures.notes];
			notesTex = std::make_unique<Texture>(target_texture.getFilename(), TextureFilterMode::Linear);
		}
		return *notesTex;
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

    void ScorePreviewWindow::drawNotes(ScoreContext& context, Renderer *renderer)
    {
		const float TOP_CUTOFF = 0.f;
		const float BOTTOM_CUTOFF = 1.f;
		std::multimap<float, int> drawingNotes;
		float current_tm = accumulateDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges);
		float scaled_tm = accumulateScaledDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);

		ImGui::Begin("Config###config_window");
		for (auto rit = context.score.notes.rbegin(), rend = context.score.notes.rend(); rit != rend; rit++) {
			auto& [id, note] = *rit;
			if (note.getType() == NoteType::Tap) {
				auto visual_tm = Engine::getNoteVisualTime(note, context.score, PrvConfig::noteSpeed);
				float y = INFINITY;
				if (visual_tm.min <= scaled_tm && scaled_tm <= visual_tm.max) {
					y = Engine::approach(visual_tm.min, visual_tm.max, scaled_tm);
					drawingNotes.insert({y, id});
				}
				//ImGui::Text("Visual (%.4f, %.4f)\tY: %.4f", visual_tm.min, visual_tm.max, y);
			}
		}
		renderer->beginBatch();
		ImGui::SliderInt("Current Tick", &context.currentTick, 0, TICKS_PER_BEAT * 20);
		
		for (auto& [y, id] : drawingNotes) {
			const Note& note = context.score.notes[id];
			drawNoteBase(note, y, noteTint, renderer);
			if (note.isFlick()) 
				drawFlickArrow(note, y, current_tm, context.score, noteTint, renderer);
		}
		ImGui::End();
		renderer->endBatch();
    }

    void ScorePreviewWindow::drawNoteBase(const Note& note, float y, const Color& tint, Renderer* renderer)
    {
		const int NOTE_SIDE_WIDTH = 93;
		if (noteTextures.notes == -1)
			return;

		const Texture& texture = getNoteTexture();
		const int sprIndex = getNoteSpriteIndex(note);
		if (!isArrayIndexInBounds(sprIndex, texture.sprites))
			return;

		const Sprite& s = texture.sprites[sprIndex];

		const float noteHeight = 75. / 850. / 2.;
		float noteLeft = Engine::laneToLeft(note.lane), noteRight = Engine::laneToLeft(note.lane) + note.width;
		const float noteTop = 1. - noteHeight, noteBottom = 1. + noteHeight;

		auto model = DirectX::XMMatrixScaling(y, y, y);
		std::array<DirectX::XMFLOAT4, 4> vPos;
		// Middle
		vPos = ResourceManager::spriteTransform.at((int)Engine::SpriteType::NoteMiddle).apply(perspectiveQuadvPos(noteLeft + 0.3f, noteRight - 0.3f, noteTop, noteBottom));
		renderer->drawQuad(vPos, model, texture, s.getX1() + NOTE_SIDE_WIDTH, s.getX2() - NOTE_SIDE_WIDTH, s.getY1(), s.getY2(), tint, (int)ZIndex::Note);

		// Left slice
		vPos = ResourceManager::spriteTransform.at((int)Engine::SpriteType::NoteLeft).apply(perspectiveQuadvPos(noteLeft, noteLeft + 0.3f, noteTop, noteBottom));
		renderer->drawQuad(vPos, model, texture, s.getX1(), s.getX1() + NOTE_SIDE_WIDTH, s.getY1(), s.getY2(), tint, (int)ZIndex::Note);
		
		// Right slice
		vPos = ResourceManager::spriteTransform.at((int)Engine::SpriteType::NoteRight).apply(perspectiveQuadvPos(noteRight - 0.3f, noteRight, noteTop, noteBottom));
		renderer->drawQuad(vPos, model, texture, s.getX2() - NOTE_SIDE_WIDTH, s.getX2(), s.getY1(), s.getY2(), tint, (int)ZIndex::Note);
    }

    void ScorePreviewWindow::drawFlickArrow(const Note &note, float y, float time, const Score &score, const Color &tint, Renderer *renderer)
    {
		if (noteTextures.notes == -1)
			return;

		const int MAX_FLICK_SPRITES = 6;
		const Texture& texture = getNoteTexture();
		const int sprIndex = getFlickArrowSpriteIndex(note);
		if (!isArrayIndexInBounds(sprIndex, texture.sprites))
			return;
		const Sprite& arrowSprite = texture.sprites[sprIndex];

		const int flickDirection = note.flick == FlickType::Left ? -1 : (note.flick == FlickType::Right ? 1 : 0);

		const float w = clamp(note.width, 0, MAX_FLICK_SPRITES) * (note.flick == FlickType::Right ? -1 : 1) / 4.f;
		
		int flickTransformIdx = clamp(note.width, 1, MAX_FLICK_SPRITES) - 1 + static_cast<int>((note.flick == FlickType::Left || note.flick == FlickType::Right) ? Engine::SpriteType::FlickArrowLeft : Engine::SpriteType::FlickArrowUp);
		auto vPos = ResourceManager::spriteTransform.at(flickTransformIdx).apply(
			orthogQuadvPos(
				Engine::laneToLeft(note.lane) + note.width / 2.f - w,
				Engine::laneToLeft(note.lane) + note.width / 2.f + w,
				1, 1 - 2 * std::abs(w) * scaledAspectRatio
			)
		);
		
		if (PrvConfig::markerAnimation) {
			float t = std::fmod(time, 0.5f) / 0.5f;
			const auto cubicEaseIn = [](float t) { return t * t * t; };
			Color mixedTint = Color{ tint.r, tint.g, tint.b, tint.a * (1 - cubicEaseIn(t)) };
			auto animationVector = DirectX::XMVectorScale(DirectX::XMVectorSet(flickDirection, -2 * scaledAspectRatio, 0.f, 0.f), t);
			auto model = DirectX::XMMatrixTranslationFromVector(animationVector) * DirectX::XMMatrixScaling(y, y, y);

			
			renderer->drawQuad(vPos, model, texture,
				arrowSprite.getX1(), arrowSprite.getX2(), arrowSprite.getY1(), arrowSprite.getY2(),
				mixedTint, (int)ZIndex::FlickArrow
			);
		} else {
			auto model = DirectX::XMMatrixScaling(y, y, y);

			renderer->drawQuad(vPos, model, texture,
				arrowSprite.getX1(), arrowSprite.getX2(), arrowSprite.getY1(), arrowSprite.getY2(),
				tint, (int)ZIndex::FlickArrow
			);
		}
    }
}