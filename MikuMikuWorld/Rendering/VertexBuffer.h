#pragma once
#include "Quad.h"

namespace MikuMikuWorld
{
	class VertexBuffer
	{
	private:
		Vertex* buffer;
		int* indices;
		int indexCapacity;
		int vertexCapcity;
		int bufferPos;

		unsigned int vao;
		unsigned int vbo;
		unsigned int ebo;

	public:
		VertexBuffer(int _capacity);
		~VertexBuffer();

		void setup();
		void dispose();
		void bind() const;
		void pushBuffer(const Quad& q);
		void resetBufferPos();
		void uploadBuffer();
		void flushBuffer();
		int getCapacity() const;
		int getSize() const;
	};
}