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
		std::string musicFilename;
		float musicOffset;
		Jacket jacket;

		EditorScoreData() :
			filename{ "" }, title{ "" }, designer{ "" }, artist{ "" }, musicFilename{ "" }, musicOffset{ 0.0f }
		{
			jacket.filename = "";
			jacket.texID = -1;
		}
	};
}