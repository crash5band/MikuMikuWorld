#include "Tempo.h"
#include "Score.h"
#include "Constants.h"

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

	float beatsPerMeasure(const TimeSignature& t)
	{
		return ((float)t.numerator / (float)t.denominator) * 4.0f;
	}

	float ticksToSec(int ticks, int beatTicks, float bpm)
	{
		return ticks * (60.0f / bpm / (float)beatTicks);
	}

	int secsToTicks(float secs, int beatTicks, float bpm)
	{
		return secs / (60.0f / bpm / (float)beatTicks);
	}

	float accumulateDuration(int tick, int beatTicks, const std::vector<Tempo>& bpms)
	{
		float total = 0;
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

	int accumulateTicks(float sec, int beatTicks, const std::vector<Tempo>& bpms)
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
			int ticksPerMeasure = beatsPerMeasure(signatures[t]) * beatTicks;
			int measures = signatures[t + 1].measure - signatures[t].measure;
			int ticks = measures * ticksPerMeasure;

			if (accTicks + ticks >= tick)
				break;

			total += measures;
			accTicks += ticks;
			lastSign = t + 1;
		}

		total += (tick - accTicks) / (beatsPerMeasure(signatures[lastSign]) * beatTicks);
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
			int ticksPerMeasure = beatsPerMeasure(signatures[t]) * beatTicks;
			int measures = signatures[t + 1].measure - signatures[t].measure;
			int ticks = measures * ticksPerMeasure;

			if (accMeasures + measures >= measure)
				break;

			total += ticks;
			accMeasures += measures;
			lastSign = t + 1;
		}

		total += (measure - signatures[lastSign].measure) * (beatsPerMeasure(signatures[lastSign]) * beatTicks);
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

	int findHighSpeedChange(int tick, const std::vector<HiSpeedChange>& hiSpeeds)
	{
		for (int i = hiSpeeds.size() - 1; i >= 0; --i)
		{
			if (hiSpeeds[i].tick <= tick)
				return i;
		}
		
		return -1;
	}

	const Tempo& getTempoAt(int tick, const std::vector<Tempo>& tempos)
	{
		for (auto it = tempos.rbegin(); it != tempos.rend(); ++it)
			if (it->tick <= tick)
				return *it;

		return tempos[0];
	}

	int snapTick(int tick, int div, const std::map<int, TimeSignature>& ts)
	{
		const TimeSignature& t = ts.at(findTimeSignature(accumulateMeasures(tick, TICKS_PER_BEAT, ts), ts));
		
		int subDiv = (beatsPerMeasure(t) * TICKS_PER_BEAT) / (((float)div / 4.0f) * t.numerator);
		int half = subDiv / 2;
		int remaining = tick % subDiv;

		// round to closest division
		tick -= remaining;
		if (remaining >= half)
			tick += half * 2;

		return std::max(tick, 0);
	}
}