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

namespace MikuMikuWorld
{
	constexpr float averageTwoF32Samples(float a, float b)
	{
		return (a + b) / 2;
	}

	class WaveformMip
	{
	public:
		size_t powerOfTwoSampleCount{};
		double secondsPerSample{};
		double samplesPerSecond{};
		std::vector<float> absoluteSamples;

		double getDuration() const
		{
			return static_cast<double>(absoluteSamples.size()) / samplesPerSecond;
		}

		float getSampleAtIndex(size_t index) const
		{
			if (index >= absoluteSamples.size())
				return 0;

			return absoluteSamples[index];
		}

		float linearSampleAtSeconds(double seconds) const
		{
			const float sampleIndexAsFloat = static_cast<float>(seconds * samplesPerSecond);
			const float sampleIndexFraction = sampleIndexAsFloat - static_cast<int32_t>(sampleIndexAsFloat);

			const size_t sampleIndexLo = std::max(0ull, static_cast<size_t>(sampleIndexAsFloat));
			const size_t sampleIndexHi = sampleIndexLo + 1;

			return lerp(getSampleAtIndex(sampleIndexLo), getSampleAtIndex(sampleIndexHi), sampleIndexFraction);
		}

		float averageNormalizedSampleInTimeRange(double startTime, double endTime) const
		{
			if (secondsPerSample <= 0)
			{
				assert(false);
				return 0.0f;
			}

			float sampleSum = 0;
			int sampleCount = 0;
			for (double t = startTime; t < endTime; t += secondsPerSample)
			{
				sampleSum += linearSampleAtSeconds(t);
				sampleCount++;
			}

			return sampleSum / static_cast<float>(std::max(sampleCount, 1));
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
	public:
		static constexpr size_t maxMipLevels{ 24 };
		static constexpr size_t minMipSamples{ 256 };

		WaveformMip mips[maxMipLevels]{};
		double durationInSeconds{};

		bool isEmpty() const
		{
			return mips[0].powerOfTwoSampleCount == 0;
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

		void generateMipChainsFromSampleBuffer(const AudioData& audioData, uint32_t channelIndex)
		{
			if (!audioData.isValid())
			{
				durationInSeconds = 0;
				clear();

				return;
			}

			durationInSeconds = static_cast<float>(audioData.frameCount) / static_cast<float>(audioData.sampleRate);
			
			if (mips[0].powerOfTwoSampleCount != 0)
				for (auto& mip : mips) mip.clear();

			WaveformMip& baseMip = mips[0];
			baseMip.powerOfTwoSampleCount = roundUpToPowerOfTwo(audioData.frameCount);
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
				float sampleA = audioData.sampleBuffer[((frameIndex * 2 + 0) * audioData.channelCount) + channelIndex];
				float sampleB = audioData.sampleBuffer[((frameIndex * 2 + 1) * audioData.channelCount) + channelIndex];
				baseMip.absoluteSamples[frameIndex] = averageTwoF32Samples(abs(sampleA), abs(sampleB));
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
				const float* parentSamples = parentMip.absoluteSamples.data();

				WaveformMip& currentMip = mips[i];
				currentMip.absoluteSamples.resize(currentMip.powerOfTwoSampleCount);
				const size_t samplesToFill = std::min(currentMip.absoluteSamples.size(), parentSampleCount / 2);

				for (size_t index = 0; index < samplesToFill; index++)
				{
					currentMip.absoluteSamples[index] = averageTwoF32Samples(parentSamples[0], parentSamples[1]);
					parentSamples += 2;
				}
			}
		}
	};
}