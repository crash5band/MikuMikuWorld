#pragma once
#include "Rendering/Texture.h"
#include "Rendering/Framebuffer.h"
#include <string>
#include <memory>

namespace MikuMikuWorld
{
	class Renderer;
	class Texture;
	struct Vector2;

	class Background
	{
	private:
		Texture texture;
		std::unique_ptr<Framebuffer> framebuffer;

		float blur;
		float brightness;

		float width;
		float height;

		bool dirty;
		bool useJacketBg;

		void resizeByRatio(float& w, float& h, const Vector2& tgt, bool vertical);

	public:
		Background();

		void load(const Texture& tex);
		void resize(Vector2 target);
		void process(Renderer* renderer);
		
		int getWidth() const;
		int getHeight() const;
		int getTextureID() const;

		float getBlur() const;
		void setBlur(float b);

		float getBrightness() const;
		void setBrightness(float b);

		bool isDirty() const;
	};
}


