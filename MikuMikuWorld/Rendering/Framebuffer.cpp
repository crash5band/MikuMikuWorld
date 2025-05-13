#include "Framebuffer.h"
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

namespace MikuMikuWorld
{
	Framebuffer::Framebuffer(unsigned int w, unsigned int h)
		: width{ w }, height{ h }
	{
		setup();
	}

	Framebuffer::Framebuffer()
	{

	}

	unsigned int Framebuffer::getWidth() const
	{
		return width;
	}

	unsigned int Framebuffer::getHeight() const
	{
		return height;
	}

	unsigned int Framebuffer::getTexture() const
	{
		return buffer;
	}

	void Framebuffer::clear()
	{
		GLbitfield clearBits = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
		glClearColor(0.2, 0.2, 0.2, 0.0);
		glClear(clearBits);
	}

	void Framebuffer::bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, width, height);
	}

	void Framebuffer::dispose()
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Framebuffer::resize(unsigned int w, unsigned int h)
	{
		if (width == w && height == h)
			return;

		width = w;
		height = h;

		createTexture(buffer);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	}

	void Framebuffer::createTexture(unsigned int tex)
	{
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	void Framebuffer::setup()
	{
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glGenTextures(1, &buffer);
		createTexture(buffer);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer, 0);

		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			printf("RenderTarget::setup() ERROR: Incomplete framebuffer");

		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}