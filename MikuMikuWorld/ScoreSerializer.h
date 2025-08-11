#pragma once

#include "Score.h"
#include <memory>
#include <stdexcept>
#include <string>

namespace MikuMikuWorld
{
	class ScoreSerializer
	{
	public:
		virtual void serialize(const Score& score, std::string filename) = 0;
		virtual Score deserialize(std::string filename) = 0;

		ScoreSerializer() {}
		virtual ~ScoreSerializer() {};

		static bool isSupportedFileFormat(const std::string_view& extension);
	};

	class ScoreSerializerFactory
	{
	public:
		static std::unique_ptr<ScoreSerializer> getSerializer(const std::string& fileExtension);
		static bool isNativeScoreFormat(const std::string& fileExtension);
	};
}