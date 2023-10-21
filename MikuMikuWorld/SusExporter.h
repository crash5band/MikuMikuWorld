#pragma once
#include <string>
#include <map>
#include <vector>
#include <unordered_map>

namespace MikuMikuWorld
{
	class ChannelProvider
	{
	private:
		struct TickRange { int start; int end; };
		std::unordered_map<int, TickRange> channels;

	public:
		ChannelProvider()
		{
			clear();
		}

		int generateChannel(int startTick, int endTick);
		void clear();
	};

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

		int getMeasureFromTicks(int ticks);
		int getTicksFromMeasure(int measure);
		void appendSlideData(const SUSNoteStream& slides, const std::string& infoPrefix);
		std::vector<std::string> getNoteLines(int baseMeasure);

	public:
		SusExporter();

		void appendData(int tick, std::string info, std::string data);
		void appendNoteData(const SUSNote& note, const std::string infoPrefix, const std::string channel);
		void dump(const SUS& sus, const std::string& filename, std::string comment = "");
	};
}
