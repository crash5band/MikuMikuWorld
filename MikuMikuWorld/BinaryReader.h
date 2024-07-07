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

		int16_t readInt16();
		int32_t readInt32();
		uint16_t readUInt16();
		uint32_t readUInt32();
		float readSingle();
		std::string readString();
	};
}
