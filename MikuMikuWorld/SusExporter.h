#pragma once
#include <string>
#include <map>
#include <vector>
#include "IO.h"
#include "SUS.h"

namespace MikuMikuWorld
{
	// SUS can only handle up to 36^2 unique BPMs
	constexpr size_t MAX_BPM_IDENTIFIERS = (36ll * 36ll) - 1;

	class ChannelProvider
	{
	public:
		struct TickRange { int start; int end; };
		ChannelProvider()
		{
			clear();
		}

		int generateChannel(int startTick, int endTick);
		void clear();

	private:
		std::map<int, TickRange> channels;
	};

	class SusExportError : public std::runtime_error
	{
	public:
		SusExportError(const char* message) : std::runtime_error(message) {}
		
		virtual ~SusExportError() = default;
		virtual std::string_view getDetailedMessage() const = 0;
	};

	class TooManySlideChannelsError : SusExportError
	{
	public:
		std::string_view getDetailedMessage() const { return detailedMessage; }

		TooManySlideChannelsError(const ChannelProvider::TickRange& tickRange)
			: tickRange{ tickRange }, SusExportError("No more slide channels available")
		{
			detailedMessage = IO::formatString("Too many overlapping slides/guides. Exporter ran out of channels at range (%d - %d)",
				this->tickRange.start, this->tickRange.end);
		}

	private:
		ChannelProvider::TickRange tickRange;
		std::string detailedMessage;
	};

	class TooManyBpmIdentifiersError : public SusExportError
	{
	public:
		TooManyBpmIdentifiersError(size_t count) : count{ count }, SusExportError("Too many BPM changes.") {}

		std::string_view getDetailedMessage() const
		{
			return IO::formatString("Number of unique identifiers (%d) exceeds maximum supported (%d)", count, MAX_BPM_IDENTIFIERS);
		}

	private:
		size_t count{};
	};

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
