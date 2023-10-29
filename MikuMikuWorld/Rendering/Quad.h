#pragma once
#include <DirectXMath.h>
#include <array>

namespace MikuMikuWorld
{
	struct Vertex
	{
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR color;
		DirectX::XMVECTOR uv;
	};

	struct Quad
	{
		int zIndex;
		int sprite;
		int texture;
		Vertex vertices[4];
		DirectX::XMMATRIX matrix;

		Quad() : zIndex{ 0 }, sprite{ 0 }, texture{ 0 } {}

		Quad(const std::array<Vertex, 4>& v, const DirectX::XMMATRIX& m, int tex, int z = 0)
		{
			sprite = 0;
			for (int i = 0; i < 4; ++i)
			{
				vertices[i].position = v[i].position;
				vertices[i].color = v[i].color;
				vertices[i].uv = v[i].uv;
			}

			matrix = m;
			texture = tex;
			zIndex = z;
		}
	};
}