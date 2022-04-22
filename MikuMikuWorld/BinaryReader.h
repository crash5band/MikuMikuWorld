#pragma once
#include <stdio.h>
#include <string>

namespace MikuMikuWorld
{
	class BinaryReader
	{
	private:
		FILE* stream;

	public:
		BinaryReader(const std::string& filename);

		bool isStreamValid();
		void close();

		size_t getFileSize();
		size_t getStreamPosition();

		uint32_t readInt32();
		float readSingle();
		std::string readString();
	};
}