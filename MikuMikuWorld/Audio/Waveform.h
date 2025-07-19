/*
	Mostly taken from https://github.com/samyuu/PeepoDrumKit/blob/master/src/audio/audio_waveform.h
	with modifications to fit the current audio engine
*/

#pragma once
#include "../Math.h"
#include "AudioManager.h"
#include <stdint.h>
#include <vector>
#include <limits>

namespace Audio
{
	constexpr int16_t averageTwoInt16Samples(int16_t a, int16_t b)
	{
		return static_cast<int16_t>((static_cast<int32_t>(a) + static_cast<int32_t>(b)) / 2);
	}

	constexpr int16_t int16_t_max = std::numeric_limits<int16_t>::max();

	class WaveformMip
	{
	public:
		size_t powerOfTwoSampleCount{};
		double secondsPerSample{};
		double samplesPerSecond{};
		std::vector<int16_t> absoluteSamples;

		double getDuration() const
		{
			return static_cast<double>(absoluteSamples.size()) / samplesPerSecond;
		}

		int16_t getSampleAtIndex(size_t index) const
		{
			if (index >= absoluteSamples.size())
				return 0;

			return absoluteSamples[index];
		}

		int16_t linearSampleAtSeconds(double seconds) const
		{
			const float sampleIndexAsFloat = static_cast<float>(seconds * samplesPerSecond);
			const float sampleIndexFraction = sampleIndexAsFloat - static_cast<int32_t>(sampleIndexAsFloat);

			const size_t sampleIndexLo = std::max(static_cast<size_t>(0ull), static_cast<size_t>(sampleIndexAsFloat));
			const size_t sampleIndexHi = sampleIndexLo + 1;

			if (sampleIndexLo >= absoluteSamples.size())
				return 0;

			const float normalizedSampleLo = getSampleAtIndex(sampleIndexLo) / static_cast<float>(int16_t_max);
			const float normalizedSampleHi = getSampleAtIndex(sampleIndexHi) / static_cast<float>(int16_t_max);
			const float normalizedSample = MikuMikuWorld::lerp(normalizedSampleLo, normalizedSampleHi, sampleIndexFraction);

			return static_cast<int16_t>(normalizedSample * static_cast<float>(int16_t_max));
		}

		float averageNormalizedSampleInTimeRange(double startTime, double endTime) const
		{
			if (absoluteSamples.empty())
				return 0.0f;

			if (secondsPerSample <= 0)
			{
				assert(false && "WaveformMip has samples but secondsPerSample <= 0?");
				return 0.0f;
			}

			int32_t sampleSum = 0, sampleCount = 0;
			for (double t = startTime; t < endTime; t += secondsPerSample)
			{
				sampleSum += linearSampleAtSeconds(t);
				sampleCount++;
			}

			const int32_t sampleAverage = sampleSum / std::max(sampleCount, 1);
			return sampleAverage / static_cast<float>(int16_t_max);
		}

		void clear()
		{
			powerOfTwoSampleCount = {};
			secondsPerSample = {};
			samplesPerSecond = {};
			absoluteSamples.clear();
		}
	};

	class WaveformMipChain
	{
	private:
		bool doneProcessing{ true };
		
	public:
		static constexpr size_t maxMipLevels{ 24 };
		static constexpr size_t minMipSamples{ 256 };

		WaveformMip mips[maxMipLevels]{};
		double durationInSeconds{};

		bool isEmpty() const
		{
			return mips[0].powerOfTwoSampleCount == 0;
		}

		bool isDoneProcessing() const
		{
			return doneProcessing;
		}

		void clear()
		{
			for (auto& mip : mips)
				mip.clear();
		}

		int getUsedMipCount() const
		{
			for (int i = 0; i < static_cast<int>(maxMipLevels); i++)
			{
				if (mips[i].powerOfTwoSampleCount == 0)
					return i;
			}

			return static_cast<int32_t>(maxMipLevels);
		}

		const WaveformMip& findClosestMip(double secondsPerPixel) const
		{
			const WaveformMip* closestMip = &mips[0];
			for (int i = 1; i < maxMipLevels; i++)
			{
				if (mips[i].powerOfTwoSampleCount == 0)
					break;

				if (abs(mips[i].secondsPerSample - secondsPerPixel) < abs(closestMip->secondsPerSample - secondsPerPixel))
					closestMip = &mips[i];
			}

			return *closestMip;
		}

		float getAmplitudeAt(const WaveformMip& mip, double seconds, double secondsPerPixel) const
		{
			return mip.averageNormalizedSampleInTimeRange(seconds, seconds + secondsPerPixel);
		}

		void generateMipChainsFromSampleBuffer(const SoundBuffer& audioData, uint32_t channelIndex)
		{
			doneProcessing = false;
			if (!audioData.isValid())
			{
				durationInSeconds = 0;
				clear();

				doneProcessing = true;
				return;
			}

			durationInSeconds = static_cast<float>(audioData.frameCount) / static_cast<float>(audioData.sampleRate);
			
			if (mips[0].powerOfTwoSampleCount != 0)
				for (auto& mip : mips) mip.clear();

			WaveformMip& baseMip = mips[0];
			baseMip.powerOfTwoSampleCount = MikuMikuWorld::roundUpToPowerOfTwo(audioData.frameCount);
			baseMip.secondsPerSample = 1.0 / static_cast<double>(audioData.sampleRate);
			baseMip.samplesPerSecond = audioData.sampleRate;

			baseMip.powerOfTwoSampleCount /= 2;
			baseMip.secondsPerSample *= 2.0;
			baseMip.samplesPerSecond /= 2.0;
			baseMip.absoluteSamples.resize(baseMip.powerOfTwoSampleCount);
			const size_t samplesToFill = std::min(baseMip.absoluteSamples.size(), static_cast<size_t>(audioData.frameCount / 2));

			// The original full samples are already included in the sample buffer
			// So we'll skip processing the full data mip to reduce memory usage
			for (size_t frameIndex = 0; frameIndex < samplesToFill; frameIndex++)
			{
				int16_t sampleA = audioData.samples[((frameIndex * 2 + 0) * audioData.channelCount) + channelIndex];
				int16_t sampleB = audioData.samples[((frameIndex * 2 + 1) * audioData.channelCount) + channelIndex];
				baseMip.absoluteSamples[frameIndex] = averageTwoInt16Samples(abs(sampleA), abs(sampleB));
			}

			// First loop to calculate sample counts
			for (size_t i = 1; i < maxMipLevels; i++)
			{
				const WaveformMip& parentMip = mips[i - 1];
				if (parentMip.powerOfTwoSampleCount <= minMipSamples)
					break;

				WaveformMip& newMip = mips[i];
				newMip.powerOfTwoSampleCount = parentMip.powerOfTwoSampleCount / 2;
				newMip.secondsPerSample = parentMip.secondsPerSample * 2.0;
				newMip.samplesPerSecond = parentMip.samplesPerSecond / 2.0;
			}

			// Second loop to generate the acutal mip sample buffers
			for (size_t i = 1; i < maxMipLevels; i++)
			{
				const WaveformMip& parentMip = mips[i - 1];
				if (parentMip.powerOfTwoSampleCount == 0)
					break;

				const size_t parentSampleCount = parentMip.absoluteSamples.size();
				const int16_t* parentSamples = parentMip.absoluteSamples.data();

				WaveformMip& currentMip = mips[i];
				currentMip.absoluteSamples.resize(currentMip.powerOfTwoSampleCount);
				const size_t samplesToFill = std::min(currentMip.absoluteSamples.size(), parentSampleCount / 2);

				for (size_t index = 0; index < samplesToFill; index++)
				{
					currentMip.absoluteSamples[index] = averageTwoInt16Samples(parentSamples[0], parentSamples[1]);
					parentSamples += 2;
				}
			}

			doneProcessing = true;
		}
	};
}