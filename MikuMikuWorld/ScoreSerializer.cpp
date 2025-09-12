#include "ScoreSerializer.h"
#include "Localization.h"
#include "Constants.h"
#include "IO.h"
#include "File.h"
#include <array>
#include <algorithm>

namespace MikuMikuWorld
{
	constexpr std::array<std::string_view, 4> supportedScoreExtensions =
	{
		MMWS_EXTENSION,
		SUS_EXTENSION,
		LVLDAT_EXTENSION
	};

	bool ScoreSerializer::isSupportedFileFormat(const std::string_view& extension)
	{
		return std::find(supportedScoreExtensions.cbegin(), supportedScoreExtensions.cend(), extension) != supportedScoreExtensions.end();
	}

	bool ScoreSerializer::isNativeScoreFormat(const std::string& fileExtension)
	{
		std::string ext = fileExtension;
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		return ext == MMWS_EXTENSION;
	}

	Score& ScoreSerializeController::getScore()
	{
		return score;
	}

	const std::string& ScoreSerializeController::getFilename() const
	{
		return filename;
	}

	const std::string& ScoreSerializeController::getScoreFilename() const
	{
		return scoreFilename;
	}

	const std::string& ScoreSerializeController::getErrorMessage() const
	{
		return errorMessage;
	}

	DefaultScoreSerializeController::DefaultScoreSerializeController(std::unique_ptr<ScoreSerializer> serializer, Score score, const std::string& filename)
		: isSerializing(true), serializer(std::move(serializer))
	{
		this->score = std::move(score);
		this->filename = filename;

		if (ScoreSerializer::isNativeScoreFormat(IO::File::getFileExtension(filename)))
			this->scoreFilename = filename;
	}

	DefaultScoreSerializeController::DefaultScoreSerializeController(std::unique_ptr<ScoreSerializer> deserializer, const std::string& filename)
		: isSerializing(false), serializer(std::move(deserializer))
	{
		this->filename = filename;

		if (ScoreSerializer::isNativeScoreFormat(IO::File::getFileExtension(filename)))
			this->scoreFilename = filename;
	}

	SerializeResult MikuMikuWorld::DefaultScoreSerializeController::update()
	{
		if (!serializer)
			return SerializeResult::Error;
		if (isSerializing)
		{
			try
			{
				serializer->serialize(score, filename);
				serializer.reset();
				return SerializeResult::SerializeSuccess;
			}
			catch (const std::exception& err)
			{
				errorMessage = IO::formatString(
					"%s\n"
					"%s: %s",
					getString("error_save_score_file"),
					getString("error"), err.what()
				);
				return SerializeResult::Error;
			}
		}
		else
		{
			// Backup next note ID in case of an import failure
			int nextIdBackup = nextID;
			try
			{
				resetNextID();
				score = serializer->deserialize(filename);
				return SerializeResult::DeserializeSuccess;
			}
			catch (std::exception& error)
			{
				nextID = nextIdBackup;
				errorMessage = IO::formatString(
					"%s\n"
					"%s: %s\n"
					"%s: %s",
					getString("error_load_score_file"),
					getString("score_file"), filename.c_str(),
					getString("error"), error.what()
				);
				return SerializeResult::Error;
			}
		}
	}
}