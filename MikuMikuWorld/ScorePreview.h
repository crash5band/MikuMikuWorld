#include "ScoreContext.h"
#include "Rendering/Framebuffer.h"
#include "Rendering/Renderer.h"

namespace MikuMikuWorld
{
	class ScorePreviewWindow {
		Framebuffer previewBuffer;
		float scaledAspectRatio;
		std::unique_ptr<Texture> notesTex;

		const Texture& getNoteTexture();
		
		void drawNoteBase(Renderer* renderer, const Note& note, float left, float right, float y, float zScalar = 1);
		void drawTraceDiamond(Renderer* renderer, const Note& note, float y);
		void drawFlickArrow(Renderer* renderer, const Note& note, float y, float cur_time);
	public:
		ScorePreviewWindow(); 
		~ScorePreviewWindow();
		void update(ScoreContext& context, Renderer* renderer);

		void drawNotes(const ScoreContext& context, Renderer* renderer);
		void drawLines(const ScoreContext& context, Renderer* renderer);
		void drawHoldTicks(const ScoreContext& context, Renderer* renderer);
		void drawHoldCurves(const ScoreContext& context, Renderer* renderer);
		void drawStage(Renderer* renderer);
	};
}