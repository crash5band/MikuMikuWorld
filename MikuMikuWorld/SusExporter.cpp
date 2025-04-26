#include "SusExporter.h"
#include "IO.h"
#include "File.h"
#include <algorithm>
#include <numeric>

using namespace IO;

namespace MikuMikuWorld
{
	int ChannelProvider::generateChannel(int startTick, int endTick)
	{
		for (auto& it : channels)
		{
			int start = it.second.start;
			int end = it.second.end;
			if ((start == 0 && end == 0) || endTick < start || startTick > end)
			{
				channels[it.first] = TickRange{ startTick, endTick };
				return it.first;
			}
		}

		throw("No more channels available");
	}

	void ChannelProvider::clear()
	{
		for (int i = 0; i < 36; ++i)
			channels[i] = TickRange{ 0, 0 };
	}

	SusExporter::SusExporter() : ticksPerBeat{ 480 }
	{

	}

	int SusExporter::getTicksFromMeasure(int measure)
	{
		int barLengthIndex = 0;
		for (int i = barLengthTicks.size() - 1; i >= 0; --i)
		{
			if (barLengthTicks[i].barLength.bar <= measure)
				barLengthIndex = std::max(0, i);
		}

		const BarLengthTicks& blt = barLengthTicks[barLengthIndex];
		int measureDiff = measure - blt.barLength.bar;
		int ticksPerMeasure = blt.barLength.length * ticksPerBeat;
		return blt.ticks + (measureDiff * ticksPerMeasure);
	}

	int SusExporter::getMeasureFromTicks(int ticks)
	{
		for (const auto& [barLength, barTicks] : barLengthTicks)
		{
			if (ticks >= barTicks)
				return barLength.bar + ((float)(ticks - barTicks) / (float)ticksPerBeat / barLength.length);
		}

		// no time signatures
		return 0;
	}

	void SusExporter::appendSlideData(const SUSNoteStream& slides, const std::string& infoPrefix)
	{
		ChannelProvider channelProvider;
		for (const auto& slide : slides)
		{
			int startTick = slide.begin()->tick;
			int endTick = slide.rbegin()->tick;
			int channel = channelProvider.generateChannel(startTick, endTick);

			char buf[10]{};
			std::string chStr(tostringBaseN(buf, channel, 36));
			for (const auto& note : slide)
				appendNoteData(note, infoPrefix, chStr);
		}
	};

	void SusExporter::appendData(int tick, std::string info, std::string data)
	{
		for (const auto& [barLength, barTicks] : barLengthTicks)
		{
			if (tick >= barTicks)
			{
				int currentMeasure = barLength.bar + ((float)(tick - barTicks) / (float)ticksPerBeat / barLength.length);
				MeasureMap& measureMap = measuresMap[currentMeasure];
				measureMap.measure = currentMeasure;

				NoteMap& map = measureMap.notesMap[info];
				map.data.push_back(NoteMap::RawData{ tick - barTicks, data });
				map.ticksPerMeasure = barLength.length * ticksPerBeat;
				break;
			}
		}
	}

	void SusExporter::appendNoteData(const SUSNote& note, const std::string infoPrefix, const std::string channel)
	{
		char buff1[10];
		std::string info = infoPrefix + tostringBaseN(buff1, note.lane, 36);
		if (channel != "-1")
			info.append(channel);

		char buff2[10];
		appendData(note.tick, info, std::to_string(note.type) + tostringBaseN(buff2, note.width, 36));
	}

	std::vector<std::string> SusExporter::getNoteLines(int baseMeasure)
	{
		std::vector<std::string> lines;

		// Notes on the same tick and lane
		std::vector<NoteMap::RawData> conflicts;

		// Holds possible note conflicts while processing other conflicts
		std::vector<NoteMap::RawData> temp;

		// Write note data
		for (const auto& [measure, map] : measuresMap)
		{
			int measureTicks = getTicksFromMeasure(measure);
			int base = (measure / 1000) * 1000;
			int offset = measure % 1000;
			if (base != baseMeasure)
			{
				lines.push_back("#MEASUREBS " + std::to_string(base));
				baseMeasure = base;
			}

			for (const auto& [info, notes] : map.notesMap)
			{
				conflicts.clear();

				int gcd = notes.ticksPerMeasure;
				for (const auto& raw : notes.data)
					gcd = std::gcd(raw.tick, gcd);

				// Number of notes including empty ones in a line
				int dataCount = notes.ticksPerMeasure / gcd;
				std::string data(dataCount * 2, '0');
				for (const auto& raw : notes.data)
				{
					int index = (raw.tick % notes.ticksPerMeasure) / gcd * 2;
					if (data.substr(index, 2) != "00")
					{
						conflicts.push_back(raw);
					}
					else
					{
						data[index + 0] = raw.data[0];
						data[index + 1] = raw.data[1];
					}
				}

				lines.push_back(formatString("#%03d%s:", measure - baseMeasure, info.c_str()) + data);

				while (conflicts.size())
				{
					temp.clear();
					std::string data2(dataCount * 2, '0');
					for (const auto& item : conflicts)
					{
						int index = (item.tick % notes.ticksPerMeasure) / gcd * 2;
						if (data2.substr(index, 2) != "00")
						{
							temp.push_back(item);
						}
						else
						{
							data2[index + 0] = item.data[0];
							data2[index + 1] = item.data[1];
						}
					}

					lines.push_back(formatString("#%03d%s:", measure - baseMeasure, info.c_str()) + data2);
					conflicts = temp;
				}
			}
		}

		return lines;
	}

	void SusExporter::dump(const SUS& sus, const std::string& filename, std::string comment)
	{
		std::vector<std::string> lines;
		if (!comment.empty())
		{
			// Make sure the comment is ignored by parsers. 
			lines.push_back(comment.substr(comment.find_first_not_of("#")));
		}

		// Write metadata
		for (const auto&[attrKey, attrValue] : sus.metadata.data)
		{
			std::string key = attrKey;
			std::transform(key.begin(), key.end(), key.begin(), ::toupper);

			lines.push_back("#" + key + " \"" + attrValue + "\"");
		}

		lines.push_back(IO::formatString("#WAVEOFFSET %g", sus.metadata.waveOffset));
		lines.push_back("");
		lines.push_back(IO::formatString("#REQUEST \"ticks_per_beat %d\"", ticksPerBeat));
		lines.push_back("");

		// Do we really need a copy of each here?
		auto barLengths = sus.barlengths;
		std::stable_sort(barLengths.begin(), barLengths.end(),
			[](const BarLength& a, const BarLength& b) { return a.bar < b.bar; });

		auto bpms = sus.bpms;
		std::stable_sort(bpms.begin(), bpms.end(),
			[](const BPM& a, const BPM& b) { return a.tick < b.tick; });

		auto taps = sus.taps;
		std::stable_sort(taps.begin(), taps.end(),
			[](const SUSNote& a, const SUSNote& b) { return a.tick < b.tick; });

		auto directionals = sus.directionals;
		std::stable_sort(directionals.begin(), directionals.end(),
			[](const SUSNote& a, const SUSNote& b) { return a.tick < b.tick; });

		auto slides = sus.slides;
		std::stable_sort(slides.begin(), slides.end(),
			[](const auto& a, const auto& b) { return a[0].tick < b[0].tick; });

		auto guides = sus.guides;
		std::stable_sort(guides.begin(), guides.end(),
			[](const auto& a, const auto& b) { return a[0].tick < b[0].tick; });

		measuresMap.clear();
		barLengthTicks.clear();
		int baseMeasure = 0;

		// Write time signatures
		for (const auto& barLength : barLengths)
		{
			int base = (barLength.bar / 1000) * 1000;
			int offset = barLength.bar % 1000;
			if (base != baseMeasure)
			{
				lines.push_back("#MEASUREBS " + std::to_string(base));
				baseMeasure = base;
			}

			lines.push_back(formatString("#%03d02: %g", offset, barLength.length));
		}

		lines.push_back("");

		int totalTicks = 0;
		for (int i = 0; i < barLengths.size(); ++i)
		{
			int next = 0;
			if (i + 1 < barLengths.size())
				next = barLengths[i + 1].bar;

			int startTick = totalTicks;
			totalTicks += (next - barLengths[i].bar) * barLengths[i].length * ticksPerBeat;
			barLengthTicks.push_back({ barLengths[i], startTick });
		}

		std::reverse(barLengthTicks.begin(), barLengthTicks.end());

		std::map<float, std::string> bpmIdentifiers;
		for (const auto& bpm : bpms)
		{
			char buf[10]{};
			std::string identifier = tostringBaseN(buf, bpmIdentifiers.size() + 1, 36);
			if (identifier.size() < 2)
				identifier = "0" + identifier;

			if (bpmIdentifiers.find(bpm.bpm) == bpmIdentifiers.end())
			{
				bpmIdentifiers[bpm.bpm] = identifier;
				lines.push_back(formatString("#BPM%s: %g", identifier.c_str(), bpm.bpm));
			}
		}

		// SUS can only handle up to 36^2 unique BPMs
		constexpr size_t maxBpmIdentifiers = (36ll * 36ll) - 1;
		if (bpmIdentifiers.size() >= maxBpmIdentifiers)
		{
			std::string errorMessage = IO::formatString("Too many BPM changes!\nNumber of unique identifiers (%l) exceeded limit (%l)", bpmIdentifiers.size(), maxBpmIdentifiers);
			printf("%s", errorMessage.c_str());

			throw std::exception(errorMessage.c_str());
		}

		// Group bpms by measure
		std::map<int, std::vector<BPM>> measuresBpms;
		for (const auto& bpm : bpms)
		{
			int measure = getMeasureFromTicks(bpm.tick);
			measuresBpms[measure].push_back(bpm);
		}

		for (const auto& [measure, bpms] : measuresBpms)
		{
			int base = (measure / 1000) * 1000;
			int offset = measure % 1000;
			if (base != baseMeasure)
			{
				lines.push_back("#MEASUREBS " + std::to_string(base));
				baseMeasure = base;
			}

			int measureTicks = getTicksFromMeasure(measure);
			int ticksPerMeasure = getTicksFromMeasure(measure + 1) - measureTicks;
			int gcd = ticksPerMeasure;

			for (const auto& bpm : bpms)
				gcd = std::gcd(bpm.tick, gcd);

			int dataCount = ticksPerMeasure / gcd;
			std::string data(dataCount * 2, '0');

			for (const auto& bpm : bpms)
			{
				int index = (bpm.tick - measureTicks) / gcd * 2;
				std::string_view identifier = bpmIdentifiers[bpm.bpm];
				data[index + 0] = identifier[0];
				data[index + 1] = identifier[1];
			}

			lines.push_back(formatString("#%03d08: %s", offset, data.c_str()));
		}

		lines.push_back("");

		std::string speedLine = "\"";
		for (int i = 0; i < sus.hiSpeeds.size(); ++i)
		{
			int measure = getMeasureFromTicks(sus.hiSpeeds[i].tick);
			int offsetTicks = sus.hiSpeeds[i].tick - getTicksFromMeasure(measure);
			float speed = sus.hiSpeeds[i].speed;

			speedLine.append(formatString("%d'%d:%g", measure, offsetTicks, speed));

			if (i < sus.hiSpeeds.size() - 1)
				speedLine.append(", ");
		}
		speedLine.append("\"");
		
		lines.push_back(formatString("#TIL00: %s", speedLine.c_str()));
		lines.push_back("#HISPEED 00");
		lines.push_back("#MEASUREHS 00");
		lines.push_back("");

		// Write short notes
		measuresMap.clear();
		for (const auto& tap : taps)
			appendNoteData(tap, "1", "-1");

		std::vector<std::string> tapLines = getNoteLines(baseMeasure);
		lines.insert(lines.end(), tapLines.begin(), tapLines.end());

		// Write directional notes
		measuresMap.clear();
		for (const auto& directional : directionals)
			appendNoteData(directional, "5", "-1");

		std::vector<std::string> directionalLines = getNoteLines(baseMeasure);
		lines.insert(lines.end(), directionalLines.begin(), directionalLines.end());

		// Write slide notes
		measuresMap.clear();
		appendSlideData(slides, "3");

		std::vector<std::string> slideLines = getNoteLines(baseMeasure);
		lines.insert(lines.end(), slideLines.begin(), slideLines.end());

		// Write guide notes
		measuresMap.clear();
		appendSlideData(guides, "9");

		std::vector<std::string> guideLines = getNoteLines(baseMeasure);
		lines.insert(lines.end(), guideLines.begin(), guideLines.end());

		File susfile(filename, FileMode::Write);

		susfile.writeAllLines(lines);
		susfile.flush();
		susfile.close();
	}
}