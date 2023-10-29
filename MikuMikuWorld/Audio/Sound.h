#pragma once
#include <array>
#include <memory>
#include <string>

// Already defined somewhere else but Visual Studio gets confused
#define NOMINMAX
#define STB_VORBIS_HEADER_ONLY
#include "../Utilities.h"
#include <miniaudio.h>
#include <stb_vorbis.c>

namespace Audio
{
	enum SoundFlags : uint8_t
	{
		NONE = 0,
		LOOP = 1 << 0,
		EXTENDABLE = 1 << 1
	};

	DECLARE_ENUM_FLAG_OPERATORS(SoundFlags)

	struct Sound
	{
		std::string name;
		std::unique_ptr<int16_t[]> samples;
		ma_format sampleFormat{ ma_format_unknown };
		ma_uint32 sampleRate{};
		ma_uint32 channelCount{};
		ma_uint64 frameCount{};
		ma_audio_buffer buffer;

		void initialize(std::string name, ma_uint32 sampleRate, ma_uint32 channelCount,
		                ma_uint64 frameCount, int16_t* samples);
		void dispose();

		bool isValid() const
		{
			return samples.get() != nullptr && sampleRate > 0 && frameCount > 0;
		}
	};

	constexpr std::array<std::string_view, 4> supportedFileFormats = { ".mp3", ".wav", ".flac",
		                                                               ".ogg" };

	MikuMikuWorld::Result decodeAudioFile(std::string filename, Sound& sound);
	bool isSupportedFileFormat(const std::string_view& fileExtension);

	struct SoundInstance
	{
		ma_sound source;
		float lastStartTime{};
		float lastEndTime{};
	};

	class SoundPool
	{
	  public:
		static constexpr int poolSize{ 32 };
		std::array<SoundInstance, poolSize> pool;
		SoundFlags flags{};

		ma_uint64 getDurationInFrames() const;

		void setLoopTime(ma_uint64 startFrames, ma_uint64 endFrames);
		bool isLooping() const;

		void setVolume(float volume);
		float getVolume() const;

		void extendInstanceDuration(SoundInstance& instance, float newEndTime);
		void play(float start, float end);
		void stopAll();

		bool isPlaying(const SoundInstance& soundInstance) const;
		bool isAnyPlaying() const;

		void initialize(const std::string& path, ma_engine* engine, ma_sound_group* group,
		                SoundFlags flags);
		void dispose();

	  private:
		float volume;
		int nextIndex;
	};
}