#include "VertexBuffer.h"
#include "glad/glad.h"

namespace MikuMikuWorld
{
	template<typename VertexType>
	VertexBuffer<VertexType>::VertexBuffer(int _capacity) :
		vertexCapcity{ _capacity }, bufferPos{ 0 }, vao{ 0 }, vbo{ 0 }, ebo{ 0 }
	{
		buffer = nullptr;
		indices = nullptr;
		indexCapacity = (vertexCapcity * 6) / 4;
	}

	template<typename VertexType>
	VertexBuffer<VertexType>::~VertexBuffer()
	{
		dispose();
	}

	template<typename VertexType>
	void VertexBuffer<VertexType>::setup()
	{
		buffer = new VertexType[vertexCapcity];
		indices = new int[indexCapacity];

		size_t offset = 0;
		for (size_t index = 0; index < indexCapacity; index += 6)
		{
			indices[index + 0] = offset + 0;
			indices[index + 1] = offset + 1;
			indices[index + 2] = offset + 2;

			indices[index + 3] = offset + 2;
			indices[index + 4] = offset + 3;
			indices[index + 5] = offset + 0;

			offset += 4;
		}

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertexCapcity * sizeof(VertexType), NULL, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCapacity * sizeof(unsigned int), indices, GL_STATIC_DRAW);

		VertexLayout<VertexType>::setup();

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	template<typename VertexType>
	void VertexBuffer<VertexType>::dispose()
	{
		delete[] buffer;
		delete[] indices;

		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ebo);
	}

	template<typename VertexType>
	void VertexBuffer<VertexType>::bind() const
	{
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
	}

	template<typename VertexType>	
	int VertexBuffer<VertexType>::getSize() const
	{
		return bufferPos * sizeof(VertexType);
	}

	template<typename VertexType>	
	int VertexBuffer<VertexType>::getCapacity() const
	{
		return vertexCapcity;
	}

	template<typename VertexType>	
	void VertexBuffer<VertexType>::pushBuffer(const Quad<VertexType>& q)
	{
		std::copy(q.vertices, q.vertices + 4, buffer + bufferPos);
		bufferPos += 4;
	}

	template<typename VertexType>
	void VertexBuffer<VertexType>::resetBufferPos()
	{
		bufferPos = 0;
	}

	template<typename VertexType>
	void VertexBuffer<VertexType>::uploadBuffer()
	{
		size_t size = getSize();
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, buffer);
	}

	template<typename VertexType>
	void VertexBuffer<VertexType>::flushBuffer()
	{
		size_t numIndices = (bufferPos / 4) * 6;
		glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
	}

    template <typename VertexType>
    inline void VertexBuffer<VertexType>::flushBuffer(int index, int size)
    {
		size_t startIndexOffset = (index) / 4 * 6 * sizeof(decltype(*indices));
		size_t numIndices = (size >= 0 ? size : bufferPos - index) / 4 * 6;
		glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, (void*)startIndexOffset);
    }
}