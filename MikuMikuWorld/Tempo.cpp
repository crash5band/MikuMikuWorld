#include "Tempo.h"

namespace MikuMikuWorld
{
	Tempo::Tempo()
		: tick{ 0 }, bpm{ 160 }
	{
	}

	Tempo::Tempo(int _tick, float _bpm)
		: tick{ _tick }, bpm{ _bpm }
	{
	}

	int beatsPerMeasure(const TimeSignature& t)
	{
		return (float)t.numerator / (float)t.denominator * 4;
	}

	double ticksToSec(int ticks, int beatTicks, float bpm)
	{
		return ticks * (60.0f / bpm / (float)beatTicks);
	}

	int secsToTicks(double secs, int beatTicks, float bpm)
	{
		return secs / (60.0f / bpm / (float)beatTicks);
	}

	double accumulateDuration(int tick, int beatTicks, const std::vector<Tempo>& bpms)
	{
		double total = 0;
		int accTicks = 0;
		int lastBPM = 0;

		for (int i = 0; i < bpms.size() - 1; ++i)
		{
			lastBPM = i;
			int ticks = bpms[i + 1].tick - bpms[i].tick;
			if (accTicks + ticks >= tick)
				break;

			accTicks += ticks;
			total += ticksToSec(bpms[i + 1].tick - bpms[i].tick, beatTicks, bpms[i].bpm);
			lastBPM = i + 1;
		}

		total += ticksToSec(tick - bpms[lastBPM].tick, beatTicks, bpms[lastBPM].bpm);

		return total;
	}

	int accumulateTicks(double sec, int beatTicks, const std::vector<Tempo>& bpms)
	{
		int total = 0;
		float accSecs = 0;
		int lastBPM = 0;

		for (int i = 0; i < bpms.size() - 1; ++i)
		{
			lastBPM = i;
			float seconds = ticksToSec(bpms[i + 1].tick - bpms[i].tick, beatTicks, bpms[i].bpm);
			if (accSecs + seconds >= sec)
				break;

			total += secsToTicks(seconds, beatTicks, bpms[i].bpm);
			accSecs += seconds;

			// last tempo is included
			lastBPM = i + 1;
		}

		total += secsToTicks(sec - accSecs, beatTicks, bpms[lastBPM].bpm);

		return total;
	}

	int accumulateMeasures(int tick, int beatTicks, const std::map<int, TimeSignature>& ts)
	{
		std::vector<TimeSignature> signatures;
		signatures.reserve(ts.size());
		for (const auto& it : ts)
			signatures.push_back(it.second);

		int accTicks = 0;
		int total = 0;
		int lastSign = 0;

		for (int t = 0; t < signatures.size() - 1; ++t)
		{
			lastSign = t;
			int ticksPerMeasure = signatures[t].numerator * beatTicks;
			int measures = signatures[t + 1].measure - signatures[t].measure;
			int ticks = measures * ticksPerMeasure;

			if (accTicks + ticks >= tick)
				break;

			total += measures;
			accTicks += ticks;
			lastSign = t + 1;
		}

		total += (tick - accTicks) / (signatures[lastSign].numerator * beatTicks);
		return total;
	}

	int measureToTicks(int measure, int beatTicks, const std::map<int, TimeSignature>& ts)
	{
		std::vector<TimeSignature> signatures;
		signatures.reserve(ts.size());
		for (const auto& it : ts)
			signatures.push_back(it.second);

		int accMeasures = 0;
		int total = 0;
		int lastSign = 0;

		for (int t = 0; t < signatures.size() - 1; ++t)
		{
			lastSign = t;
			int ticksPerMeasure = signatures[t].numerator * beatTicks;
			int measures = signatures[t + 1].measure - signatures[t].measure;
			int ticks = measures * ticksPerMeasure;

			if (accMeasures + measures >= measure)
				break;

			total += ticks;
			accMeasures += measures;
			lastSign = t + 1;
		}

		total += (measure - signatures[lastSign].measure) * (signatures[lastSign].numerator * beatTicks);
		return total;
 	}

	int findTimeSignature(int measure, const std::map<int, TimeSignature>& ts)
	{
		for (auto it = ts.rbegin(); it != ts.rend(); ++it)
		{
			if (it->second.measure <= measure)
				return it->second.measure;
		}

		return 0;
	}
}