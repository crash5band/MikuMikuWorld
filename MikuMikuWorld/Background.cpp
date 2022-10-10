#include "Background.h"
#include "Math.h"
#include "Color.h"
#include "ResourceManager.h"
#include "Rendering/Renderer.h"
#include "Rendering/Framebuffer.h"

namespace MikuMikuWorld
{
	Background::Background() :
		blur{ 0.0f }, brightness{ 0.4f }, width{ 0 }, height{ 0 }, dirty{ false }
	{
		framebuffer = std::make_unique<Framebuffer>(1, 1);
	}

	void Background::load(const Texture& tex)
	{
		texture = tex;
		framebuffer->resize(texture.getWidth(), texture.getHeight());

		dirty = true;
	}

	void Background::resizeByRatio(float& w, float& h, const Vector2& tgt, bool vertical)
	{
		if (vertical)
		{
			float ratio = tgt.y / h;
			h = tgt.y;
			w *= ratio;
		}
		else
		{
			float ratio = tgt.x / w;
			w = tgt.x;
			h *= ratio;
		}
	}

	void Background::resize(Vector2 target)
	{
		float w = texture.getWidth();
		float h = texture.getHeight();
		float tgtAspect = target.x / target.y;

		if (tgtAspect > 1.0f)
			resizeByRatio(w, h, target, false);
		else if (tgtAspect < 1.0f)
			resizeByRatio(w, h, target, true);
		else
			w = h = target.x;

		// for non-square aspect ratios
		if (h < target.y)
			resizeByRatio(w, h, target, true);
		else if (w < target.x)
			resizeByRatio(w, h, target, false);

		width = w;
		height = h;
	}

	void Background::process(Renderer* renderer)
	{
		int s = ResourceManager::getShader("basic2d");
		if (s == -1)
			return;

		int w = texture.getWidth();
		int h = texture.getHeight();

		if (w < 1 || h < 1)
			return;

		Shader* blur = ResourceManager::shaders[s];
		blur->use();
		blur->setMatrix4("projection", DirectX::XMMatrixOrthographicRH(w, -h, 0.001f, 100));

		framebuffer->bind();
		framebuffer->clear();
		renderer->beginBatch();

		// align background quad to canvas position
		Vector2 posR;
		Vector2 size(w, h);
		Color tint(brightness, brightness, brightness, 1.0f);

		renderer->drawSprite(posR, 0.0f, size, AnchorType::MiddleCenter, texture, 0, tint);
		renderer->endBatch();
		glDisable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		dirty = false;
	}

	int Background::getWidth() const
	{
		return width;
	}

	int Background::getHeight() const
	{
		return height;
	}

	int Background::getTextureID() const
	{
		return framebuffer->getTexture();
	}

	float Background::getBlur() const
	{
		return blur;
	}

	void Background::setBlur(float b)
	{
		blur = b;
		dirty = true;
	}

	float Background::getBrightness() const
	{
		return brightness;
	}

	void Background::setBrightness(float b)
	{
		brightness = b;
		dirty = true;
	}

	bool Background::isDirty() const
	{
		return dirty;
	}
}
