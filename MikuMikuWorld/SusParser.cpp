#include "SusParser.h"
#include "IO.h"
#include "File.h"
#include <algorithm>

using namespace IO;

namespace MikuMikuWorld
{
	SusDataLine::SusDataLine(int measureOffset, const std::string& line) : measureOffset{ measureOffset }
	{
		size_t separatorIndex = line.find_first_of(":");
		header = trim(line.substr(1, separatorIndex - 1));
		data = trim(line.substr(separatorIndex + 1));

		std::string headerMeasure = header.substr(0, 3);
		if (isDigit(headerMeasure))
		{
			measure = atoi(headerMeasure.c_str());
		}
	}

	SusParser::SusParser()
		: ticksPerBeat{ 480 }, measureOffset{ 0 }, waveOffset{ 0 }
	{
	}

	bool SusParser::isCommand(const std::string& line)
	{
		if (isdigit(line[1]))
			return false;

		// Test for text value commands
		if (line.find_first_of('"') != std::string::npos)
		{
			std::vector<std::string> lineSplit = split(line, " ");
			if (lineSplit.size() < 2)
				return false;

			if (lineSplit[0].find_first_of(':') != std::string::npos)
				return false;

			int firstQuote = line.find_first_of('"');
			int lastQuote = line.find_last_of('"');
			return firstQuote != lastQuote && lastQuote != std::string::npos;
		}

		return line.find_first_of(':') == std::string::npos;
	}

	int SusParser::getTicks(int measure, int i, int total)
	{
		int bIndex = 0;
		int accBarTicks = 0;
		for (size_t i = 0; i < bars.size(); ++i)
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

	SUSNoteStream SusParser::getNoteStream(const std::vector<SUSNote>& stream)
	{
		std::vector<SUSNote> sortedStream = stream;
		std::stable_sort(sortedStream.begin(), sortedStream.end(),
			[](const SUSNote& n1, const SUSNote& n2) { return n1.tick < n2.tick; });

		bool newSlide = true;
		SUSNoteStream slides;
		std::vector<SUSNote> currentSlides;
		for (const auto& note : sortedStream)
		{
			if (newSlide)
			{
				currentSlides.clear();
				newSlide = false;
			}

			currentSlides.push_back(note);

			// Found slide end
			if (note.type == 2)
			{
				slides.push_back(currentSlides);
				newSlide = true;
			}
		}

		return slides;
	}

	std::vector<SUSNote> SusParser::getNotes(const SusDataLine& line)
	{
		std::vector<SUSNote> notes;
		for (size_t i = 0; i < line.data.size(); i += 2)
		{
			if (line.data[i] == '0' && line.data[i + 1] == '0')
				continue;

			notes.push_back(SUSNote{ getTicks(line.getEffectiveMeasure(), i, line.data.size()),
				(int)std::stoul(line.header.substr(4, 1), nullptr, 36),
				(int)std::stoul(line.data.substr(i + 1, 1), nullptr, 36),
				(int)std::stoul(line.data.substr(i, 1), nullptr, 36)
			});
		}

		return notes;
	}

	std::vector<BPM> SusParser::getBpms(const std::vector<SusDataLine>& bpmLines)
	{
		std::vector<BPM> bpms;
		for (const auto& line : bpmLines)
		{
			for (size_t i = 0; i < line.data.size(); i += 2)
			{
				if (line.data[i] == '0' && line.data[i + 1] == '0')
					continue;

				int tick = getTicks(line.getEffectiveMeasure(), i, line.data.size());
				float bpm = 120;

				std::string subData = line.data.substr(i, 2);
				if (bpmDefinitions.find(subData) != bpmDefinitions.end())
					bpm = bpmDefinitions[subData];

				bpms.push_back({ tick, bpm });
			}
		}
		std::sort(bpms.begin(), bpms.end(),
			[](const BPM& a, const BPM& b) { return a.tick < b.tick; });

		return bpms;
	}

	std::vector<Bar> SusParser::getBars(const std::vector<BarLength>& barLengths)
	{
		std::vector<Bar> bars;
		bars.reserve(barLengths.size());
		bars.push_back(Bar{ barLengths[0].bar, (int)(barLengths[0].length * ticksPerBeat), 0 });
		for (size_t i = 1; i < barLengths.size(); ++i)
		{
			int measure = barLengths[i].bar;
			int ticksPerMeasure = barLengths[i].length * ticksPerBeat;
			int ticks = (measure - barLengths[i - 1].bar) * barLengths[i - 1].length * ticksPerBeat;

			bars.push_back(Bar{ measure, ticksPerMeasure, ticks });
		}
		std::sort(bars.begin(), bars.end(),
			[](const Bar& b1, const Bar& b2) { return b1.measure < b2.measure; });

		return bars;
	}

	std::vector<HiSpeed> SusParser::getHiSpeeds(const std::vector<SusDataLine>& hiSpeedLines)
	{
		std::vector<HiSpeed> hiSpeeds;
		for (const auto& line : hiSpeedLines)
		{
			std::string lineData = line.data;
			size_t firstQuote = lineData.find_first_of('"') + 1;
			size_t lastQuote = lineData.find_last_of('"');

			lineData = lineData.substr(firstQuote, lastQuote - firstQuote);
			if (!lineData.size())
				continue;

			std::vector<std::string> speedChanges = split(lineData, ",");
			for (const auto& change : speedChanges)
			{
				int measure = 0;
				int tick = 0;
				float speed = 1.0f;

				size_t i1 = 0, i2 = 0;

				i2 = change.find_first_of("'", i1);
				measure = atoi(change.substr(i1, i2 - i1).c_str());

				i1 = ++i2;
				i2 = change.find_first_of(":", i2);
				tick = atoi(change.substr(i1, i2 - i1).c_str());

				i1 = ++i2;
				speed = atof(change.substr(i1).c_str());

				int measureTicks = getTicks(measure, 0, 1);
				hiSpeeds.push_back({ measureTicks + tick, speed });
			}
		}

		return hiSpeeds;
	}

	void SusParser::processCommand(std::string& line)
	{
		size_t keyPos = line.find_first_of(' ');
		if (keyPos == std::string::npos)
			return;

		std::string key = line.substr(1, keyPos - 1);
		std::string value = line.substr(keyPos + 1);

		std::transform(key.begin(), key.end(), key.begin(), ::toupper);

		// Exclude double quotes around the value
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
		File susFile(mbToWideStr(filename), L"r");
		auto lines = susFile.readAllLines();
		susFile.close();

		SUS sus{};

		std::vector<SusDataLine> noteLines;
		std::vector<SusDataLine> bpmLines;
		std::vector<SusDataLine> hiSpeedLines;
		bpmDefinitions.clear();
		measureOffset = 0;

		for (auto& l : lines)
		{
			std::string line = trim(l);
			if (!startsWith(line, "#"))
				continue;

			if (isCommand(line))
			{
				processCommand(line);
			}
			else
			{
				SusDataLine susLine = SusDataLine(measureOffset, line);
				const std::string& header = susLine.header;

				if (header.size() != 5 && header.size() != 6)
					continue;

				if (endsWith(header, "02") && isDigit(header))
				{
					sus.barlengths.push_back({ susLine.getEffectiveMeasure(), (float)atof((susLine.data.c_str()))});
				}
				else if (startsWith(header, "BPM"))
				{
					bpmDefinitions[header.substr(3)] = atof(susLine.data.c_str());
				}
				else if (endsWith(header, "08"))
				{
					bpmLines.push_back(susLine);
				}
				else if (startsWith(header, "TIL"))
				{
					hiSpeedLines.push_back(susLine);
				}
				else
				{
					noteLines.push_back(susLine);
				}
			}
		}

		// There must be a time signature
		if (sus.barlengths.empty())
			sus.barlengths.push_back({ 0, 4.0f });

		bars = getBars(sus.barlengths);
		sus.bpms = getBpms(bpmLines);
		sus.hiSpeeds = getHiSpeeds(hiSpeedLines);

		std::vector<SUSNote> taps;
		std::vector<SUSNote> directionals;
		std::unordered_map<int, std::vector<SUSNote>> slideStreams;
		std::unordered_map<int, std::vector<SUSNote>> guideStreams;
		for (const auto& line : noteLines)
		{
			const std::string& header = line.header;
			if (header.size() == 5 && header[3] == '1')
			{
				std::vector<SUSNote> appendNotes = getNotes(line);
				sus.taps.insert(sus.taps.end(), appendNotes.begin(), appendNotes.end());
			}
			else if (header.size() == 5 && header[3] == '5')
			{
				std::vector<SUSNote> appendNotes = getNotes(line);
				sus.directionals.insert(sus.directionals.end(), appendNotes.begin(), appendNotes.end());
			}
			else if (header.size() == 6 && header[3] == '3')
			{
				int channel = std::stoul(header.substr(5, 1), nullptr, 36);

				std::vector<SUSNote> appendNotes = getNotes(line);
				slideStreams[channel].insert(slideStreams[channel].end(), appendNotes.begin(), appendNotes.end());
			}
			else if (header.size() == 6 && header[3] == '9')
			{
				int channel = std::stoul(header.substr(5, 1), nullptr, 36);

				std::vector<SUSNote> appendNotes = getNotes(line);
				guideStreams[channel].insert(guideStreams[channel].end(), appendNotes.begin(), appendNotes.end());
			}
		}

		for (const auto& stream : slideStreams)
		{
			auto appendSlides = getNoteStream(stream.second);
			sus.slides.insert(sus.slides.end(), appendSlides.begin(), appendSlides.end());
		}

		for (const auto& stream : guideStreams)
		{
			auto appendGuides = getNoteStream(stream.second);
			sus.guides.insert(sus.guides.end(), appendGuides.begin(), appendGuides.end());
		}

		sus.metadata.data["title"] = title;
		sus.metadata.data["artist"] = artist;
		sus.metadata.data["designer"] = designer;
		sus.metadata.waveOffset = waveOffset;

		return sus;
	}
}