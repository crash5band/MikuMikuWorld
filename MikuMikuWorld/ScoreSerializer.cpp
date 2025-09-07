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
		SCP_EXTENSION,
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

	Score& ScoreSerializeDialog::getScore()
	{
		return score;
	}

	const std::string& ScoreSerializeDialog::getScoreFilename() const
	{
		return scoreFilename;
	}

	const std::string& ScoreSerializeDialog::getErrorMessage() const
	{
		return errorMessage;
	}

	GenericScoreSerializeDialog::GenericScoreSerializeDialog(std::unique_ptr<ScoreSerializer> serializer, Score score, const std::string& filename)
		: isSerializing(true), serializer(std::move(serializer)), filename(filename)
	{
		this->score = std::move(score);

		if (ScoreSerializer::isNativeScoreFormat(IO::File::getFileExtension(filename)))
			this->scoreFilename = filename;
	}

	GenericScoreSerializeDialog::GenericScoreSerializeDialog(std::unique_ptr<ScoreSerializer> deserializer, const std::string& filename)
		: isSerializing(false), serializer(std::move(deserializer)), filename(filename)
	{

		if (ScoreSerializer::isNativeScoreFormat(IO::File::getFileExtension(filename)))
			this->scoreFilename = filename;
	}

	SerializeDialogResult MikuMikuWorld::GenericScoreSerializeDialog::update()
	{
		if (!serializer)
			return SerializeDialogResult::Error;
		if (isSerializing)
		{
			try
			{
				serializer->serialize(score, filename);
				serializer.reset();
				return SerializeDialogResult::SerializeSuccess;
			}
			catch (const std::exception& err)
			{
				errorMessage = IO::formatString(
					"%s\n"
					"%s: %s",
					getString("error_save_score_file"),
					getString("error"), err.what()
				);
				return SerializeDialogResult::Error;
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
				return SerializeDialogResult::DeserializeSuccess;
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
				return SerializeDialogResult::Error;
			}
		}
	}
}