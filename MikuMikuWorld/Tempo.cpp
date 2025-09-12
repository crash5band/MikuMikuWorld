#include "Tempo.h"
#include "Score.h"
#include "Constants.h"
#include <algorithm>

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

	double accumulateScaledDuration(int tick, int ticksPerBeat, const std::vector<Tempo>& bpms, const std::vector<HiSpeedChange>& hispeeds)
	{
		int prvBpm = 0, prvSpd = -1;
		int accTicks = 0;
		double totalDuration = 0;

		while (accTicks < tick)
		{
			int nxtBpmTick = prvBpm + 1 < bpms.size() ? bpms[prvBpm + 1].tick : INT32_MAX;
			int nxtSpdTick = prvSpd + 1 < hispeeds.size() ? hispeeds[prvSpd + 1].tick : INT32_MAX;
			int nxtTick = std::min({nxtBpmTick, nxtSpdTick, tick});

			float currentBpm = bpms.at(prvBpm).bpm;
			float currentSpd = prvSpd >= 0 ? hispeeds[prvSpd].speed : 1.0f;

			totalDuration += ticksToSec(nxtTick - accTicks, ticksPerBeat, currentBpm) * currentSpd;

			if (nxtTick == nxtBpmTick)
				prvBpm++;
			if (nxtTick == nxtSpdTick)
				prvSpd++;
			accTicks = nxtTick;
		}
		return totalDuration;
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
		int accTicks = 0;
		int total = 0;
		auto lastSignature = ts.begin();

		for (auto t = ts.begin(); t != std::prev(ts.end()); t = std::next(t))
		{
			lastSignature = t;
			int ticksPerMeasure = beatsPerMeasure(t->second) * beatTicks;
			int measures = std::next(t)->first - t->first;
			int ticks = measures * ticksPerMeasure;

			if (accTicks + ticks >= tick)
				break;

			total += measures;
			accTicks += ticks;
			lastSignature = std::next(t);
		}

		total += (tick - accTicks) / (beatsPerMeasure(lastSignature->second) * beatTicks);
		return total;
	}

	int measureToTicks(int measure, int beatTicks, const std::map<int, TimeSignature>& ts)
	{
		int accMeasures = 0;
		int total = 0;
		auto lastSignature = ts.begin();

		for (auto t = ts.begin(); t != std::prev(ts.end()); t = std::next(t))
		{
			lastSignature = t;
			int ticksPerMeasure = beatsPerMeasure(t->second) * beatTicks;
			int measures = std::next(t)->first - t->first;
			int ticks = measures * ticksPerMeasure;

			if (accMeasures + measures >= measure)
				break;

			total += ticks;
			accMeasures += measures;
			lastSignature = std::next(t);
		}

		total += (measure - lastSignature->first) * (beatsPerMeasure(lastSignature->second) * beatTicks);
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

	int snapTick(int tick, int div)
	{
		const int subDivision = TICKS_PER_BEAT / (static_cast<float>(div) / 4);
		const int remainingTicks = tick % subDivision;

		tick -= remainingTicks;
		if (remainingTicks >= subDivision / 2)
			tick += subDivision;

		return std::max(tick, 0);
	}
}