#pragma once
#include <string>

namespace MikuMikuWorld
{
	struct EditorScoreData
	{
		std::string title;
		std::string designer;
		std::string artist;
		int difficulty;

		EditorScoreData() : title{ "" }, designer{ "" }, artist{ "" }, difficulty{ 1 }
		{

		}
	};
}