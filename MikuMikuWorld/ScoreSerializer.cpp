#include "ScoreSerializer.h"

#include "NativeScoreSerializer.h"
#include "SusSerializer.h"
#include "SonolusSerializer.h"
#include "Localization.h"

namespace MikuMikuWorld
{
	constexpr std::array<std::string_view, 3> supportedScoreExtensions =
	{
		MMWS_EXTENSION,
		SUS_EXTENSION,
		SCP_EXTENSION
	};

	bool ScoreSerializer::isSupportedFileFormat(const std::string_view& extension)
	{
		return std::find(supportedScoreExtensions.cbegin(), supportedScoreExtensions.cend(), extension) != supportedScoreExtensions.end();
	}

	std::unique_ptr<ScoreSerializer> ScoreSerializerFactory::getSerializer(const std::string& fileExtension)
	{
		std::string ext;
		ext.reserve(fileExtension.size());
		std::transform(fileExtension.begin(), fileExtension.end(), std::back_inserter(ext), ::tolower);

		if (ext == MMWS_EXTENSION)
		{
			return std::make_unique<NativeScoreSerializer>();
		}
		else if (ext == SUS_EXTENSION)
		{
			return std::make_unique<SusSerializer>();
		}
		else if (ext == SCP_EXTENSION)
		{
			return std::make_unique<SonolusSerializer>();
		}
		else
		{
			return nullptr;
		}
	}

	bool ScoreSerializerFactory::isNativeScoreFormat(const std::string& fileExtension)
	{
		std::string ext;
		ext.reserve(fileExtension.size());
		std::transform(fileExtension.begin(), fileExtension.end(), std::back_inserter(ext), ::tolower);

		return ext == MMWS_EXTENSION;
	}

	Score& ScoreSerializationDialog::getScore()
	{
		return score;
	}

	const std::string& ScoreSerializationDialog::getFilename() const
	{
		static const std::string noName;
		if (isNativeFormat)
			return filename;
		else
			return noName;
	}

	const std::string& ScoreSerializationDialog::getErrorMessage() const
	{
		return errorMessage;
	}

	void ScoreSerializationDialog::openSerializingDialog(const ScoreContext& context, const std::string& filename)
	{
		std::string extension = IO::File::getFileExtension(filename);
		isSerializing = true;
		open = true;
		serializer = ScoreSerializerFactory::getSerializer(extension);
		if (!serializer)
			this->errorMessage = "Unsupported score format (" + extension + ")";
		this->filename = filename;
		score = context.score;
		score.metadata = context.workingData.toScoreMetadata();
		isNativeFormat = ScoreSerializerFactory::isNativeScoreFormat(extension);
	}

	void ScoreSerializationDialog::openDeserializingDialog(const std::string& filename)
	{
		std::string extension = IO::File::getFileExtension(filename);
		isSerializing = false;
		open = true;
		serializer = ScoreSerializerFactory::getSerializer(extension);
		if (!serializer)
			this->errorMessage = "Unsupported score format (" + extension + ")";
		this->filename = filename;
		isNativeFormat = ScoreSerializerFactory::isNativeScoreFormat(extension);
	}

	SerializationDialogResult ScoreSerializationDialog::update()
	{
		if (!serializer)
			return SerializationDialogResult::Error;
		if (isSerializing)
		{
			try
			{
				serializer->serialize(score, filename);
				return SerializationDialogResult::SerializeSuccess;
			}
			catch (const std::exception& err)
			{
				errorMessage = IO::formatString("An error occured while saving the score file\n%s", err.what());
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
				return SerializationDialogResult::DeserializeSuccess;
			}
			catch (std::exception& error)
			{
				nextID = nextIdBackup;
				errorMessage = IO::formatString("%s\n%s: %s\n%s: %s",
					getString("error_load_score_file"),
					getString("score_file"),
					filename.c_str(),
					getString("error"),
					error.what()
				);
			}
		}
		return SerializationDialogResult::Error;
	}

	std::unique_ptr<ScoreSerializationDialog> SerializationDialogFactory::getDialog(const std::string& filename)
	{
		std::string extension = IO::File::getFileExtension(filename);
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
		if (extension == SCP_EXTENSION)
			return std::make_unique<SonolusSerializationDialog>();
		return std::make_unique<ScoreSerializationDialog>();
	}
}