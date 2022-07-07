#include "Sound.h"
#include "../StringOperations.h"

namespace MikuMikuWorld
{
	ma_uint32 flags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC;

	Sound::Sound() : next{ 0 }
	{

	}

	Sound::Sound(const std::string& path, ma_engine* engine, ma_sound_group* group, bool loop)
	{
		init(path, engine, group, loop);
	}

	void Sound::playSound(float start, float end)
	{
		sources[next].reset();
		sources[next].setStart(start);
		sources[next].setEnd(end);
		sources[next].play();

		next = ++next % sources.size();
	}

	void Sound::init(const std::string& path, ma_engine* engine, ma_sound_group* group, bool loop)
	{
		std::wstring wPath = mbToWideStr(path);
		for (int i = 0; i < sources.size(); ++i)
		{
			sources[i] = SoundSource();
			sources[i].init(wPath, engine, group, flags, loop);
		}

		next = 0;
	}

	void Sound::dispose()
	{
		for (auto& src : sources)
			src.dispose();
	}

	void Sound::stopAll()
	{
		for (auto& src : sources)
			src.stop();
	}

	void Sound::setLooptime(ma_uint64 s, ma_uint64 e)
	{
		for (auto& src : sources)
			src.setLoopTime(s, e);
	}

	ma_uint64 Sound::getDurationInFrames()
	{
		return sources[0].getDurationInFrames();
	}

	float Sound::getDurectionInSeconds()
	{
		return sources[0].getDurationInSeconds();
	}

	bool Sound::isAnyPlaying()
	{
		for (auto& src : sources)
			if (src.isPlaying())
				return true;

		return false;
	}
}
