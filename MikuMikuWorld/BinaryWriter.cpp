#include "BinaryWriter.h"
#include "IO.h"
#include "Platform.h"

namespace IO
{
	BinaryWriter::BinaryWriter(const std::string& filename)
	{
		stream = NULL;
		stream = Platform::OpenFile(filename, "wb");
	}

	BinaryWriter::~BinaryWriter()
	{
		close();
	}

	bool BinaryWriter::isStreamValid()
	{
		return stream;
	}

	void BinaryWriter::close()
	{
		if (stream)
			fclose(stream);

		stream = NULL;
	}

	void BinaryWriter::flush()
	{
		if (stream)
			fflush(stream);
	}

	size_t BinaryWriter::getFileSize()
	{
		size_t pos = ftell(stream);
		fseek(stream, 0, SEEK_END);

		size_t size = ftell(stream);
		fseek(stream, pos, SEEK_SET);

		return size;
	}

	size_t BinaryWriter::getStreamPosition()
	{
		return ftell(stream);
	}

	void BinaryWriter::seek(size_t pos)
	{
		if (stream)
			fseek(stream, pos, SEEK_SET);
	}

	void BinaryWriter::writeInt16(uint16_t data)
	{
		if (!Platform::Bit::IsLittleEndian())
			data = Platform::Bit::ByteSwap16(data);
		if (stream)
			fwrite(&data, sizeof(uint16_t), 1, stream);
	}

	void BinaryWriter::writeInt32(uint32_t data)
	{
		if (!Platform::Bit::IsLittleEndian())
			data = Platform::Bit::ByteSwap32(data);
		if (stream)
			fwrite(&data, sizeof(uint32_t), 1, stream);
	}

	void BinaryWriter::writeSingle(float data)
	{
		if (!Platform::Bit::IsLittleEndian())
			data = Platform::Bit::ByteSwapf32(data);
		if (stream)
			fwrite(&data, sizeof(float), 1, stream);
	}

	void BinaryWriter::writeNull(size_t length)
	{
		static const uint8_t zero[1024] = { 0 };
		if (!stream)
			return;
		while (sizeof(zero) < length)
		{
			fwrite(zero, sizeof(uint8_t), sizeof(zero), stream);
			length -= sizeof(zero);
		}
		fwrite(zero, sizeof(uint8_t), length, stream);
	}

	void BinaryWriter::writeString(std::string data)
	{
		if (stream)
		{
			size_t length = data.size();
			if (length)
			{
				fwrite(data.c_str(), sizeof(char), length, stream);
				writeNull(1);
			}
			else
			{
				writeNull(1);
			}
		}
	}
}