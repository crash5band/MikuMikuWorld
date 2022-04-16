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

	float beatsPerMeasure(const TimeSignature& t);
	float ticksToSec(int ticks, int beatTicks, float bpm);
	int secsToTicks(float secs, int beatTicks, float bpm);
	float accumulateDuration(int tick, int beatTicks, const std::vector<Tempo>& tempos);
	int accumulateTicks(float sec, int beatTicks, const std::vector<Tempo>& tempos);
	int accumulateMeasures(int ticks, int beatTicks, const std::map<int, TimeSignature>& ts);
	int measureToTicks(int measure, int beatTicks, const std::map<int, TimeSignature>& ts);
	int findTimeSignature(int measure, const std::map<int, TimeSignature>& ts);
}