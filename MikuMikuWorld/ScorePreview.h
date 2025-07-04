#include "cmath"
#include "ScoreContext.h"
#include "Rendering/Camera.h"
#include "Rendering/Framebuffer.h"
#include "Rendering/Renderer.h"

namespace MikuMikuWorld
{
	class Shader;

	class ScorePreviewWindow {
		Framebuffer previewBuffer;
	public:
		ScorePreviewWindow(); 
		void update(ScoreContext& context, Renderer* renderer);

		void drawNotes(ScoreContext& context, Renderer* renderer);
		void drawNote(const Note& note, float y, const Color& tint, Renderer* renderer);
		void drawStage(Renderer* renderer);
	};
}