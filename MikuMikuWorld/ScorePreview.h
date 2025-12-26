#pragma once
#include "ScoreContext.h"
#include "ScoreEditorTimeline.h"
#include "Rendering/Framebuffer.h"
#include "Rendering/Renderer.h"
#include "Rendering/Camera.h"

namespace MikuMikuWorld
{
	class ScorePreviewBackground
	{
		std::string backgroundFile;
		std::string jacketFile;
		Framebuffer frameBuffer;
		float brightness;
		bool init;
		
		struct DefaultJacket
		{
			static std::array<DirectX::XMFLOAT4, 4> getLeftUV();
			static std::array<DirectX::XMFLOAT4, 4> getRightUV();
			static std::array<DirectX::XMFLOAT4, 4> getLeftMirrorUV();
			static std::array<DirectX::XMFLOAT4, 4> getRightMirrorUV();
			static std::array<DirectX::XMFLOAT4, 4> getCenterUV();
			static std::array<DirectX::XMFLOAT4, 4> getMirrorCenterUV();
		};
		void updateDrawDefaultJacket(Renderer* renderer, const Jacket& jacket);
		public:
		ScorePreviewBackground();
		~ScorePreviewBackground();

		void setBrightness(float value);
		void update(Renderer* renderer, const Jacket& jacket);
		bool shouldUpdate(const Jacket& jacket) const;
		void draw(Renderer* renderer, float scrWidth, float scrHeight) const;		
	};

	class ScorePreviewWindow
	{
		Framebuffer previewBuffer;
		ScorePreviewBackground background;
		float scaledAspectRatio;
		Camera noteEffectsCamera;

		mutable bool fullWindow{};
		bool lastFrameFullWindow{};

		const Texture& getNoteTexture();

		void drawNoteBase(Renderer* renderer, const Note& note, float left, float right, float y, float zScalar = 1);
		void drawTraceDiamond(Renderer* renderer, const Note& note, float left, float right, float y);
		void drawFlickArrow(Renderer* renderer, const Note& note, float y, double cur_time);

		void drawStageCoverMask(Renderer* renderer);
		
		void updateToolbar(ScoreEditorTimeline& timeline, ScoreContext& context) const;
		float getScrollbarWidth() const;
		void updateScrollbar(ScoreEditorTimeline& timeline, ScoreContext& context) const;
	public:
		ScorePreviewWindow(); 
		~ScorePreviewWindow();
		void update(ScoreContext& context, Renderer* renderer);
		void updateUI(ScoreEditorTimeline& timeline, ScoreContext& context);

		void drawNotes(const ScoreContext& context, Renderer* renderer);
		void drawLines(const ScoreContext& context, Renderer* renderer);
		void drawHoldTicks(const ScoreContext& context, Renderer* renderer);
		void drawHoldCurves(const ScoreContext& context, Renderer* renderer);
 		
		void drawStage(Renderer* renderer);
		void drawStageCover(Renderer* renderer);
		void drawStageCoverDecoration(Renderer* renderer);

		void setFullWindow(bool fullScreen);
		
		inline bool isFullWindow() const { return fullWindow; };
		bool wasLastFrameFullWindow() const { return lastFrameFullWindow; }

		void loadNoteEffects(Effect::EffectView& effectView);
	};
}