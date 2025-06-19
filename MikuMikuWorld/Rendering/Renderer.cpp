#include "Renderer.h"
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <algorithm>

namespace MikuMikuWorld
{
	Renderer::Renderer() : vBuffer{ VertexBuffer(maxQuads) }
	{
		vBuffer.setup();
		vBuffer.bind();
		quads.reserve(maxQuads);
		init();
	}

	void Renderer::init()
	{
		// order: top-right, bottom-right, bottom-left, top-left
		vPos[0] = DirectX::XMVECTOR{ 0.5f, 0.5f, 0.0f, 1.0f };
		vPos[1] = DirectX::XMVECTOR{ 0.5f, -0.5f, 0.0f, 1.0f };
		vPos[2] = DirectX::XMVECTOR{ -0.5f, -0.5f, 0.0f, 1.0f };
		vPos[3] = DirectX::XMVECTOR{ -0.5f, 0.5f, 0.0f, 1.0f };
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

		vPos[0] = DirectX::XMVECTOR{ right, top, 0.0f, 1.0f };
		vPos[1] = DirectX::XMVECTOR{ right, bottom, 0.0f, 1.0f };
		vPos[2] = DirectX::XMVECTOR{ left, bottom, 0.0f, 1.0f };
		vPos[3] = DirectX::XMVECTOR{ left, top, 0.0f, 1.0f };
	}

	void Renderer::setUVCoords(const Texture& tex, float x1, float x2, float y1, float y2)
	{
		float left		= x1 / tex.getWidth();
		float right		= x2 / tex.getWidth();
		float top		= y1 / tex.getHeight();
		float bottom	= y2 / tex.getHeight();

		uvCoords[0] = DirectX::XMVECTOR{ right, top, 0.0f, 0.0f };
		uvCoords[1] = DirectX::XMVECTOR{ right, bottom, 0.0f, 0.0f };
		uvCoords[2] = DirectX::XMVECTOR{ left, bottom, 0.0f, 0.0f };
		uvCoords[3] = DirectX::XMVECTOR{ left, top, 0.0f, 0.0f };
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
		DirectX::XMVECTOR color{ tint.r, tint.g, tint.b, tint.a };
		setUVCoords(tex, x1, x2, y1, y2);
		setAnchor(anchor);

		pushQuad(vPos, uvCoords, model, color, tex.getID(), z);
	}

	void Renderer::drawQuad(const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector2& p4,
		const Texture& tex, float x1, float x2, float y1, float y2, const Color& tint, int z)
	{
		setUVCoords(tex, x1, x2, y1, y2);
		vPos[0] = DirectX::XMVECTOR{ p4.x, p4.y, 0.0f, 1.0f };
		vPos[1] = DirectX::XMVECTOR{ p2.x, p2.y, 0.0f, 1.0f };
		vPos[2] = DirectX::XMVECTOR{ p1.x, p1.y, 0.0f, 1.0f };
		vPos[3] = DirectX::XMVECTOR{ p3.x, p3.y, 0.0f, 1.0f };
		DirectX::XMVECTOR color{ tint.r, tint.g, tint.b, tint.a };

		pushQuad(vPos, uvCoords, DirectX::XMMatrixIdentity(), color, tex.getID(), z);
	}

	void Renderer::drawRectangle(Vector2 position, Vector2 size, const Texture& tex, float x1, float x2, float y1, float y2, Color tint, int z)
	{
		Vector2 p1{ position.x, position.y };
		Vector2 p2{ position.x + size.x, position.y };
		Vector2 p3{ position.x + size.x, position.y + size.y };
		Vector2 p4{ position.x, position.y + size.y };

		drawQuad(p4, p3, p1, p2, tex, x1, x2, y1, y2, tint, z);
	}

	void Renderer::pushQuad(const std::array<DirectX::XMVECTOR, 4>& pos, const std::array<DirectX::XMVECTOR, 4>& uv,
		const DirectX::XMMATRIX& m, const DirectX::XMVECTOR& col, int tex, int z)
	{
		Quad q{ tex, z };
		q.vertices[0].position = DirectX::XMVector2Transform(pos[0], m);
		q.vertices[0].color = col;
		q.vertices[0].uv = uvCoords[0];

		q.vertices[1].position = DirectX::XMVector2Transform(pos[1], m);
		q.vertices[1].color = col;
		q.vertices[1].uv = uvCoords[1];

		q.vertices[2].position = DirectX::XMVector2Transform(pos[2], m);
		q.vertices[2].color = col;
		q.vertices[2].uv = uvCoords[2];

		q.vertices[3].position = DirectX::XMVector2Transform(pos[3], m);
		q.vertices[3].color = col;
		q.vertices[3].uv = uvCoords[3];

		quads.push_back(std::move(q));

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
		resetRenderStats();
	}

	void Renderer::endBatch()
	{
		assert(batchStarted && "Render batch not started. Forgot to call beginBatch?");

		numBatchVertices = numVertices;
		numBatchQuads = numQuads;

		if (!quads.size())
			return;

		std::stable_sort(quads.begin(), quads.end(),
			[](const Quad& q1, const Quad& q2) {return q1.zIndex < q2.zIndex; });
		
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

		batchStarted = false;
	}
}