#pragma once
#include <vector>
#include <map>

namespace MikuMikuWorld
{
	struct HiSpeedChange;

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

	int snapTick(int tick, int div);
	float beatsPerMeasure(const TimeSignature& t);

	float ticksToSec(int ticks, int beatTicks, float bpm);
	int secsToTicks(float secs, int beatTicks, float bpm);

	float accumulateDuration(int tick, int beatTicks, const std::vector<Tempo>& tempos);
	int accumulateTicks(float sec, int beatTicks, const std::vector<Tempo>& tempos);

	int accumulateMeasures(int ticks, int beatTicks, const std::map<int, TimeSignature>& ts);
	int measureToTicks(int measure, int beatTicks, const std::map<int, TimeSignature>& ts);

	double accumulateScaledDuration(int tick, int ticksPerBeat, const std::vector<Tempo>& bpms, const std::vector<HiSpeedChange>& hispeeds);

	const Tempo& getTempoAt(int tick, const std::vector<Tempo>& tempos);
	int findTimeSignature(int measure, const std::map<int, TimeSignature>& ts);
	int findHighSpeedChange(int tick, const std::vector<HiSpeedChange>& hiSpeeds);
}