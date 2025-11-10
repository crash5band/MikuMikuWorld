#include "Renderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>

namespace MikuMikuWorld
{
	Renderer::Renderer() : vBuffer{ maxQuads }, mvBuffer{ 24 }
	{
		vBuffer.setup();
		mvBuffer.setup();
		quads.reserve(maxQuads);
		init();
	}

	void Renderer::init()
	{
		// order: top-right, bottom-right, bottom-left, top-left
		vPos[0] = { 0.5f,  0.5f, 0.0f, 1.0f };
		vPos[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
		vPos[2] = { -0.5f, -0.5f, 0.0f, 1.0f };
		vPos[3] = { -0.5f,  0.5f, 0.0f, 1.0f };
	}

	void Renderer::setAnchor(AnchorType type)
	{
		float top = 0.0f;
		float bottom = -1.0f;
		float left = 0.0f;
		float right = 1.0f;

		switch ((uint8_t)type / 3)
		{
		case 1:
			top = 0.5f; bottom = -0.5f;
			break;

		case 2:
			top = 1.0f; bottom = 0.0f;
			break;

		default: break;
		}

		switch ((uint8_t)type % 3)
		{
		case 1: left = -0.5f; right = 0.5f;
			break;

		case 2:
			left = -1.0f; right = 0.0f;
			break;

		default: break;
		}

		vPos[0] = { right, top, 0.0f, 1.0f };
		vPos[1] = { right, bottom, 0.0f, 1.0f };
		vPos[2] = { left, bottom, 0.0f, 1.0f };
		vPos[3] = { left, top, 0.0f, 1.0f };
	}

	void Renderer::setUVCoords(const Texture& tex, float x1, float x2, float y1, float y2, float z)
	{
		float left = x1 / tex.getWidth();
		float right = x2 / tex.getWidth();
		float top = y1 / tex.getHeight();
		float bottom = y2 / tex.getHeight();

		uvCoords[0] = { right, top, z, 0.0f };
		uvCoords[1] = { right, bottom, z, 0.0f };
		uvCoords[2] = { left, bottom, z, 0.0f };
		uvCoords[3] = { left, top, z, 0.0f };
	}

	DirectX::XMMATRIX Renderer::getModelMatrix(const Vector2& pos, const float rot, const Vector2& sz)
	{
		DirectX::XMMATRIX model = DirectX::XMMatrixIdentity();
		model *= DirectX::XMMatrixScaling(sz.x, sz.y, 1.0f);
		model *= DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(rot));
		model *= DirectX::XMMatrixTranslation(pos.x, pos.y, 0.0f);

		return model;
	}

	void Renderer::drawSprite(const Vector2& pos, float rot, const Vector2& sz, AnchorType anchor,
		const Texture& tex, int spr, const Color& tint, int z)
	{
		const Sprite& s = tex.sprites[spr];
		drawSprite(pos, rot, sz, anchor, tex, s.getX1(), s.getX2(), s.getY1(), s.getY2(), tint, z);
	}

	void Renderer::drawSprite(const Vector2& pos, float rot, const Vector2& sz, AnchorType anchor,
		const Texture& tex, float x1, float x2, float y1, float y2, const Color& tint, int z)
	{
		DirectX::XMMATRIX model = getModelMatrix(pos, rot, sz);
		DirectX::XMFLOAT4 color{ tint.r, tint.g, tint.b, tint.a };
		setUVCoords(tex, x1, x2, y1, y2, 0);
		setAnchor(anchor);

		pushQuad(vPos, uvCoords, model, color, tex.getID(), z);
	}

	void Renderer::drawQuad(const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector2& p4,
		const Texture& tex, float x1, float x2, float y1, float y2, const Color& tint, int z)
	{
		setUVCoords(tex, x1, x2, y1, y2, 0);
		vPos[0] = { p4.x, p4.y, 0.0f, 1.0f };
		vPos[1] = { p2.x, p2.y, 0.0f, 1.0f };
		vPos[2] = { p1.x, p1.y, 0.0f, 1.0f };
		vPos[3] = { p3.x, p3.y, 0.0f, 1.0f };
		DirectX::XMFLOAT4 color{ tint.r, tint.g, tint.b, tint.a };

		pushQuad(vPos, uvCoords, DirectX::XMMatrixIdentity(), color, tex.getID(), z);
	}

	void Renderer::drawQuad(const std::array<DirectX::XMFLOAT4, 4>& pos, const DirectX::XMMATRIX& m,
		const Texture& tex, float x1, float x2, float y1, float y2, const Color& tint, int z)
	{
		setUVCoords(tex, x1, x2, y1, y2, 0.0f);
		DirectX::XMFLOAT4 color{ tint.r, tint.g, tint.b, tint.a };

		pushQuad(pos, uvCoords, m, color, tex.getID(), z);
	}

	void Renderer::drawQuadWithBlend(const DirectX::XMMATRIX& m, const Texture& tex, const Sprite& s,
		const Color& tint, int z, float blend)
	{
		setAnchor(AnchorType::MiddleCenter);
		setUVCoords(tex, s.getX1(), s.getX2(), s.getY1(), s.getY2(), blend);
		DirectX::XMFLOAT4 color{ tint.r, tint.g, tint.b, tint.a };

		pushQuad(vPos, uvCoords, m, color, tex.getID(), z);
	}

	void Renderer::drawQuadWithBlend(const DirectX::XMMATRIX& m, const Texture& tex, int splitX, int splitY, int frame,
		const Color& tint, int z, float blend)
	{
		setAnchor(AnchorType::MiddleCenter);
		
		int row = frame / splitX;
		int col = frame % splitX;
		int w = tex.getWidth() / splitX;
		int h = tex.getHeight() / splitY;

		float x1 = col * w;
		float x2 = x1 + w;
		float y1 = row * h;
		float y2 = y1 + h;

		setUVCoords(tex, x1, x2, y1, y2, blend);
		DirectX::XMFLOAT4 color{ tint.r, tint.g, tint.b, tint.a };

		pushQuad(vPos, uvCoords, m, color, tex.getID(), z);
	}

	void Renderer::drawRectangle(Vector2 position, Vector2 size, const Texture& tex, float x1, float x2, float y1, float y2, Color tint, int z)
	{
		Vector2 p1{ position.x, position.y };
		Vector2 p2{ position.x + size.x, position.y };
		Vector2 p3{ position.x + size.x, position.y + size.y };
		Vector2 p4{ position.x, position.y + size.y };

		drawQuad(p4, p3, p1, p2, tex, x1, x2, y1, y2, tint, z);
	}

	void Renderer::pushQuad(const std::array<DirectX::XMFLOAT4, 4>& pos, const std::array<DirectX::XMFLOAT4, 4>& uv,
		const DirectX::XMMATRIX& m, const DirectX::XMFLOAT4& col, int tex, int z)
	{
		Quad<Vertex> q{ tex, z };
		q.vertices[0].position = DirectX::XMVector2Transform(DirectX::XMLoadFloat4(&pos[0]), m);
		q.vertices[0].color = DirectX::XMLoadFloat4(&col);
		q.vertices[0].uv = DirectX::XMLoadFloat4(&uv[0]);

		q.vertices[1].position = DirectX::XMVector2Transform(DirectX::XMLoadFloat4(&pos[1]), m);
		q.vertices[1].color = DirectX::XMLoadFloat4(&col);
		q.vertices[1].uv = DirectX::XMLoadFloat4(&uv[1]);

		q.vertices[2].position = DirectX::XMVector2Transform(DirectX::XMLoadFloat4(&pos[2]), m);
		q.vertices[2].color = DirectX::XMLoadFloat4(&col);
		q.vertices[2].uv = DirectX::XMLoadFloat4(&uv[2]);

		q.vertices[3].position = DirectX::XMVector2Transform(DirectX::XMLoadFloat4(&pos[3]), m);
		q.vertices[3].color = DirectX::XMLoadFloat4(&col);
		q.vertices[3].uv = DirectX::XMLoadFloat4(&uv[3]);

		quads.push_back(std::move(q));

		++numQuads;
		numVertices += 4;
		numIndices += 6;
	}

    void Renderer::pushQuadMasked(const std::array<DirectX::XMFLOAT4, 4> &pos, const std::array<DirectX::XMFLOAT4, 4> &UV, const std::array<DirectX::XMFLOAT4, 4> &maskUV, const DirectX::XMFLOAT4 &col, int tex, int maskTex)
    {
		Quad<MaskVertex> mQuad { tex, maskTex };
		for (size_t i = 0; i < 4; i++)
		{
			mQuad.vertices[i].position = DirectX::XMLoadFloat4(&pos[i]);
			mQuad.vertices[i].uvBase = DirectX::XMLoadFloat4(&UV[i]);
			mQuad.vertices[i].uvMask = DirectX::XMLoadFloat4(&maskUV[i]);
			mQuad.vertices[i].color = DirectX::XMLoadFloat4(&col);
		}
		mQuads.push_back(mQuad);

		++numQuads;
		numVertices += 4;
		numIndices += 6;
    }

    void Renderer::resetRenderStats()
	{
		numIndices = 0;
		numVertices = 0;
		numQuads = 0;
	}

	void Renderer::bindTexture(int tex)
	{
		texID = tex;
		glBindTexture(GL_TEXTURE_2D, texID);
	}

	void Renderer::beginBatch()
	{	
		batchStarted = true;
		vBuffer.resetBufferPos();
		quads.clear();
		mvBuffer.resetBufferPos();
		mQuads.clear();
		resetRenderStats();
	}

	void Renderer::endBatch()
	{
		assert(batchStarted && "Render batch not started. Forgot to call beginBatch?");

		numBatchVertices = numVertices;
		numBatchQuads = numQuads;

		if (quads.size())
		{
			std::stable_sort(quads.begin(), quads.end(),
				[](const Quad<Vertex>& q1, const Quad<Vertex>& q2) {return q1.zIndex < q2.zIndex; });
			
			vBuffer.bind();
			bindTexture(quads[0].texture);
			int vertexCount = 0;
	
			for (const auto& q : quads)
			{
				if (texID != q.texture || vertexCount + 4 >= vBuffer.getCapacity())
				{
					vBuffer.uploadBuffer();
					vBuffer.flushBuffer();
					vBuffer.resetBufferPos();
					vertexCount = 0;
	
					bindTexture(q.texture);
				}
	
				vBuffer.pushBuffer(q);
				vertexCount += 4;
			}
	
			vBuffer.uploadBuffer();
			vBuffer.flushBuffer();
		}

		if (mQuads.size())
		{
			mvBuffer.bind();
			int quadPerBatch = mvBuffer.getCapacity() / 4;
			for (size_t batch = 0, nBatch = (mQuads.size() + quadPerBatch - 1) / quadPerBatch; batch < nBatch; batch++)
			{
				size_t start = batch * quadPerBatch, stop = std::min(start + quadPerBatch, mQuads.size());
				for (size_t i = start; i < stop; i++)
					mvBuffer.pushBuffer(mQuads[i]);
				mvBuffer.uploadBuffer();
				for (size_t i = start, vi = 0; i < stop; i++, vi += 4)
				{
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, mQuads[i].zIndex);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, mQuads[i].texture);
					mvBuffer.flushBuffer(vi, 4);
				}
			}
		}

		batchStarted = false;
	}

    void Renderer::endBatchWithBlending(int srcRGB, int dstRGB, int srcA, int dstA)
    {
		GLboolean blending = glIsEnabled(GL_BLEND); // should always be true but just in case
		if (!blending)
			glEnable(GL_BLEND);
		GLint oldSrcRGB, oldDstRGB, oldSrcAlpha, oldDstAlpha;
		glGetIntegerv(GL_BLEND_SRC_RGB, &oldSrcRGB);
		glGetIntegerv(GL_BLEND_DST_RGB, &oldDstRGB);
		glGetIntegerv(GL_BLEND_SRC_ALPHA, &oldSrcAlpha);
		glGetIntegerv(GL_BLEND_DST_ALPHA, &oldDstAlpha);
		glBlendFuncSeparate(srcRGB, dstRGB, srcA, dstA);
		//glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

		endBatch();

		glBlendFuncSeparate(oldSrcRGB, oldDstRGB, oldSrcAlpha, oldDstAlpha);
		//glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		if (!blending)
			glDisable(GL_BLEND);
    }
    
	void Renderer::endBatchWithDepthTest(int depthFunc)
    {
		GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
		GLint oldDepthFunc;
		if (!depthTest)
			glEnable(GL_DEPTH_TEST);
		glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
		glDepthFunc(depthFunc);

		endBatch();

		glDepthFunc(oldDepthFunc);
		if (!depthTest)
			glDisable(GL_DEPTH_TEST);
    }
}