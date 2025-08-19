#pragma once

#include "ScoreContext.h"
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
	};

	class ScoreSerializerFactory
	{
	public:
		static std::unique_ptr<ScoreSerializer> getSerializer(const std::string& fileExtension);
		static bool isNativeScoreFormat(const std::string& fileExtension);
	};

	enum class SerializationDialogResult
	{
		None,
		Cancel,
		SerializeSuccess,
		DeserializeSuccess,
		Error,
	};

	class ScoreSerializationDialog
	{
	protected:
		bool open = false;
		bool isSerializing;
		bool isNativeFormat;
		std::unique_ptr<ScoreSerializer> serializer;
		std::string filename;
		std::string errorMessage;
		Score score;
	public:
		Score& getScore();
		const std::string& getFilename() const;
		const std::string& getErrorMessage() const;

		virtual void openSerializingDialog(const ScoreContext& context, const std::string& filename);
		virtual void openDeserializingDialog(const std::string& filename);
		virtual SerializationDialogResult update();
	};

	class SerializationDialogFactory
	{
	public:
		static std::unique_ptr<ScoreSerializationDialog> getDialog(const std::string& filename);
	};
}