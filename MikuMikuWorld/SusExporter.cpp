#include "SUS.h"
#include "SusExporter.h"
#include "StringOperations.h"
#include "File.h"
#include "ChannelProvider.h"
#include <algorithm>
#include <numeric>

namespace MikuMikuWorld
{
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
	}

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

	void SusExporter::dump(const SUS& sus, const std::string& filename, std::string comment)
	{
		std::vector<std::string> lines;
		if (comment.size())
		{
			// make sure the comment is ignored by parsers. 
			lines.push_back(comment.substr(comment.find_first_not_of("#")));
		}

		// write metadata
		for (const auto&[attrKey, attrValue] : sus.metadata.data)
		{
			std::string key = attrKey;
			std::transform(key.begin(), key.end(), key.begin(), ::toupper);

			std::string line = "#" + key + " \"" + attrValue + "\"";
			lines.push_back(line);
		}

		lines.push_back(std::string("#WAVEOFFSET " + std::to_string(sus.metadata.waveOffset)));
		lines.push_back("");
		lines.push_back("#REQUEST \"ticks_per_beat " + std::to_string(ticksPerBeat) + "\"");
		lines.push_back("");

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

		measuresMap.clear();
		barLengthTicks.clear();
		channelProvider.clear();
		int baseMeasure = 0;

		// write time signatures
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

		// write tempo changes
		if (bpms.size() >= (36 * 36) - 1)
		{
			printf("Too much tempo changes bpms.size() >= 36^2 - 1: %d", (int)bpms.size());
			throw bpms.size();
		}

		std::unordered_map<float, std::string> bpmIdentifiers;
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

		// group bpms by measure
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

			std::map<int, std::string> data;
			for (const auto& bpm : bpms)
				data[bpm.tick - measureTicks] = bpmIdentifiers[bpm.bpm];

			std::string values;
			for (int i = 0; i < ticksPerMeasure; i += gcd)
				values += (data.find(i) == data.end() ? "00" : data[i]);

			lines.push_back(formatString("#%03d08: %s", offset, values.c_str()));
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

		// prepare note data
		for (const auto& tap : taps)
			appendNoteData(tap, "1", "-1");

		for (const auto& directional : directionals)
			appendNoteData(directional, "5", "-1");

		for (const auto& steps : slides)
		{
			int startTick = steps[0].tick;
			int endTick = steps[steps.size() - 1].tick;
			int channel = channelProvider.generateChannel(startTick, endTick);

			char buf[10]{};
			std::string chStr(tostringBaseN(buf, channel, 36));
			for (const auto& note : steps)
				appendNoteData(note, "3", chStr);
		}

		// write note data
		for (const auto& [measure, map] : measuresMap)
		{
			int base = (measure / 1000) * 1000;
			int offset = measure % 1000;
			if (base != baseMeasure)
			{
				lines.push_back("#MEASUREBS " + std::to_string(base));
				baseMeasure = base;
			}

			for (const auto& [info, notes] : map.notesMap)
			{
				int gcd = notes.ticksPerMeasure;
				for (const auto& raw : notes.data)
					gcd = std::gcd(raw.tick, gcd);

				std::map<int, std::string> data;
				for (const auto& raw : notes.data)
					data[raw.tick % notes.ticksPerMeasure] = raw.data;

				std::vector<std::string> values;
				for (int i = 0; i < notes.ticksPerMeasure; i += gcd)
					values.push_back((data.find(i) == data.end() ? "00" : data[i]));

				std::string line = formatString("#%03d%s:", measure - baseMeasure, info.c_str());
				for (const auto& value : values)
					line.append(value);

				lines.push_back(line);
			}
		}

		std::wstring wFilename = mbToWideStr(filename);
		File susfile(wFilename, L"w");

		susfile.writeAllLines(lines);
		susfile.flush();
		susfile.close();
	}
}