#pragma once
#include <vector>
#include <map>

namespace MikuMikuWorld
{
	struct TimeSignature
	{
		int measure;
		int numerator;
		int denominator;
	};

	struct Tempo
	{
		int tick;
		float bpm;

		Tempo();
		Tempo(int tick, float bpm);
	};

	int beatsPerMeasure(const TimeSignature& t);
	double ticksToSec(int ticks, int beatTicks, float bpm);
	int secsToTicks(double secs, int beatTicks, float bpm);
	double accumulateDuration(int tick, int beatTicks, const std::vector<Tempo>& tempos);
	int accumulateTicks(double sec, int beatTicks, const std::vector<Tempo>& tempos);
	int accumulateMeasures(int ticks, int beatTicks, const std::map<int, TimeSignature>& ts);
	int measureToTicks(int measure, int beatTicks, const std::map<int, TimeSignature>& ts);
	int findTimeSignature(int measure, const std::map<int, TimeSignature>& ts);
}