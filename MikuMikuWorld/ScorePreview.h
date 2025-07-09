#include "ScoreContext.h"
#include "Rendering/Framebuffer.h"
#include "Rendering/Renderer.h"
#include "PreviewEngine.h"

namespace MikuMikuWorld
{
	class Shader;

	class ScorePreviewWindow {
		Framebuffer previewBuffer;
		float scaledAspectRatio;
		std::unique_ptr<Texture> notesTex;

		const Texture& getNoteTexture();
		
		void drawNoteBase(const Note& note, float y, const Color& tint, Renderer* renderer);
		void drawTraceDiamond(const Note& note, float y, const Color& tint, Renderer* renderer);
		void drawFlickArrow(const Note& note, float y, float cur_time, const Score& score, const Color& tint, Renderer* renderer);
	public:
		ScorePreviewWindow(); 
		~ScorePreviewWindow();
		void update(ScoreContext& context, Renderer* renderer);

		void drawNotes(ScoreContext& context, Renderer* renderer);
		void drawLines(ScoreContext& context, Renderer* renderer);
		void drawStage(Renderer* renderer);
	};
}