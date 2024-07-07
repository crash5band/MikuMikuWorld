#pragma once

namespace MikuMikuWorld
{
	class Framebuffer
	{
	  private:
		unsigned int fbo;
		unsigned int rbo;
		unsigned int buffer;
		unsigned int width;
		unsigned int height;

		void setup();
		void createTexture(unsigned int tex);

	  public:
		Framebuffer(unsigned int w, unsigned int h);
		Framebuffer();

		void clear();
		void bind();
		void dispose();
		void resize(unsigned int w, unsigned int h);

		unsigned int getWidth() const;
		unsigned int getHeight() const;
		unsigned int getTexture() const;
	};
}