#pragma once
#include <string>

namespace MikuMikuWorld
{
	struct EditorScoreData
	{
		std::string title;
		std::string designer;
		std::string artist;
		std::string filename;

		EditorScoreData() : filename{ "" }, title { "" }, designer{ "" }, artist{ "" }
		{

		}
	};
}