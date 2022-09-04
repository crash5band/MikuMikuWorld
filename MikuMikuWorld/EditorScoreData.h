#pragma once
#include "Jacket.h"
#include <string>

namespace MikuMikuWorld
{
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
			jacket.clear();
		}
	};
}