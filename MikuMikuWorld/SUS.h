#pragma once
#include <unordered_map>
#include <map>
#include <vector>
#include <string>

namespace MikuMikuWorld
{
	struct SUSNote
	{
		int tick;
		int lane;
		int width;
		int type;
	};

	struct BPM
	{
		int tick;
		float bpm;
	};

	struct Bar
	{
		int measure;
		int ticksPerMeasure;
		int ticks;
	};

	struct BarLength
	{
		int bar;
		float length;
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

	struct SUSMetadata
	{
		std::unordered_map<std::string, std::string> data;
		std::vector<std::string> requests;
		float waveOffset;
		float movieOffset;
		float baseBPM;

		SUSMetadata() :
			waveOffset{ 0 }, movieOffset{ 0 }, baseBPM{ 0 } {}
	};

	struct SUS
	{
		SUSMetadata metadata;
		std::vector<SUSNote> taps;
		std::vector<SUSNote> directionals;
		std::vector<std::vector<SUSNote>> slides;
		std::vector<BPM> bpms;
		std::vector<BarLength> barlengths;
	};

	SUSMetadata processSUSMetadata(const std::unordered_map<std::string, std::string>& data);
	SUS processSusScore(const std::vector<std::string>& score, const std::unordered_map<std::string, std::string>& metadata);
	SUS loadSUS(const std::string& filename);
	void saveSUS(const SUS& sus, const std::string& filename);
}