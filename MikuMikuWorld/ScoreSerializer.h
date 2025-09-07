#pragma once

#include "Score.h"
#include <memory>
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

	enum class SerializeDialogResult
	{
		None,
		Cancel,
		SerializeSuccess,
		DeserializeSuccess,
		Error,
	};

	class ScoreSerializeDialog
	{
	protected:
		ScoreSerializeDialog() = default;
	public:
		Score& getScore();
		const std::string& getScoreFilename() const;
		const std::string& getErrorMessage() const;

		virtual SerializeDialogResult update() = 0;
		virtual ~ScoreSerializeDialog() {};
	protected:
		std::string errorMessage;
		std::string scoreFilename;
		Score score;
	};

	class GenericScoreSerializeDialog : public ScoreSerializeDialog
	{
	public:
		GenericScoreSerializeDialog(std::unique_ptr<ScoreSerializer> serializer, Score score, const std::string& filename);
		GenericScoreSerializeDialog(std::unique_ptr<ScoreSerializer> deserializer, const std::string& filename);
	
		SerializeDialogResult update() override;
	private:
		bool isSerializing;
		std::unique_ptr<ScoreSerializer> serializer;
		std::string filename;
	};
}