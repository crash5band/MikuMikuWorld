#pragma once
#include <miniaudio.h>
#include <string>

namespace MikuMikuWorld
{
	enum SoundFlags : uint8_t
	{
		None,
		Loop,
	};

	class SoundSource
	{
	private:
		ma_sound source;
		SoundFlags flags;

	public:
		SoundSource();

		void play();
		void stop();
		void reset();
		void setStart(float start);
		void setEnd(float end);
		void setLoop(bool val);
		void setLoopTime(ma_uint64 s, ma_uint64 e);
		void init(const std::wstring& filename, ma_engine* engine, ma_sound_group* group, ma_uint32 mFlags, bool loop);
		void dispose();

		ma_uint64 getPosition();
		ma_uint64 getDurationInFrames();
		float getDurationInSeconds();
		bool isAtEnd();
		bool isPlaying();
	};
}