#pragma once
#include <array>
#include <string>
#include <memory>
#include <map>
#include <string_view>

// Already defined somewhere else but Visual Studio gets confused
#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>
#include <miniaudio.h>
#include "../Utilities.h"

namespace Audio
{
	constexpr size_t soundEffectsProfileCount = 2;

	constexpr const char* soundEffectsProfileNames[]
	{
		"SE 01",
		"SE 02"
	};

	enum SoundFlags : uint8_t
	{
		NONE		= 0,
		LOOP		= 1 << 0,
		EXTENDABLE	= 1 << 1
	};

	DECLARE_ENUM_FLAG_OPERATORS(SoundFlags)

	constexpr ma_uint32 maSoundFlagsDefault = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION;
	constexpr ma_uint32 maSoundFlagsDecodeAsync =
		MA_SOUND_FLAG_NO_PITCH |
		MA_SOUND_FLAG_NO_SPATIALIZATION |
		MA_SOUND_FLAG_DECODE |
		MA_SOUND_FLAG_ASYNC;

	struct SoundBuffer
	{
		std::string name;
		std::unique_ptr<int16_t[]> samples;
		ma_format sampleFormat{ ma_format_unknown };
		ma_uint32 sampleRate{};
		ma_uint32 channelCount{};
		ma_uint64 frameCount{};
		ma_audio_buffer buffer;

		ma_uint32 effectiveSampleRate;

		void initialize(const std::string &name, ma_uint32 sampleRate, ma_uint32 channelCount, ma_uint64 frameCount, int16_t* samples);
		void dispose();

		bool isValid() const { return samples.get() != nullptr && sampleRate > 0 && frameCount > 0; }
	};

	constexpr std::array<std::string_view, 4> supportedFileFormats =
	{
		".mp3", ".wav", ".flac", ".ogg"
	};

	MikuMikuWorld::Result decodeAudioFile(std::string filename, SoundBuffer& sound);
	bool isSupportedFileFormat(const std::string_view& fileExtension);

	struct SoundInstance
	{
		std::string name;
		ma_sound source;
		float lastStartTime{};
		float lastEndTime{};

		// The absolute start time in seconds
		float absoluteStart{};

		// The absolute end time in seconds
		float absoluteEnd{};

		inline void play() { ma_sound_start(&source); }
		inline void stop() { ma_sound_stop(&source); }
		inline void seek(uint64_t frame) { ma_sound_seek_to_pcm_frame(&source, frame); }
		inline bool isPlaying() const { return ma_sound_is_playing(&source); }
		
		uint64_t getCurrentFrame()
		{
			uint64_t frame{};
			ma_sound_get_cursor_in_pcm_frames(&source, &frame);
			return frame;
		}

		uint64_t getLengthInFrames()
		{
			uint64_t frame{};
			ma_sound_get_length_in_pcm_frames(&source, &frame);
			return frame;
		}

		float getCurrentTime()
		{
			float time{};
			ma_sound_get_cursor_in_seconds(&source, &time);
			return time;
		}

		float getDuration()
		{
			float time{};
			ma_sound_get_length_in_seconds(&source, &time);
			return time;
		}

		void extendDuration(float currentTime, float newAbsoluteEnd, float timeScale)
		{
			const float engineTime = static_cast<float>(ma_engine_get_time_in_milliseconds(source.engineNode.pEngine)) / 1000.0f;
			const float duration = newAbsoluteEnd - absoluteStart;
			const float instanceTime = currentTime - absoluteStart;
			const float newEndTime = ((duration - instanceTime) / timeScale) + engineTime;

			absoluteEnd = newAbsoluteEnd;
			lastEndTime = newEndTime;
			ma_sound_set_stop_time_in_milliseconds(&source, newEndTime * 1000);
		}
	};

	class SoundPool
	{
	public:
		static constexpr int poolSize{ 24 };
		std::array<SoundInstance, poolSize> pool;
		SoundFlags flags{};
	
		ma_uint64 getDurationInFrames() const;
		float getDurationInSeconds() const;

		void setLoopTime(ma_uint64 startFrames, ma_uint64 endFrames);
		bool isLooping() const;

		void setVolume(float volume);
		float getVolume() const;
		
		void extendInstanceDuration(SoundInstance& instance, float newEndTime);
		void play(float start, float end);
		void stopAll();

		bool isPlaying(const SoundInstance& soundInstance) const;
		bool isAnyPlaying() const;
		
		void initialize(const std::string& name, const std::string& path, ma_engine* engine, ma_sound_group* group, SoundFlags flags);
		void initialize(const std::string& path, ma_engine* engine, ma_sound_group* group, SoundFlags flags);
		void initialize(SoundBuffer& sound, ma_engine* engine, ma_sound_group* group, SoundFlags flags);
		void dispose();

		std::string getName() const { return name; }
		int getCurrentIndex() const { return currentIndex; }

	private:
		float volume{ 1.0f };
		int currentIndex{ 0 };

		std::string name{};
	};

	struct SoundEffectProfile
	{
		std::string name;
		std::map<std::string_view, std::unique_ptr<SoundPool>> pool;
	};
}