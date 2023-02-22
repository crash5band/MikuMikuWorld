#pragma once
#include <string>
#include <map>
#include <vector>
#include "ChannelProvider.h"

namespace MikuMikuWorld
{
	struct SUS;

	struct NoteMap
	{
		struct RawData
		{
			int tick;
			std::string data;
		};

		std::vector<RawData> data;
		int ticksPerMeasure;
	};

	struct MeasureMap
	{
		int measure;
		std::map<std::string, NoteMap> notesMap;
	};

	struct BarLengthTicks
	{
		BarLength barLength;
		int ticks;
	};

	class SusExporter
	{
	private:
		int ticksPerBeat;
		std::map<int, MeasureMap> measuresMap;
		std::vector<BarLengthTicks> barLengthTicks;

		ChannelProvider channelProvider;

		int getMeasureFromTicks(int ticks);

	public:
		SusExporter();

		void appendData(int tick, std::string info, std::string data);
		void appendNoteData(const SUSNote& note, const std::string infoPrefix, const std::string channel);
		void dump(const SUS& sus, const std::string& filename, std::string comment = "");
	};
}
