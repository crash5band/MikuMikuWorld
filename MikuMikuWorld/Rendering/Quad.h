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
		int texture;
		Vertex vertices[4];

		Quad() : zIndex{ 0 }, texture{ 0 }
		{
		}

		Quad(int texture, int zIndex) : zIndex{ zIndex }, texture{ texture }
		{

		}

		Quad(const std::array<Vertex, 4>& v, const DirectX::XMMATRIX& m, int tex, int z = 0)
		{
			vertices[0].position = v[0].position;
			vertices[0].color = v[0].color;
			vertices[0].uv = v[0].uv;

			vertices[1].position = v[1].position;
			vertices[1].color = v[1].color;
			vertices[1].uv = v[1].uv;

			vertices[2].position = v[2].position;
			vertices[2].color = v[2].color;
			vertices[2].uv = v[2].uv;

			vertices[3].position = v[3].position;
			vertices[3].color = v[3].color;
			vertices[3].uv = v[3].uv;

			texture = tex;
			zIndex = z;
		}
	};
}