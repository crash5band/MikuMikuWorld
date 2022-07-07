#include "Sound.h"
#include "../StringOperations.h"

namespace MikuMikuWorld
{
	ma_uint32 flags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC;

	Sound::Sound() : next{ -1 }
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
		ma_sound src;
		ma_result res = ma_sound_init_from_file_w(engine, wPath.c_str(), flags, group, NULL, &src);

		if (res == MA_SUCCESS)
		{
			for (int i = 0; i < sources.size(); ++i)
			{
				sources[i] = SoundSource();
				sources[i].init(engine, group, &src, flags, loop);
			}
		}
		else
		{
			printf("Sound::Sound(): -ERROR- Failed to initialize sound sources %s", ma_result_description(res));
		}
	}

	void Sound::dispose()
	{
		for (auto& sound : sources)
			sound.dispose();
	}

	void Sound::stopAll()
	{
		for (auto& sound : sources)
			sound.stop();
	}
}
