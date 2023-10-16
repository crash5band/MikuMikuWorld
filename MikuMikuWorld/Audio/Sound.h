#pragma once
#include <array>
#include <string>

// Already defined somewhere else but Visual Studio gets confused
#define NOMINMAX
#include "miniaudio.h"
#include "../Utilities.h"

namespace Audio
{
	enum SoundFlags : uint8_t
	{
		NONE		= 0,
		LOOP		= 1 << 0,
		EXTENDABLE	= 1 << 1
	};

	DECLARE_ENUM_FLAG_OPERATORS(SoundFlags)

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
		
		void initialize(const std::string& path, ma_engine* engine, ma_sound_group* group, SoundFlags flags);
		void dispose();

	private:
		float volume;
		int nextIndex;
	};
}