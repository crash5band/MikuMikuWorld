#pragma once

/*
	miniaudio includes windows.h without defining NOMINMAX and even though it is defined
	elsewhere and compiles properly, visual studio is confused with std::min/std::max calls
*/
#define NOMINMAX
#include <miniaudio.h>
#include <string>

namespace MikuMikuWorld
{
	class SoundSource
	{
	private:
		ma_sound source;

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
