#include "ScoreContext.h"
#include "Rendering/Framebuffer.h"
#include "Rendering/Renderer.h"

namespace MikuMikuWorld
{
	class Shader;

	class ScorePreviewWindow {
		Framebuffer previewBuffer;
		float scaledAspectRatio;
		std::unique_ptr<Texture> notesTex;

		const Texture& getNoteTexture();
	public:
		ScorePreviewWindow(); 
		~ScorePreviewWindow();
		void update(ScoreContext& context, Renderer* renderer);

		void drawNotes(ScoreContext& context, Renderer* renderer);
		void drawNoteBase(const Note& note, float y, const Color& tint, Renderer* renderer);
		void drawFlickArrow(const Note& note, float y, float cur_time, const Score& score, const Color& tint, Renderer* renderer);
		void drawStage(Renderer* renderer);
	};
}