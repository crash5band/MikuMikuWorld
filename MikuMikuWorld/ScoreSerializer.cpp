#include "ScoreSerializer.h"

#include "NativeScoreSerializer.h"
#include "SusSerializer.h"

namespace MikuMikuWorld
{
	std::unique_ptr<ScoreSerializer> ScoreSerializerFactory::getSerializer(const std::string& fileExtension)
	{
		std::string ext{fileExtension};
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		if (ext == MMWS_EXTENSION)
		{
			return std::make_unique<NativeScoreSerializer>();
		}
		else if (ext == SUS_EXTENSION)
		{
			return std::make_unique<SusSerializer>();
		}
		else
		{
			throw UnsupportedScoreFormatError(fileExtension);
		}
	}

	bool ScoreSerializerFactory::isNativeScoreFormat(const std::string& fileExtension)
	{
		std::string ext{ fileExtension };
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		return ext == MMWS_EXTENSION;
	}
}