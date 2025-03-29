#pragma once

#include "Score.h"
#include <memory>
#include <stdexcept>

namespace MikuMikuWorld
{
	class ScoreSerializer
	{
	public:
		virtual void serialize(Score score, std::string filename) = 0;
		virtual Score deserialize(std::string filename) = 0;

		ScoreSerializer() {}
		virtual ~ScoreSerializer() {};
	};

	class ScoreSerializerFactory
	{
	public:
		static std::unique_ptr<ScoreSerializer> getSerializer(const std::string& fileExtension);
		static bool isNativeScoreFormat(const std::string& fileExtension);
	};

	class UnsupportedScoreFormatError : public std::runtime_error
	{
	public:
		UnsupportedScoreFormatError(const std::string& format) :
			std::runtime_error("Unsupported score format (" + format + ")") {}
	};
}