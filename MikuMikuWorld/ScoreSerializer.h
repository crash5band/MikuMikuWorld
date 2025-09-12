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
		static bool isNativeScoreFormat(const std::string& fileExtension);
	};

	enum class SerializeResult
	{
		None,
		Cancel,
		SerializeSuccess,
		DeserializeSuccess,
		Error,
	};

	class ScoreSerializeController
	{
	protected:
		ScoreSerializeController() = default;
	public:
		Score& getScore();
		const std::string& getFilename() const;
		const std::string& getScoreFilename() const;
		const std::string& getErrorMessage() const;

		virtual SerializeResult update() = 0;
		virtual ~ScoreSerializeController() {};
	protected:
		std::string errorMessage;
		std::string filename;
		std::string scoreFilename;
		Score score;
	};

	class DefaultScoreSerializeController : public ScoreSerializeController
	{
	public:
		DefaultScoreSerializeController(std::unique_ptr<ScoreSerializer> serializer, Score score, const std::string& filename);
		DefaultScoreSerializeController(std::unique_ptr<ScoreSerializer> deserializer, const std::string& filename);
	
		SerializeResult update() override;
	private:
		bool isSerializing;
		std::unique_ptr<ScoreSerializer> serializer;
	};
}