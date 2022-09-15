#pragma once
#include "Quad.h"
#include "../Math.h"
#include "Texture.h"
#include "AnchorType.h"
#include "VertexBuffer.h"
#include <vector>
#include <array>

namespace MikuMikuWorld
{
	constexpr size_t maxQuads = 1500;

	class Renderer
	{
	private:
		size_t numVertices;
		size_t numBatchVertices;
		size_t numIndices;
		size_t numQuads;
		size_t numBatchQuads;

		VertexBuffer vBuffer;
		std::vector<Quad> quads;
		std::array<DirectX::XMVECTOR, 4> vPos;
		std::array<DirectX::XMVECTOR, 4> uvCoords;

		unsigned int vao, vbo, ebo;
		int texID;
		bool batchStarted;

		void init();
		void resetRenderStats();

	public:
		Renderer();

		void drawSprite(const Vector2& pos, float rot, const Vector2& sz, AnchorType anchor, const Texture& tex, int spr, const Color& tint, int z = 0);
		void drawSprite(const Vector2& pos, float rot, const Vector2& sz, AnchorType anchor, const Texture& tex,
			float x1, float x2, float y1, float y2, const Color& tint = {1.0f, 1.0f, 1.0f, 1.0f}, int z = 0);
		
		void drawQuad(const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector2& p4, const Texture& tex, float x1, float x2, float y1, float y2,
			const Color& tint = { 1.0f, 1.0f, 1.0f, 1.0f }, int z = 0);

		void drawRectangle(Vector2 position, Vector2 size, const Texture& tex, float x1, float x2, float y1, float y2, Color tint, int z);

		void setUVCoords(const Texture& tex, float x1, float x2, float y1, float y2);
		void setAnchor(AnchorType type);
		DirectX::XMMATRIX getModelMatrix(const Vector2& pos, const float rot, const Vector2& sz);

		void pushQuad(const std::array<DirectX::XMVECTOR, 4>& pos, const std::array<DirectX::XMVECTOR, 4>& uv,
			const DirectX::XMMATRIX& m, const DirectX::XMVECTOR& col, int tex, int z);

		void bindTexture(int tex);
		void beginBatch();
		void endBatch();
		void flush();

		inline int getNumVertices() const { return numBatchVertices; }
		inline int getNumQuads() const { return numBatchQuads; }
	};
}
