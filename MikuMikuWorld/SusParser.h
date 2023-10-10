#pragma once
#include <string>
#include <regex>
#include "SUS.h"

namespace MikuMikuWorld
{
	struct SusLineData
	{
		int lineIndex;
		int measureOffset;
		std::string line;
	};

	struct SusLineArgs
	{
		std::string header;
		std::string value;
	};

	class SusParser
	{
	private:
		int ticksPerBeat;
		int measureOffset;
    int laneOffset;
    bool sideLane;
		float waveOffset;
		std::string title;
		std::string artist;
		std::string designer;
		std::unordered_map<std::string, float> bpmDefinitions;
		std::vector<Bar> bars;

		bool isCommand(const std::string& line);
		int toTicks(int measure, int i, int total);
		SUSNoteStream toSlides(const std::vector<SUSNote>& stream);
		std::vector<SUSNote> toNotes(const std::string& header, const std::string& data, int measureBase);

	public:
		SusParser();

		SUS parse(const std::string& filename);
		void processCommand(std::string& line);
	};
}
