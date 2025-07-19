#pragma once
#include <GLES3/gl3.h>
#include <cstddef>
#include <array>
#include <DirectXMath.h>

namespace MikuMikuWorld
{
	struct Vertex
	{
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR color;
		DirectX::XMVECTOR uv;
	};

	struct MaskVertex
	{
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR color;
		DirectX::XMVECTOR uvBase;
		DirectX::XMVECTOR uvMask;
	};

	template<typename T>
	struct VertexLayout;

	template<>
	struct VertexLayout<Vertex> {
		static inline void setup()
		{
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
		}
	};

	template<>
	struct VertexLayout<MaskVertex> {
		static inline void setup()
		{
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MaskVertex), (void*)offsetof(MaskVertex, position));

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(MaskVertex), (void*)offsetof(MaskVertex, color));

			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MaskVertex), (void*)offsetof(MaskVertex, uvBase));

			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(MaskVertex), (void*)offsetof(MaskVertex, uvMask));
		}
	};

	template<typename VertexType>
	struct Quad
	{
		int zIndex;
		int texture;
		VertexType vertices[4];

		Quad() : zIndex{ 0 }, texture{ 0 }
		{
		}

		Quad(int texture, int zIndex) : zIndex{ zIndex }, texture{ texture }
		{

		}

		Quad(const std::array<VertexType, 4>& v, const DirectX::XMMATRIX& m, int tex, int z = 0)
		{
			std::copy(v.begin(), v.end(), vertices);
			texture = tex;
			zIndex = z;
		}
	};
}