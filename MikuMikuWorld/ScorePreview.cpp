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
		static float noteSpeed = 6.f;
		static bool markerAnimation = true;
		static bool drawSimLine = true;
		static bool holdAnimation = false;
		static float holdAlpha = 1.f;

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
			ImGui::Checkbox("Connector Animation", &holdAnimation);
			ImGui::SliderFloat("Connector Alpha", &holdAlpha, 0, 1);
			
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

	static const int HOLD_XCUTOFF = 36;
	static const int GUIDE_XCUTOFF = 3;
	static const int GUIDE_Y_TOP_CUTOFF = -41;
	static const int GUIDE_Y_BOTTOM_CUTOFF = -12;
	static Color defaultTint { 1.f, 1.f, 1.f, 1.f };

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

		renderer->beginBatch();
		drawStage(renderer);
		renderer->endBatch();

		renderer->beginBatch();
		if (PrvConfig::drawSimLine)
			drawLines(context, renderer);
		drawHoldCurves(context, renderer);
		renderer->endBatch();

		renderer->beginBatch();
		drawHoldTicks(context, renderer);
		drawNotes(context, renderer);
		renderer->endBatch();
		
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
		
		renderer->drawRectangle({-left, top}, {width, height}, stage, 0, 2048, 0, 1176, defaultTint, 0);
    }

    void ScorePreviewWindow::drawNotes(ScoreContext& context, Renderer *renderer)
    {
		float current_tm = accumulateDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges);
		float scaled_tm = accumulateScaledDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
		const auto& drawData = context.scorePreviewDrawData;
	
		for (auto& note : drawData.drawingNotes) {
			if (scaled_tm < note.visualTime.min || scaled_tm > note.visualTime.max)
				continue;
			const Note& noteData = context.score.notes[note.refID];
			float y = Engine::approach(note.visualTime.min, note.visualTime.max, scaled_tm);
			
			drawNoteBase(noteData, y, renderer);
			if (noteData.friction)
				drawTraceDiamond(noteData, y, renderer);
			if (noteData.isFlick()) 
				drawFlickArrow(noteData, y, current_tm, context.score, renderer);
		}
    }

	void ScorePreviewWindow::drawLines(ScoreContext& context, Renderer* renderer)
	{
		if (noteTextures.notes == -1)
			return;
		float scaled_tm = accumulateScaledDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
		const auto& drawData = context.scorePreviewDrawData;

		const Texture& texture = getNoteTexture();
		size_t sprIndex = SPR_SIMULTANEOUS_CONNECTION;
		if (!isArrayIndexInBounds(sprIndex, texture.sprites))
			return;
		const Sprite& sprite = texture.sprites[sprIndex];
		size_t transIndex = static_cast<size_t>(Engine::SpriteType::SimultaneousLine);
		if (!isArrayIndexInBounds(transIndex, ResourceManager::spriteTransform))
			return;
		const Transform& lineTransform = ResourceManager::spriteTransform[transIndex];
		const Color color (1.f, 1.f, 1.f, 1.f);
		const float noteTop = 1. + Engine::getNoteHeight(), noteBottom = 1. - Engine::getNoteHeight();

		for (auto& line : drawData.drawingLines) {
			if (scaled_tm < line.visualTime.min || scaled_tm > line.visualTime.max)
				continue;
			float y = Engine::approach(line.visualTime.min, line.visualTime.max, scaled_tm);
			auto vPos = lineTransform.apply(perspectiveQuadvPos(line.xPos.min, line.xPos.max, noteTop, noteBottom));
			auto model = DirectX::XMMatrixScaling(y, y, 1.f);
			renderer->drawQuad(vPos, model, texture, sprite.getX1(), sprite.getX2(), sprite.getY1(), sprite.getY2());
		}
	}

    void ScorePreviewWindow::drawHoldTicks(ScoreContext &context, Renderer *renderer)
    {
		if (noteTextures.notes == -1)
			return;
		float scaled_tm = accumulateScaledDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
		const float w = Engine::getNoteHeight() / scaledAspectRatio;
		const float noteTop = 1. + Engine::getNoteHeight(), noteBottom = 1. - Engine::getNoteHeight();
		const Texture& texture = getNoteTexture();

		for (auto& tick : context.scorePreviewDrawData.drawingHoldTicks) {
			if (scaled_tm < tick.visualTime.min || scaled_tm > tick.visualTime.max)
				continue;
			const Note& note = context.score.notes[tick.refID];
			int sprIndex = getNoteSpriteIndex(note);
			if (!isArrayIndexInBounds(sprIndex, texture.sprites))
				continue;
			const Sprite& sprite = texture.sprites[sprIndex];
			size_t transIndex = static_cast<size_t>(Engine::SpriteType::HoldTick);
			if (!isArrayIndexInBounds(transIndex, ResourceManager::spriteTransform))
				return;
			const Transform& transform = ResourceManager::spriteTransform[transIndex];
			float y = Engine::approach(tick.visualTime.min, tick.visualTime.max, scaled_tm);
			
			auto vPos = transform.apply(orthogQuadvPos(
				tick.center - w, tick.center + w,
				noteTop, noteBottom
			));
			auto model = DirectX::XMMatrixScaling(y, y, 1.f);
			int zIndex = Engine::getZIndex(Engine::Layer::DIAMOND, tick.center, y);
			renderer->drawQuad(vPos, model, texture, sprite.getX1(), sprite.getX2(), sprite.getY1(), sprite.getY2(), defaultTint, zIndex);
		}
    }

    void ScorePreviewWindow::drawHoldCurves(ScoreContext &context, Renderer *renderer)
    {
		float total_tm = accumulateScaledDuration(context.scorePreviewDrawData.maxTicks, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
		float curr_tm = accumulateDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges);
		float scaled_tm = accumulateScaledDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
		float noteDuration = Engine::getNoteDuration(context.scorePreviewDrawData.config.noteSpeed);
		const auto& drawData = context.scorePreviewDrawData;
		
		for (auto& segment : drawData.drawingHoldSegments) {
			if (scaled_tm < (segment.headTime - noteDuration) || scaled_tm > segment.tailTime)
				continue;
			
			Note& holdEndNote = context.score.notes[segment.endID];
			Note& holdStartNote = context.score.notes[holdEndNote.parentID];
			float holdStartCenter = Engine::getNoteCenter(holdStartNote);
			bool isActivated = curr_tm >= segment.startTime;
			int textureID = segment.isGuide ? noteTextures.touchLine : noteTextures.holdPath;
			if (textureID == -1)
				continue;
			const Texture& pathTexture = ResourceManager::textures[textureID];
			const int sprIndex = (!holdStartNote.critical ? 1 : 3);
			if (!isArrayIndexInBounds(sprIndex, pathTexture.sprites))
				continue;
			const Sprite& normalSprite = pathTexture.sprites[sprIndex];

			// Clamp the segment to be within the visible stage
			float segmentStart = std::max(segment.headTime, scaled_tm);
			float segmentEnd = std::min(segment.tailTime, scaled_tm + noteDuration);
			
			int steps = segment.ease == EaseType::Linear ? 10 : 15;
			auto ease = getEaseFunction(segment.ease);
			Note& startNote = context.score.notes[segment.headID];
			float startLeft = Engine::laneToLeft(startNote.lane);
			float startRight = Engine::laneToLeft(startNote.lane) + startNote.width;
			Note& endNote = context.score.notes[segment.tailID];
			float endLeft = Engine::laneToLeft(endNote.lane);
			float endRight = Engine::laneToLeft(endNote.lane) + endNote.width;

			float holdStartTime, holdEndTime;
			if (segment.isGuide)
			{
				holdStartTime = accumulateScaledDuration(holdStartNote.tick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
				holdEndTime = accumulateScaledDuration(holdEndNote.tick, TICKS_PER_BEAT, context.score.tempoChanges, context.score.hiSpeedChanges);
			}

			for (int i = 0; i < steps; i++) {
				float from_percentage = float(i) / steps, to_percentage = float(i + 1) / steps;
				float stepStart = lerp(segmentStart, segmentEnd, from_percentage);
				float stepEnd = lerp(segmentStart, segmentEnd, to_percentage);

				float stepTop    = Engine::approach(stepStart - noteDuration, stepStart, scaled_tm);
				float stepBottom = Engine::approach(stepEnd   - noteDuration, stepEnd  , scaled_tm);

				// Since we clamped the segment to only the visible parts
				// We need to figure out which part of the segment we are on
				float t_stepStart = unlerp(segment.headTime, segment.tailTime, stepStart);
				float   t_stepEnd = unlerp(segment.headTime, segment.tailTime, stepEnd);

				float stepStartLeft = ease(startLeft, endLeft, t_stepStart);
				float   stepEndLeft = ease(startLeft, endLeft, t_stepEnd);
				float stepStartRight = ease(startRight, endRight, t_stepStart);
				float   stepEndRight = ease(startRight, endRight, t_stepEnd);

				auto vPos = perspectiveQuadvPos(stepStartLeft, stepEndLeft, stepStartRight, stepEndRight, stepTop, stepBottom);
				auto model = DirectX::XMMatrixIdentity();
				float alpha = PrvConfig::holdAlpha;
				float zIndex = Engine::getZIndex(Engine::Layer::HOLD_PATH, holdStartCenter, segment.headTime / total_tm);

				float spr_x1, spr_x2, spr_y1, spr_y2;
				if (segment.isGuide) {
					spr_x1 = normalSprite.getX1() + GUIDE_XCUTOFF;
					spr_x2 = normalSprite.getX2() - GUIDE_XCUTOFF;
					spr_y1 = lerp(normalSprite.getY2() - GUIDE_Y_BOTTOM_CUTOFF, normalSprite.getY1() + GUIDE_Y_TOP_CUTOFF, unlerp(holdStartTime, holdEndTime, stepStart));
					spr_y2 = lerp(normalSprite.getY2() - GUIDE_Y_BOTTOM_CUTOFF, normalSprite.getY1() + GUIDE_Y_TOP_CUTOFF, unlerp(holdStartTime, holdEndTime, stepEnd));
				}
				else {
					spr_x1 = normalSprite.getX1() + HOLD_XCUTOFF;
					spr_x2 = normalSprite.getX2() - HOLD_XCUTOFF;
					spr_y1 = normalSprite.getY1();
					spr_y2 = normalSprite.getY2();
				}

				if (PrvConfig::holdAnimation && isActivated && isArrayIndexInBounds(sprIndex - 1, pathTexture.sprites)) {
					const Sprite& activeSprite = pathTexture.sprites[sprIndex - 1];
					const int norm2ActiveOffset = activeSprite.getY1() - normalSprite.getY1();
					float delta_tm = curr_tm - segment.startTime;
					float normalAplha = (std::cos(delta_tm * 2 * M_PI) + 2) / 3.;

					renderer->drawQuad(vPos, model, pathTexture, spr_x1, spr_x2, spr_y1, spr_y2, defaultTint.scaleAlpha(alpha * normalAplha), zIndex);
					renderer->drawQuad(vPos, model, pathTexture, spr_x1, spr_x2, spr_y1 + norm2ActiveOffset, spr_y2 + norm2ActiveOffset, defaultTint.scaleAlpha(alpha * (1.f - normalAplha)), zIndex);
				}
				else
					renderer->drawQuad(vPos, model, pathTexture, spr_x1, spr_x2, spr_y1, spr_y2, defaultTint.scaleAlpha(alpha), zIndex);
			}
		}
    }

    void ScorePreviewWindow::drawNoteBase(const Note& note, float y, Renderer* renderer)
    {
		const int NOTE_SIDE_WIDTH = 93;
		if (noteTextures.notes == -1)
			return;

		const Texture& texture = getNoteTexture();
		const int sprIndex = getNoteSpriteIndex(note);
		if (!isArrayIndexInBounds(sprIndex, texture.sprites))
			return;
		const Sprite& sprite = texture.sprites[sprIndex];

		size_t transIndex = static_cast<size_t>(Engine::SpriteType::NoteMiddle);
		if (!isArrayIndexInBounds(transIndex, ResourceManager::spriteTransform))
			return;
		const Transform& mTransform = ResourceManager::spriteTransform[transIndex];

		transIndex = static_cast<size_t>(Engine::SpriteType::NoteLeft);
		if (!isArrayIndexInBounds(transIndex, ResourceManager::spriteTransform))
			return;
		const Transform& lTransform = ResourceManager::spriteTransform[transIndex];

		transIndex = static_cast<size_t>(Engine::SpriteType::NoteRight);
		if (!isArrayIndexInBounds(transIndex, ResourceManager::spriteTransform))
			return;
		const Transform& rTransform = ResourceManager::spriteTransform[transIndex];

		const float noteHeight = Engine::getNoteHeight();
		float noteLeft = Engine::laneToLeft(note.lane), noteRight = Engine::laneToLeft(note.lane) + note.width;
		const float noteTop = 1. - noteHeight, noteBottom = 1. + noteHeight;
		int zIndex = Engine::getZIndex(
			!note.friction ? Engine::Layer::BASE_NOTE : Engine::Layer::TICK_NOTE,
			Engine::getNoteCenter(note), y
		);

		auto model = DirectX::XMMatrixScaling(y, y, 1.f);
		std::array<DirectX::XMFLOAT4, 4> vPos;
		// Middle
		vPos = mTransform.apply(perspectiveQuadvPos(noteLeft + 0.3f, noteRight - 0.3f, noteTop, noteBottom));
		renderer->drawQuad(vPos, model, texture,
			sprite.getX1() + NOTE_SIDE_WIDTH, sprite.getX2() - NOTE_SIDE_WIDTH, sprite.getY1(), sprite.getY2(),
			defaultTint, zIndex
		);

		// Left slice
		vPos = lTransform.apply(perspectiveQuadvPos(noteLeft, noteLeft + 0.3f, noteTop, noteBottom));
		renderer->drawQuad(vPos, model, texture,
			sprite.getX1(), sprite.getX1() + NOTE_SIDE_WIDTH, sprite.getY1(), sprite.getY2(),
			defaultTint, zIndex
		);
		
		// Right slice
		vPos = rTransform.apply(perspectiveQuadvPos(noteRight - 0.3f, noteRight, noteTop, noteBottom));
		renderer->drawQuad(vPos, model, texture,
			sprite.getX2() - NOTE_SIDE_WIDTH, sprite.getX2(), sprite.getY1(), sprite.getY2(),
			defaultTint, zIndex
		);
    }

    void ScorePreviewWindow::drawTraceDiamond(const Note &note, float y, Renderer *renderer)
    {
		if (noteTextures.notes == -1)
			return;
		const Texture& texture = getNoteTexture();
		int frictionSprIndex = getFrictionSpriteIndex(note);
		if (!isArrayIndexInBounds(frictionSprIndex, texture.sprites))
			return;
		const Sprite& frictionSpr = texture.sprites[frictionSprIndex];
		size_t transIndex = static_cast<size_t>(Engine::SpriteType::TraceDiamond);
		if (!isArrayIndexInBounds(transIndex, ResourceManager::spriteTransform))
			return;
		const Transform& transform = ResourceManager::spriteTransform[transIndex];

		const float w = Engine::getNoteHeight() / scaledAspectRatio;
		const float noteTop = 1. + Engine::getNoteHeight(), noteBottom = 1. - Engine::getNoteHeight();
		int zIndex = Engine::getZIndex(Engine::Layer::DIAMOND, Engine::getNoteCenter(note), y);

		auto vPos = transform.apply(orthogQuadvPos(
			Engine::getNoteCenter(note) - w,
			Engine::getNoteCenter(note) + w,
			noteTop, noteBottom)
		);
		auto model = DirectX::XMMatrixScaling(y, y, 1.f);
		
		renderer->drawQuad(vPos, model, texture, frictionSpr.getX1(), frictionSpr.getX2(), frictionSpr.getY1(), frictionSpr.getY2(), defaultTint, zIndex);
    }

    void ScorePreviewWindow::drawFlickArrow(const Note &note, float y, float time, const Score &score, Renderer *renderer)
    {
		if (noteTextures.notes == -1)
			return;

		const int MAX_FLICK_SPRITES = 6;
		const Texture& texture = getNoteTexture();
		const int sprIndex = getFlickArrowSpriteIndex(note);
		if (!isArrayIndexInBounds(sprIndex, texture.sprites))
			return;
		const Sprite& arrowSprite = texture.sprites[sprIndex];
		size_t flickTransformIdx = clamp(note.width, 1, MAX_FLICK_SPRITES) - 1 + static_cast<int>((note.flick == FlickType::Left || note.flick == FlickType::Right) ? Engine::SpriteType::FlickArrowLeft : Engine::SpriteType::FlickArrowUp);
		if (!isArrayIndexInBounds(flickTransformIdx, ResourceManager::spriteTransform))
			return;
		const Transform& transform = ResourceManager::spriteTransform[flickTransformIdx];

		const int flickDirection = note.flick == FlickType::Left ? -1 : (note.flick == FlickType::Right ? 1 : 0);

		const float w = clamp(note.width, 0, MAX_FLICK_SPRITES) * (note.flick == FlickType::Right ? -1 : 1) / 4.f;
		
		auto vPos = transform.apply(
			orthogQuadvPos(
				Engine::getNoteCenter(note) - w,
				Engine::getNoteCenter(note) + w,
				1, 1 - 2 * std::abs(w) * scaledAspectRatio
			)
		);
		int zIndex = Engine::getZIndex(Engine::Layer::FLICK_ARROW, Engine::getNoteCenter(note), y);
		
		if (PrvConfig::markerAnimation) {
			float t = std::fmod(time, 0.5f) / 0.5f;
			const auto cubicEaseIn = [](float t) { return t * t * t; };
			auto animationVector = DirectX::XMVectorScale(DirectX::XMVectorSet(flickDirection, -2 * scaledAspectRatio, 0.f, 0.f), t);
			auto model = DirectX::XMMatrixTranslationFromVector(animationVector) * DirectX::XMMatrixScaling(y, y, 1.f);
			
			renderer->drawQuad(vPos, model, texture,
				arrowSprite.getX1(), arrowSprite.getX2(), arrowSprite.getY1(), arrowSprite.getY2(),
				defaultTint.scaleAlpha(1 - cubicEaseIn(t)), zIndex
			);
		} else {
			auto model = DirectX::XMMatrixScaling(y, y, 1.f);

			renderer->drawQuad(vPos, model, texture,
				arrowSprite.getX1(), arrowSprite.getX2(), arrowSprite.getY1(), arrowSprite.getY2(),
				defaultTint, zIndex
			);
		}
    }
}