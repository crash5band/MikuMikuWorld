#include "VertexBuffer.h"
#include "GLES3/gl3.h"
#include <cstddef>

namespace MikuMikuWorld
{
	VertexBuffer::VertexBuffer(int _capacity) :
		vertexCapcity{ _capacity }, bufferPos{ 0 }, vao{ 0 }, vbo{ 0 }, ebo{ 0 }
	{
		buffer = nullptr;
		indices = nullptr;
		indexCapacity = (vertexCapcity * 6) / 4;
	}

	VertexBuffer::~VertexBuffer()
	{
		dispose();
	}

	void VertexBuffer::setup()
	{
		buffer = new Vertex[vertexCapcity];
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
		glBufferData(GL_ARRAY_BUFFER, vertexCapcity * sizeof(Vertex), NULL, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCapacity * sizeof(unsigned int), indices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void VertexBuffer::dispose()
	{
		delete[] buffer;
		delete[] indices;

		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ebo);
	}

	void VertexBuffer::bind() const
	{
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
	}

	int VertexBuffer::getSize() const
	{
		return bufferPos * sizeof(Vertex);
	}

	int VertexBuffer::getCapacity() const
	{
		return vertexCapcity;
	}

	void VertexBuffer::pushBuffer(const Quad& q)
	{
		buffer[bufferPos + 0].position = q.vertices[0].position;
		buffer[bufferPos + 0].color = q.vertices[0].color;
		buffer[bufferPos + 0].uv = q.vertices[0].uv;

		buffer[bufferPos + 1].position = q.vertices[1].position;
		buffer[bufferPos + 1].color = q.vertices[1].color;
		buffer[bufferPos + 1].uv = q.vertices[1].uv;

		buffer[bufferPos + 2].position = q.vertices[2].position;
		buffer[bufferPos + 2].color = q.vertices[2].color;
		buffer[bufferPos + 2].uv = q.vertices[2].uv;

		buffer[bufferPos + 3].position = q.vertices[3].position;
		buffer[bufferPos + 3].color = q.vertices[3].color;
		buffer[bufferPos + 3].uv = q.vertices[3].uv;

		bufferPos += 4;
	}

	void VertexBuffer::resetBufferPos()
	{
		bufferPos = 0;
	}

	void VertexBuffer::uploadBuffer()
	{
		size_t size = getSize();
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, buffer);
	}

	void VertexBuffer::flushBuffer()
	{
		size_t numIndices = (bufferPos / 4) * 6;
		glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
	}
}