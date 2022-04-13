#pragma once
#include <miniaudio.h>

namespace MikuMikuWorld
{
	class AudioEvent
	{
	private:
		ma_sound sound;
		ma_uint64 durationInFrames;
		float duration;
		float start;
		float end;
		bool loop;

	public:
		AudioEvent(ma_engine* engine, ma_sound_group* group, ma_sound* sound, float start, bool loop, float end = -1);

		void play();
		void stop();
		void dispose();
	};
}