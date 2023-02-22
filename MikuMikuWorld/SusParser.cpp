#include "SusParser.h"
#include "StringOperations.h"
#include "File.h"

namespace MikuMikuWorld
{
	SusParser::SusParser()
		: ticksPerBeat{ 480 }, measureOffset{ 0 }, waveOffset{ 0 }
	{
	}

	bool SusParser::isCommand(const std::string& line)
	{
		if (isDigit(line.substr(1, 1)))
			return false;

		// test for text value commands
		if (line.find_first_of('"') != std::string::npos)
		{
			if (split(line, " ").size() < 2)
				return false;

			int firstQuote = line.find_first_of('"');
			int lastQuote = line.find_last_of('"');
			return firstQuote != lastQuote && lastQuote != std::string::npos;
		}

		return line.find_first_of(':') == std::string::npos;
	}

	int SusParser::toTicks(int measure, int i, int total)
	{
		int bIndex = 0;
		int accBarTicks = 0;
		for (int i = 0; i < bars.size(); ++i)
		{
			if (bars[i].measure > measure)
				break;

			bIndex = i;
			accBarTicks += bars[i].ticks;
		}

		return accBarTicks
			+ ((measure - bars[bIndex].measure) * bars[bIndex].ticksPerMeasure)
			+ ((i * bars[bIndex].ticksPerMeasure) / total);
	}

	std::vector<std::vector<SUSNote>> SusParser::toSlides(const std::vector<SUSNote>& stream)
	{
		std::vector<SUSNote> sortedStream = stream;
		std::stable_sort(sortedStream.begin(), sortedStream.end(),
			[](const SUSNote& n1, const SUSNote& n2) { return n1.tick < n2.tick; });

		bool newSlide = true;
		std::vector<std::vector<SUSNote>> slides;
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

	std::vector<SUSNote> SusParser::toNotes(const std::string& header, const std::string& data, int measureBase)
	{
		std::vector<SUSNote> notes;
		int measure = measureBase + std::stoul(header.substr(0, 3).c_str(), nullptr, 10);
		for (int i = 0; i < data.size(); i += 2)
		{
			// no data
			if (data.substr(i, 2) == "00")
				continue;

			notes.push_back(SUSNote{ toTicks(measure, i, data.size()),
				(int)std::stoul(header.substr(4, 1), nullptr, 36),
				(int)std::stoul(data.substr(i + 1, 1), nullptr, 36),
				(int)std::stoul(data.substr(i, 1), nullptr, 36)
			});
		}

		return notes;
	}

	void SusParser::processCommand(std::string& line)
	{
		int keyPos = line.find_first_of(' ');
		if (keyPos == std::string::npos)
			return;

		std::string key = line.substr(1, keyPos - 1);
		std::string value = line.substr(keyPos + 1);

		std::transform(key.begin(), key.end(), key.begin(), ::toupper);

		// exclude double quotes around the value
		if (startsWith(value, "\"") && endsWith(value, "\""))
			value = value.substr(1, value.size() - 2);

		if (key == "TITLE")
			title = value;
		else if (key == "ARTIST")
			artist = value;
		else if (key == "DESIGNER")
			designer = value;
		else if (key == "WAVEOFFSET")
			waveOffset = atof(value.c_str());
		else if (key == "MEASUREBS")
			measureOffset = atoi(value.c_str());
		else if (key == "REQUEST")
		{
			std::vector<std::string> requestArgs = split(value, " ");
			if (requestArgs.size() == 2 && requestArgs[0] == "ticks_per_beat")
				ticksPerBeat = atoi(requestArgs[1].c_str());
		}
	}

	SUS SusParser::parse(const std::string& filename)
	{
		std::wstring wFilename = mbToWideStr(filename);

		File susFile(wFilename, L"r");
		auto lines = susFile.readAllLines();
		susFile.close();

		std::vector<SusLineData> noteLines;
		std::vector<SusLineData> bpmLines;
		std::vector<BarLength> barLengths;
		for (int i = 0; i < lines.size(); ++i)
		{
			lines[i] = trim(lines[i]);
			if (!startsWith(lines[i], "#"))
				continue;

			if (isCommand(lines[i]))
			{
				processCommand(lines[i]);
			}
			else
			{
				auto l = split(lines[i], ":");
				if (l.size() < 2) // no ':' found
					continue;

				std::string header = trim(l[0]).substr(1);
				std::string data = l.size() > 1 ? trim(l[1]) : "";

				if (header.size() == 5 && header.substr(header.size() - 2) == "02" && isDigit(header))
				{
					barLengths.push_back({ measureOffset + atoi(header.substr(0, 3).c_str()), (float)atof((data.c_str())) });
				}
				else if (header.size() == 5 && startsWith(header, "BPM"))
				{
					bpmDefinitions[header.substr(3)] = atof(data.c_str());
				}
				else if (header.size() == 5 && header.substr(header.size() - 2, 2) == "08")
				{
					bpmLines.push_back({ i, measureOffset, lines[i] });
				}
				else if (header.size() == 5 || header.size() == 6)
				{
					noteLines.push_back({ i, measureOffset, lines[i] });
				}
			}
		}

		// process time signatures
		if (!barLengths.size())
			barLengths.push_back({ 0, 4.0f });

		int ticks = 0;
		bars.clear();
		bars.reserve(barLengths.size());
		for (int i = 0; i < barLengths.size(); ++i)
		{
			int measure = barLengths[i].bar;
			int ticksPerMeasure = barLengths[i].length * ticksPerBeat;
			ticks = ticks + i == 0 ? 0 : (measure - barLengths[i - 1].bar) * barLengths[i - 1].length * ticksPerBeat;

			bars.push_back(Bar{ measure, ticksPerMeasure, ticks });
		}
		std::sort(bars.begin(), bars.end(),
			[](const Bar& b1, const Bar& b2) { return b1.measure < b2.measure; });

		// process bpm changes
		std::vector<BPM> bpms;
		for (auto& line : bpmLines)
		{
			auto l = split(line.line, ":");
			if (l.size() < 2) // no ':' found
				continue;

			std::string header = trim(l[0]).substr(1);
			std::string data = l.size() > 1 ? trim(l[1]) : "";

			for (int i = 0; i < data.size(); i += 2)
			{
				std::string subData = data.substr(i, 2);
				if (subData == "00")
					continue;

				int tick = toTicks(line.measureOffset + atoi(header.substr(0, 3).c_str()), i, data.size());
				float bpm = 120;

				std::string definition = data.substr(i, 2);
				if (bpmDefinitions.find(definition) != bpmDefinitions.end())
					bpm = bpmDefinitions[definition];

				bpms.push_back({tick, bpm});
			}
		}
		std::sort(bpms.begin(), bpms.end(),
			[](const BPM& a, const BPM& b) { return a.tick < b.tick; });

		// process notes
		std::vector<SUSNote> taps;
		std::vector<SUSNote> directionals;
		std::unordered_map<int, std::vector<SUSNote>> streams;
		for (auto& line : noteLines)
		{
			auto l = split(line.line, ":");
			if (l.size() < 2) // no ':' found
				continue;

			std::string header = trim(l[0]).substr(1);
			std::string data = l.size() > 1 ? trim(l[1]) : "";

			if (header.size() == 5 && header[3] == '1')
			{
				std::vector<SUSNote> appendNotes = toNotes(header, data, line.measureOffset);
				taps.reserve(taps.size() + appendNotes.size());
				taps.insert(taps.end(), appendNotes.begin(), appendNotes.end());
			}
			else if (header.size() == 6 && header[3] == '3')
			{
				int channel = std::stoul(header.substr(5, 1), nullptr, 36);
				std::vector<SUSNote> appendNotes = toNotes(header, data, line.measureOffset);
				streams[channel].reserve(streams[channel].size() + appendNotes.size());
				streams[channel].insert(streams[channel].end(), appendNotes.begin(), appendNotes.end());
			}
			else if (header.size() == 5 && header[3] == '5')
			{
				std::vector<SUSNote> appendNotes = toNotes(header, data, line.measureOffset);
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

		SUSMetadata metadata;
		metadata.data["title"] = title;
		metadata.data["artist"] = artist;
		metadata.data["designer"] = designer;
		metadata.waveOffset = waveOffset;

		return SUS{ metadata, taps, directionals, slides, bpms, barLengths };
	}
}