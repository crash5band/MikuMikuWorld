#include "ScoreSerializer.h"
#include "Localization.h"
#include "Constants.h"
#include "IO.h"
#include "File.h"
#include <array>
#include <algorithm>

namespace MikuMikuWorld
{
	SerializeFormat ScoreSerializeController::toSerializeFormat(const std::string_view& filename)
	{
		const auto hasExtension = IO::endsWith;
		if (hasExtension(filename, MMWS_EXTENSION))
		{
			return SerializeFormat::NativeFormat;
		}
		else if (hasExtension(filename, SUS_EXTENSION))
		{
			return SerializeFormat::SusFormat;
		}
		else if (hasExtension(filename, JSON_EXTENSION) ||
		         hasExtension(filename, GZ_JSON_EXTENSION))
		{
			return SerializeFormat::LvlDataFormat;
		}
		return SerializeFormat::FormatCount;
	}

	
	bool ScoreSerializeController::isValidFormat(SerializeFormat format)
	{
		return static_cast<int>(format) >= 0 &&
		       static_cast<int>(format) < static_cast<int>(SerializeFormat::FormatCount);
	}

	IO::FileDialogFilter ScoreSerializeController::getFormatFilter(SerializeFormat format)
	{
		switch (format)
		{
		case SerializeFormat::NativeFormat:
			return IO::mmwsFilter;
		case SerializeFormat::SusFormat:
			return IO::susFilter;
		case SerializeFormat::LvlDataFormat:
			return IO::lvlDatFilter;
		default:
			return IO::allFilter;
		}
	}

	std::string ScoreSerializeController::getFormatDefaultExtension(SerializeFormat format)
	{
		switch (format)
		{
		case SerializeFormat::NativeFormat:
			return "unchmmws";
		case SerializeFormat::SusFormat:
			return "sus";
		case SerializeFormat::LvlDataFormat:
			return "json.gz";
		default:
			return "";
		}
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
}