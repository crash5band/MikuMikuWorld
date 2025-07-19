#pragma once
#include "Quad.h"

namespace MikuMikuWorld
{
	template<typename VertexType>
	class VertexBuffer
	{
	private:
		VertexType* buffer;
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
		void pushBuffer(const Quad<VertexType>& q);
		void resetBufferPos();
		void uploadBuffer();
		void flushBuffer();
		void flushBuffer(int bufPos, int bufSize);
		int getCapacity() const;
		int getSize() const;
	};
}

#include "VertexBuffer.inl"