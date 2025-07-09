#include "ScorePreview.h"
#include "PreviewEngine.h"
#include "Rendering/Camera.h"
#include "ResourceManager.h"
#include "Colors.h"
#include "ImageCrop.h"

namespace MikuMikuWorld
{
	namespace PrvConfig {
		static bool lockAspectRatio = true;
		static float noteSpeed = 1;
		static bool markerAnimation = true;
		static bool drawSimLine = true;
		float ref_alpha = 0.0f;
		float bound[4] = { 0.f, 0.f, 0.f, 0.f };

		static void drawMenu(ScoreContext& context) {
			if(!ImGui::Begin("Config###config_window")) {
				ImGui::End();
				return;
			}
			ImGui::Checkbox("Lock Aspect Ratio", &lockAspectRatio);
			ImGui::Checkbox("Flick Animation", &markerAnimation);
			ImGui::Checkbox("Simultaneous Line", &drawSimLine);
			if (ImGui::SliderFloat("Note Speed", &noteSpeed, 1, 12, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
				context.scorePreviewDrawData.config.noteSpeed = noteSpeed;
				context.scorePreviewDrawData.calculateDrawData(context.score);
			}
			
			ImGui::InputFloat4("Texture Bound", bound);
			ImGui::SliderFloat("Reference", &ref_alpha, 0, 1.f);
			ImGui::SliderInt("Current Tick", &context.currentTick, 0, TICKS_PER_BEAT * 20);
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

    ScorePreviewWindow::~ScorePreviewWindow()
    {
		if(notesTex) notesTex->dispose();
    }

	void ScorePreviewWindow::update(ScoreContext& context, Renderer* renderer) {
		
		PrvConfig::drawMenu(context);
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
		if (PrvConfig::drawSimLine)
			drawLines(context, renderer);
		drawNotes(context, renderer);

		previewBuffer.unblind();
		drawList->AddImage((ImTextureID)(size_t)previewBuffer.getTexture(), position, position + size);
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
		float current_tm = accumulateDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges);
		float scaled_tm = accumulateScaledDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
		const auto& drawData = context.scorePreviewDrawData;
	
		renderer->beginBatch();
		for (auto& note : drawData.drawingNotes) {
			if (scaled_tm < note.visualTime.min || scaled_tm > note.visualTime.max)
				continue;
			const Note& noteData = context.score.notes[note.refID];
			float y = Engine::approach(note.visualTime.min, note.visualTime.max, scaled_tm);
			
			drawNoteBase(noteData, y, noteTint, renderer);
			if (noteData.friction)
				drawTraceDiamond(noteData, y, noteTint, renderer);
			if (noteData.isFlick()) 
				drawFlickArrow(noteData, y, current_tm, context.score, noteTint, renderer);
		}
		renderer->endBatch();
    }

	void ScorePreviewWindow::drawLines(ScoreContext& context, Renderer* renderer)
	{
		float scaled_tm = accumulateScaledDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
		const auto& drawData = context.scorePreviewDrawData;

		const Texture& texture = getNoteTexture();
		const int sprIndex = SPR_SIMULTANEOUS_CONNECTION;
		if (!isArrayIndexInBounds(sprIndex, texture.sprites))
			return;
		const Sprite& sprite = texture.sprites[sprIndex];
		const Transform& lineTransform = ResourceManager::spriteTransform.at((size_t)Engine::SpriteType::SimultaneousLine);
		const Color color (1.f, 1.f, 1.f, 1.f);
		const float noteTop = 1. + Engine::getNoteHeight(), noteBottom = 1. - Engine::getNoteHeight();

		renderer->beginBatch();
		for (auto& line : drawData.drawingLines) {
			if (scaled_tm < line.visualTime.min || scaled_tm > line.visualTime.max)
				continue;
			float y = Engine::approach(line.visualTime.min, line.visualTime.max, scaled_tm);
			auto vPos = lineTransform.apply(perspectiveQuadvPos(line.xPos.min, line.xPos.max, noteTop, noteBottom));
			auto model = DirectX::XMMatrixScaling(y, y, 1.f);
			renderer->drawQuad(vPos, model, texture, sprite.getX1(), sprite.getX2(), sprite.getY1(), sprite.getY2());
		}
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

		const Sprite& sprite = texture.sprites[sprIndex];

		const float noteHeight = Engine::getNoteHeight();
		float noteLeft = Engine::laneToLeft(note.lane), noteRight = Engine::laneToLeft(note.lane) + note.width;
		const float noteTop = 1. - noteHeight, noteBottom = 1. + noteHeight;
		int zIndex = Engine::getZIndex(
			!note.friction ? Engine::Layer::BASE_NOTE : Engine::Layer::TICK_NOTE,
			Engine::laneToLeft(note.lane) + note.width / 2.f, y
		);

		auto model = DirectX::XMMatrixScaling(y, y, 1.f);
		std::array<DirectX::XMFLOAT4, 4> vPos;
		// Middle
		vPos = ResourceManager::spriteTransform.at((int)Engine::SpriteType::NoteMiddle).apply(perspectiveQuadvPos(noteLeft + 0.3f, noteRight - 0.3f, noteTop, noteBottom));
		renderer->drawQuad(vPos, model, texture,
			sprite.getX1() + NOTE_SIDE_WIDTH, sprite.getX2() - NOTE_SIDE_WIDTH, sprite.getY1(), sprite.getY2(),
			tint, zIndex
		);

		// Left slice
		vPos = ResourceManager::spriteTransform.at((int)Engine::SpriteType::NoteLeft).apply(perspectiveQuadvPos(noteLeft, noteLeft + 0.3f, noteTop, noteBottom));
		renderer->drawQuad(vPos, model, texture,
			sprite.getX1(), sprite.getX1() + NOTE_SIDE_WIDTH, sprite.getY1(), sprite.getY2(),
			tint, zIndex
		);
		
		// Right slice
		vPos = ResourceManager::spriteTransform.at((int)Engine::SpriteType::NoteRight).apply(perspectiveQuadvPos(noteRight - 0.3f, noteRight, noteTop, noteBottom));
		renderer->drawQuad(vPos, model, texture,
			sprite.getX2() - NOTE_SIDE_WIDTH, sprite.getX2(), sprite.getY1(), sprite.getY2(),
			tint, zIndex
		);
    }

    void ScorePreviewWindow::drawTraceDiamond(const Note &note, float y, const Color &tint, Renderer *renderer)
    {
		const Texture& texture = getNoteTexture();
		int frictionSprIndex = getFrictionSpriteIndex(note);
		if (!isArrayIndexInBounds(frictionSprIndex, texture.sprites))
			return;
		const Sprite& frictionSpr = texture.sprites[frictionSprIndex];

		const float w = Engine::getNoteHeight() / scaledAspectRatio;
		const float noteTop = 1. + Engine::getNoteHeight(), noteBottom = 1. - Engine::getNoteHeight();
		int zIndex = Engine::getZIndex(Engine::Layer::DIAMOND, Engine::laneToLeft(note.lane) + note.width / 2.f, y);

		const Transform& transform = ResourceManager::spriteTransform.at((int)Engine::SpriteType::TraceDiamond);
		auto vPos = transform.apply(orthogQuadvPos(
			Engine::laneToLeft(note.lane) + note.width / 2.f - w,
			Engine::laneToLeft(note.lane) + note.width / 2.f + w,
			noteTop, noteBottom)
		);
		auto model = DirectX::XMMatrixScaling(y, y, 1.f);
		
		renderer->drawQuad(vPos, model, texture, frictionSpr.getX1(), frictionSpr.getX2(), frictionSpr.getY1(), frictionSpr.getY2(), tint, zIndex);
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
		int zIndex = Engine::getZIndex(Engine::Layer::FLICK_ARROW, Engine::laneToLeft(note.lane) + note.width / 2.f, y);
		
		if (PrvConfig::markerAnimation) {
			float t = std::fmod(time, 0.5f) / 0.5f;
			const auto cubicEaseIn = [](float t) { return t * t * t; };
			Color mixedTint = Color{ tint.r, tint.g, tint.b, tint.a * (1 - cubicEaseIn(t)) };
			auto animationVector = DirectX::XMVectorScale(DirectX::XMVectorSet(flickDirection, -2 * scaledAspectRatio, 0.f, 0.f), t);
			auto model = DirectX::XMMatrixTranslationFromVector(animationVector) * DirectX::XMMatrixScaling(y, y, 1.f);

			
			renderer->drawQuad(vPos, model, texture,
				arrowSprite.getX1(), arrowSprite.getX2(), arrowSprite.getY1(), arrowSprite.getY2(),
				mixedTint, zIndex
			);
		} else {
			auto model = DirectX::XMMatrixScaling(y, y, 1.f);

			renderer->drawQuad(vPos, model, texture,
				arrowSprite.getX1(), arrowSprite.getX2(), arrowSprite.getY1(), arrowSprite.getY2(),
				tint, zIndex
			);
		}
    }
}