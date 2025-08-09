#pragma once
#include <stdio.h>
#include <string>

namespace IO
{
	class BinaryReader
	{
	private:
		FILE* stream;

	public:
		BinaryReader(const std::string& filename);
		~BinaryReader();

		bool isStreamValid();
		void close();

		size_t getFileSize();
		size_t getStreamPosition();
		void seek(size_t pos);

		uint16_t readInt16();
		uint32_t readInt32();
		float readSingle();
		std::string readString();
	};
}