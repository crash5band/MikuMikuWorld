#include "SUS.h"
#include "File.h"
#include "StringOperations.h"
#include "ChannelProvider.h"
#include <regex>
#include <map>
#include <numeric>
#include "Application.h"

namespace MikuMikuWorld
{
	int toTicks(int measure, int i, int total, const std::vector<Bar>& bars)
	{
		int bIndex = 0;
		for (int i = 0; i < bars.size(); ++i)
		{
			if (measure >= bars[i].measure)
				bIndex = i;
		}

		return bars[bIndex].ticks
			+ ((measure - bars[bIndex].measure) * bars[bIndex].ticksPerMeasure)
			+ ((i * bars[bIndex].ticksPerMeasure) / total);
	}

	std::vector<std::vector<SUSNote>> toSlides(const std::vector<SUSNote>& stream)
	{
		std::vector<std::vector<SUSNote>> slides;
		int curr = -1;
		
		std::vector<SUSNote> sortedStream = stream;
		std::stable_sort(sortedStream.begin(), sortedStream.end(),
			[](const SUSNote& n1, const SUSNote& n2) { return n1.tick < n2.tick; });

		bool newSlide = true;
		std::vector<SUSNote> currentSlides;
		for (const auto& note : sortedStream)
		{
			if (newSlide)
			{
				currentSlides.clear();
				newSlide = false;
			}

			currentSlides.push_back(note);
			if (note.type == 2)
			{
				slides.push_back(currentSlides);
				newSlide = true;
			}
		}

		return slides;
	}

	std::vector<SUSNote> toNotes(const std::string& header, const std::string& data, const std::vector<Bar>& bars)
	{
		std::vector<SUSNote> notes;
		int measure = std::stoul(header.substr(0, 3).c_str(), nullptr, 10);
		for (int i = 0; i < data.size(); i += 2)
		{
			// no data
			if (data.substr(i, 2) == "00")
				continue;

			notes.push_back(SUSNote{ toTicks(measure, i, data.size(), bars),
				(int)std::stoul(header.substr(4, 1), nullptr, 36),
				(int)std::stoul(data.substr(i + 1, 1), nullptr, 36),
				(int)std::stoul(data.substr(i, 1), nullptr, 36) });
		}

		return notes;
	}

	void appendData(int tick, std::string info, std::string data,
		const std::vector<std::pair<BarLength, int>>& barLengths, int ticksPerBeat, std::unordered_map<std::string, NoteMap>& noteMaps)
	{
		for (const auto&[barLength, barTicks] : barLengths)
		{
			if (tick >= barTicks)
			{
				int currentMeasure = barLength.bar + ((tick - barTicks) / ticksPerBeat / barLength.length);
				std::string key = formatString("%03d", currentMeasure) + info;

				NoteMap& map = noteMaps[key];
				map.data.push_back(NoteMap::RawData{ tick - barTicks, data });
				map.ticksPerMeasure = barLength.length * ticksPerBeat;
				break;
			}
		}
	}

	void appendNoteData(const SUSNote& note, const std::string infoPrefix, const std::string channel,
		const std::vector<std::pair<BarLength, int>>& barLengths, int ticksPerBeat, std::unordered_map<std::string, NoteMap>& noteMaps)
	{
		char buff1[10];
		std::string info = infoPrefix + tostringBaseN(buff1, note.lane, 36);
		if (channel != "-1")
			info.append(channel);

		char buff2[10];
		std::string data = std::to_string(note.type) + tostringBaseN(buff2, note.width, 36);
		appendData(note.tick, info, data, barLengths, ticksPerBeat, noteMaps);
	}

	SUSMetadata processSUSMetadata(const std::unordered_map<std::string, std::string>& data)
	{
		SUSMetadata result;
		for (const auto& it : data)
		{
			std::string key = it.first.substr(1);
			std::string val = it.second;

			if (val.size())
			{
				val = trim(val);
				if (startsWith(val, "\""))
					val = val.substr(1, val.size() - 2);
			}

			std::string rKey = key;
			std::transform(key.begin(), key.end(), rKey.begin(),
				[](unsigned char c) { return std::tolower(c); });

			if (rKey == "waveoffset")
				result.waveOffset = atof(val.c_str());
			else if (rKey == "movieoffset")
				result.movieOffset = atof(val.c_str());
			else if (rKey == "basebpm")
				result.baseBPM = atof(val.c_str());
			else if (rKey == "request")
				result.requests.push_back(val);
			else
				result.data[rKey] = val;
		}

		return result;
	}

	SUS processSusScore(const std::vector<std::string>& score, const std::unordered_map<std::string, std::string>& metadata)
	{
		SUSMetadata processedMetadata = processSUSMetadata(metadata);
		
		int ticksPerBeat = 480;
		for (const auto& request : processedMetadata.requests)
		{
			if (startsWith(request, "ticks_per_beat"))
				ticksPerBeat = atoi(split(request, " ")[1].c_str());
		}

		std::vector<BarLength> barLengths;
		for (const auto& line : score)
		{
			auto l = split(line, ":");
			std::string header = l[0].substr(1);
			std::string data = l[1];

			if (header.size() == 5 && header.substr(header.size() - 2) == "02" && isDigit(header))
				barLengths.push_back(BarLength{ atoi(header.substr(0, 3).c_str()), (float)atof((data.c_str())) });
		}

		if (!barLengths.size())
			barLengths.push_back(BarLength{ 0, 4.0f });

		std::sort(barLengths.begin(), barLengths.end(),
			[](const BarLength& b1, const BarLength& b2) { return b1.length < b2.length; });

		int ticks = 0;

		std::vector<Bar> bars;
		bars.reserve(barLengths.size());
		for (int i = 0; i < barLengths.size(); ++i)
		{
			int measure = barLengths[i].bar;
			int ticksPerMeasure = barLengths[i].length * ticksPerBeat;
			ticks = ticks + i == 0 ? 0 : (measure - barLengths[i - 1].bar) * barLengths[i - 1].length * ticksPerBeat;

			bars.push_back(Bar{ measure, ticksPerMeasure, ticks });
		}
		std::reverse(bars.begin(), bars.end());

		std::unordered_map<std::string, float> bpmMap;
		std::unordered_map<int, std::string> bpmChanges;
		std::vector<SUSNote> taps;
		std::vector<SUSNote> directionals;
		std::unordered_map<int, std::vector<SUSNote>> streams;
		for (const auto& line : score)
		{
			auto l = split(line, ":");
			std::string header = l[0].substr(1);
			std::string data = l[1];

			if (header.size() == 5 && startsWith(header, "BPM"))
				bpmMap[header.substr(3)] = atof(data.c_str());
			else if (header.size() == 5 && header.substr(header.size() - 2, 2) == "08")
			{
				data = trim(data);
				for (int i = 0; i < data.size(); i += 2)
				{
					std::string subdata = data.substr(i, 2);
					if (subdata == "00")
						continue;

					int measure = std::stoul(header.substr(0, 3), nullptr, 10);
					int t = toTicks(measure, i, data.size(), bars);
					bpmChanges[t] = subdata;
				}
			}
			else if (header.size() == 5 && header[3] == '1')
			{
				std::vector<SUSNote> appendNotes = toNotes(header, data, bars);
				taps.reserve(taps.size() + appendNotes.size());
				taps.insert(taps.end(), appendNotes.begin(), appendNotes.end());
			}
			else if (header.size() == 6 && header[3] == '3')
			{
				int channel = std::stoul(header.substr(5, 1), nullptr, 36);
				std::vector<SUSNote> appendNotes = toNotes(header, data, bars);
				streams[channel].reserve(streams[channel].size() + appendNotes.size());
				streams[channel].insert(streams[channel].end(), appendNotes.begin(), appendNotes.end());
			}
			else if (header.size() == 5 && header[3] == '5')
			{
				std::vector<SUSNote> appendNotes = toNotes(header, data, bars);
				directionals.reserve(directionals.size() + appendNotes.size());
				directionals.insert(directionals.end(), appendNotes.begin(), appendNotes.end());
			}
		}

		std::vector<std::vector<SUSNote>> slides;
		for (auto& stream : streams)
		{
			auto appendSlides = toSlides(stream.second);
			slides.reserve(slides.size() + appendSlides.size());
			slides.insert(slides.end(), appendSlides.begin(), appendSlides.end());
		}

		std::vector<BPM> bpms;
		{
			std::map<int, std::string> sortedBpmChanges;
			for (auto& change : bpmChanges)
				sortedBpmChanges[change.first] = change.second;

			for (const auto& obj : sortedBpmChanges)
				bpms.push_back(BPM{ obj.first, (bpmMap.find(obj.second) != bpmMap.end() ? bpmMap[obj.second] : 0) });
		}

		return SUS{ processedMetadata, taps, directionals, slides, bpms, barLengths };
	}

	SUS loadSUS(const std::string& filename)
	{
		std::unordered_map<std::string, std::string> metadata;
		std::vector<std::string> scoredata;

		std::wstring wFilename = mbToWideStr(filename);

		File susFile(wFilename, L"r");
		auto lines = susFile.readAllLines();
		susFile.close();

		std::regex regex("^#(\\w+):\\s*(.*)$");
		std::smatch sm;

		for (std::string& line : lines)
		{
			if (!startsWith(line, "#"))
				continue;

			line = trim(line);
			std::regex_match(line.cbegin(), line.cend(), sm, regex);
			if (sm.size())
				scoredata.push_back(line);
			else
			{
				size_t keyEnd = line.find_first_of(' ');
				metadata[line.substr(0, keyEnd - 0)] = line.substr(keyEnd + 1);
			}
		}

		return processSusScore(scoredata, metadata);
	}

	void saveSUS(const SUS& score, const std::string& filename)
	{
		std::vector<std::string> lines;
		const std::string comment = "This file was generated by MikuMikuWorld " + Application::getAppVersion() + "\n";
		
		lines.push_back(comment);

		int ticksPerBeat = 480;
		for (const auto& attr : score.metadata.data)
		{
			std::string key = attr.first;
			std::transform(attr.first.begin(), attr.first.end(), key.begin(),
				[](unsigned char c) { return std::toupper(c); });

			std::string a = "#" + key + " \"" + attr.second + "\"";
			lines.push_back(a);
		}

		lines.push_back("");
		for (const auto& request : score.metadata.requests)
		{
			std::string r = "#REQUEST \"" + request + "\"";
			if (startsWith(request, "ticks_per_beat"))
				ticksPerBeat = atoi(split(request, " ")[1].c_str());

			lines.push_back(r);
		}

		lines.push_back(std::string("#WAVEOFFSET " + std::to_string(score.metadata.waveOffset)));

		std::unordered_map<std::string, NoteMap> noteMaps;

		auto barLengths = score.barlengths;
		std::stable_sort(barLengths.begin(), barLengths.end(),
			[](const BarLength& a, const BarLength& b) { return a.bar < b.bar; });

		auto bpms = score.bpms;
		std::stable_sort(bpms.begin(), bpms.end(),
			[](const BPM& a, const BPM& b) { return a.tick < b.tick; });

		auto taps = score.taps;
		std::stable_sort(taps.begin(), taps.end(),
			[](const SUSNote& a, const SUSNote& b) { return a.tick < b.tick; });

		auto directionals = score.directionals;
		std::stable_sort(directionals.begin(), directionals.end(),
			[](const SUSNote& a, const SUSNote& b) { return a.tick < b.tick; });

		auto slides = score.slides;
		std::stable_sort(slides.begin(), slides.end(),
			[](const auto& a, const auto& b) { return a[0].tick < b[0].tick; });

		lines.push_back("");

		for (const auto& entry : barLengths)
		{
			std::string line = "#";
			line.append(formatString("%03d", entry.bar));
			line.append("02:");
			line.append(formatString("%g", entry.length));
			lines.push_back(line);
		}

		int totalTicks = 0;
		std::vector<std::pair<BarLength, int>> barLengthTicks;
		for (int i = 0; i < barLengths.size(); ++i)
		{
			int next = 0;
			if (i + 1 < barLengths.size())
				next = barLengths[i + 1].bar;

			int startTick = totalTicks;
			totalTicks += (next - barLengths[i].bar) * barLengths[i].length * ticksPerBeat;
			barLengthTicks.push_back(std::pair<BarLength, int>(barLengths[i], startTick));
		}

		std::reverse(barLengthTicks.begin(), barLengthTicks.end());

		if (bpms.size() >= (36 * 36) - 1)
		{
			printf("Too much tempo changes bpms.size() >= 36^2 - 1: %d", (int)bpms.size());
			throw bpms.size();
		}

		std::unordered_map<float, std::string> bpmIdentifiers;
		for (const auto& bpm : bpms)
		{
			char buf[10];
			std::string identifier = tostringBaseN(buf, bpmIdentifiers.size() + 1, 36);
			if (identifier.size() < 2)
				identifier = "0" + identifier;

			if (bpmIdentifiers.find(bpm.bpm) == bpmIdentifiers.end())
			{
				bpmIdentifiers[bpm.bpm] = identifier;

				std::string line = "#BPM" + identifier + ":" + formatString("%g", bpm.bpm);
				lines.push_back(line);
			}

			appendData(bpm.tick, "08", bpmIdentifiers[bpm.bpm], barLengthTicks, ticksPerBeat, noteMaps);
		}

		for (const auto& tap : taps)
			appendNoteData(tap, "1", "-1", barLengthTicks, ticksPerBeat, noteMaps);

		for (const auto& directional : directionals)
			appendNoteData(directional, "5", "-1", barLengthTicks, ticksPerBeat, noteMaps);

		ChannelProvider provider;
		for (const auto& steps : slides)
		{
			int startTick = steps[0].tick;
			int endTick = steps[steps.size() - 1].tick;
			int channel = provider.generateChannel(startTick, endTick);

			char buf[10];
			std::string chStr(tostringBaseN(buf, channel, 36));
			for (const auto& note : steps)
				appendNoteData(note, "3", chStr, barLengthTicks, ticksPerBeat, noteMaps);
		}

		for (const auto&[tag, map] : noteMaps)
		{
			int gcd = map.ticksPerMeasure;
			for (const auto& raw : map.data)
				gcd = std::gcd(raw.tick, gcd);

			/*
			std::unordered_map<int, std::string> data;
			for (const auto& raw : map.data)
				data[raw.tick % map.ticksPerMeasure] = raw.data;

			std::vector<std::string> values;
			for (int i = 0; i < map.ticksPerMeasure; i += gcd)
			{
				if (data.find(i) != data.end())
					values.push_back(data[i]);
				else
					values.push_back("00");
			}
			*/

			int dataPtr = 0;

			std::string line = "#" + tag + ":";
			line.reserve(line.size() + (gcd / map.data.size()));

			for (int i = 0; i < map.ticksPerMeasure; i += gcd)
				if (dataPtr >= map.data.size())
					line.append("00");
				else
					line.append(i == (map.data[dataPtr].tick % map.ticksPerMeasure) ? map.data[dataPtr++].data : "00");

			lines.push_back(line);
		}

		std::wstring wFilename = mbToWideStr(filename);
		File susfile(wFilename, L"w");
		susfile.writeAllLines(lines);

		susfile.close();
	}
}
