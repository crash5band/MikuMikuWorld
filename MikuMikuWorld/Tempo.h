#pragma once
#include <vector>
#include <map>
#include "ScoreEvents.h"

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

	/// <summary>
	/// Calculates the number of beats per measure of a given time signature
	/// </summary>
	/// <param name="t">The time signature</param>
	/// <returns>The number of beats per measure</returns>
	float beatsPerMeasure(const TimeSignature& t);

	/// <summary>
	/// Converts the specified ticks to time in seconds
	/// </summary>
	/// <param name="ticks">The number of ticks to convert</param>
	/// <param name="beatTicks">The number of ticks per beat</param>
	/// <param name="bpm">The BPM of the tick range</param>
	/// <returns>The time in seconds</returns>
	float ticksToSec(int ticks, int beatTicks, float bpm);

	/// <summary>
	/// Converts the specified time in seconds to ticks
	/// </summary>
	/// <param name="secs">The number of seconds</param>
	/// <param name="beatTicks">The number of ticks per beat</param>
	/// <param name="bpm">The BPM of the time range</param>
	/// <returns>The number of ticks</returns>
	int secsToTicks(float secs, int beatTicks, float bpm);

	/// <summary>
	/// Calculates the total number of seconds up to the specified tick
	/// </summary>
	/// <param name="tick">The tick range</param>
	/// <param name="beatTicks">The number of ticks per beat</param>
	/// <param name="tempos">The list of BPMs for the specified tick range</param>
	/// <returns>Total number of seconds</returns>
	float accumulateDuration(int tick, int beatTicks, const std::vector<Tempo>& tempos);

	/// <summary>
	/// Calculates the total number of ticks up to the specified time in seconds
	/// </summary>
	/// <param name="sec">The time in seconds</param>
	/// <param name="beatTicks">The number of ticks per beat</param>
	/// <param name="tempos">The list of BPMs for the specified time range</param>
	/// <returns>The total number of ticks</returns>
	int accumulateTicks(float sec, int beatTicks, const std::vector<Tempo>& tempos);

	/// <summary>
	/// Calculates the total number of measures up to the specified tick
	/// </summary>
	/// <param name="ticks">The number of ticks</param>
	/// <param name="beatTicks">The number of ticks per beat</param>
	/// <param name="ts">The list of time signatures</param>
	/// <returns>The total number of measures</returns>
	int accumulateMeasures(int ticks, int beatTicks, const std::map<int, TimeSignature>& ts);

	/// <summary>
	/// Calculates the total number of ticks up to the specified measure
	/// </summary>
	/// <param name="measure">The measure number</param>
	/// <param name="beatTicks">The number of ticks per beat</param>
	/// <param name="ts">The list of time signatures</param>
	/// <returns>The total number of ticks</returns>
	int measureToTicks(int measure, int beatTicks, const std::map<int, TimeSignature>& ts);

	/// <summary>
	/// Finds the time signature of the specified measure
	/// </summary>
	/// <param name="measure">The measure number</param>
	/// <param name="ts">A list of time signatures to search</param>
	/// <returns>The measure of the time signature</returns>
	int findTimeSignature(int measure, const std::map<int, TimeSignature>& ts);


	/// <summary>
	/// Finds the BPM at the specified tick
	/// </summary>
	/// <param name="tick">The tick number</param>
	/// <param name="tempos">A list of BPMs to search</param>
	/// <returns>The tempo change at the specified tick</returns>
	const Tempo& getTempoAt(int tick, const std::vector<Tempo>& tempos);
}