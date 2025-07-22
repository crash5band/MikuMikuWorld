#pragma once
#include <stdio.h>
#include <string>

namespace IO
{
	class BinaryWriter
	{
	private:
		FILE* stream;

	public:
		BinaryWriter(const std::string& filename);
		~BinaryWriter();

		bool isStreamValid();
		void close();
		void flush();

		size_t getFileSize();
		size_t getStreamPosition();

		void seek(size_t pos);
		void writeInt16(uint16_t data);
		void writeInt32(uint32_t data);
		void writeSingle(float data);
		void writeString(std::string data);
		void writeNull(size_t length);
	};
}