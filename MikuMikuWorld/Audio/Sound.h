#pragma once
#include <array>
#include <string>
#include "SoundSource.h"

namespace MikuMikuWorld
{
	class Sound
	{
	private:
		std::array<SoundSource, 32> sources;
		int next;

	public:
		Sound(const std::string& path, ma_engine* engine, ma_sound_group* group, bool loop);
		Sound();

		void init(const std::string& path, ma_engine* engine, ma_sound_group* group, bool loop);
		void dispose();
		void playSound(float start, float end);
		void setLooptime(ma_uint64 s, ma_uint64 e);
		void stopAll();

		ma_uint64 getDurationInFrames();
		float getDurectionInSeconds();
		bool isAnyPlaying();
	};
}