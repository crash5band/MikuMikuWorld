#include "ScoreSerializer.h"

#include "NativeScoreSerializer.h"
#include "SusSerializer.h"
#include "SonolusSerializer.h"

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
		else if (ext == SCP_EXTENSION)
		{
			bool gzip = true;
			bool prettyDump = false;
#if _DEBUG
			prettyDump = true;
#endif
			return std::make_unique<SonolusSerializer>(prettyDump, gzip);
		}
		else
		{
			return nullptr;
		}
	}

	bool ScoreSerializerFactory::isNativeScoreFormat(const std::string& fileExtension)
	{
		std::string ext{ fileExtension };
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		return ext == MMWS_EXTENSION;
	}
}