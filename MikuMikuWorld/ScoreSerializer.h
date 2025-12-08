#pragma once

#include "Score.h"
#include "File.h"
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
	};

	enum class SerializeFormat
	{
		NativeFormat,
		SusFormat,
		LvlDataFormat,
		FormatCount
	};

	enum class SerializeResult
	{
		None,
		Cancel,
		SerializeSuccess,
		DeserializeSuccess,
		Error,
		PartialSerializeSuccess,
		PartialDeserializeSuccess,
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

		static SerializeFormat toSerializeFormat(const std::string_view& filename);
		static bool isValidFormat(SerializeFormat format);
		static IO::FileDialogFilter getFormatFilter(SerializeFormat format);
		static std::string getFormatDefaultExtension(SerializeFormat format);
	protected:
		std::string errorMessage;
		std::string filename;
		std::string scoreFilename;
		Score score;
	};

	class PartialScoreSerializeError : public std::runtime_error
	{
	public:
	using std::runtime_error::runtime_error;
	};

	class PartialScoreDeserializeError : public std::runtime_error
	{
	public:
		PartialScoreDeserializeError(Score score, const std::string& message) : partialScore(std::move(score)), std::runtime_error(message) { }

		inline const Score& getScore() const { return partialScore; }
	private:
		Score partialScore;
	};
}