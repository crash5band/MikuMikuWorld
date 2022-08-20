#pragma once
#include <string>

namespace MikuMikuWorld
{
	struct Jacket
	{
		std::string filename;
		int texID;
	};

	struct EditorScoreData
	{
		std::string title;
		std::string designer;
		std::string artist;
		std::string filename;
		Jacket jacket;

		EditorScoreData() : filename{ "" }, title { "" }, designer{ "" }, artist{ "" }
		{
			jacket.filename = "";
			jacket.texID = -1;
		}
	};
}