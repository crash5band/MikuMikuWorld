#include <Windows.h>
#include "../Application.h"
#include "../StringOperations.h"
#include "../UI.h"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include <filesystem>
#include <execution>
#include "AudioManager.h"

#undef STB_VORBIS_HEADER_ONLY

namespace MikuMikuWorld
{
	void AudioManager::initAudio()
	{
		std::string err = "";
		ma_result result = MA_SUCCESS;
		try
		{
			result = ma_engine_init(NULL, &engine);
			if (result != MA_SUCCESS)
			{
				err = "FATAL: Failed to start audio engine. Aborting.\n";
				throw(result);
			}

			result = ma_sound_group_init(&engine, 0, NULL, &bgmGroup);
			if (result != MA_SUCCESS)
			{
				err = "Failed to initialize BGM audio group.\n";
				throw(result);
			}

			result = ma_sound_group_init(&engine, 0, NULL, &seGroup);
			if (result != MA_SUCCESS)
			{
				err = "Failed to initialize SE audio group.\n";
				throw(result);
			}
		}
		catch (int)
		{
			err.append(ma_result_description(result));
			MessageBox(NULL, err.c_str(), APP_NAME, MB_OK | MB_ICONERROR);

			exit(result);
		}

		musicInitialized = false;
		loadSE();
	}

	void AudioManager::loadSE()
	{
		std::string path{ Application::getAppDir() + "res/sound/" };
		for (int i = 0; i < 10; ++i)
			sounds.emplace(std::move(std::pair<std::string, std::unique_ptr<Sound>>(SE_NAMES[i], std::make_unique<Sound>())));

		std::for_each(std::execution::par, sounds.begin(), sounds.end(), [&](auto& s) {
			std::string filename = path + s.first + ".mp3";
			s.second->init(filename, &engine, &seGroup, s.first == SE_CONNECT || s.first == SE_CRITICAL_CONNECT);
		});
		
		// adjust hold SE loop times for gapless playback
		ma_uint64 holdNrmDuration = sounds[SE_CONNECT]->getDurationInFrames();
		ma_uint64 holdCrtDuration = sounds[SE_CRITICAL_CONNECT]->getDurationInFrames();
		sounds[SE_CONNECT]->setLooptime(3000, holdNrmDuration - 3000);
		sounds[SE_CRITICAL_CONNECT]->setLooptime(3000, holdCrtDuration - 3000);
	}

	void AudioManager::uninitAudio()
	{
		if (musicInitialized)
			ma_sound_uninit(&bgm);

		ma_engine_uninit(&engine);
	}

	bool AudioManager::changeBGM(const std::string& filename)
	{
		disposeBGM();

		std::wstring wFilename = mbToWideStr(filename);
		if (!std::filesystem::exists(wFilename))
			return false;

		ma_uint32 flags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC;
		ma_result bgmResult = ma_sound_init_from_file_w(&engine, wFilename.c_str(), flags, &bgmGroup, NULL, &bgm);
		if (bgmResult != MA_SUCCESS)
		{
			musicInitialized = false;
			printf("Failed to initialize audio from file %ws", wFilename.c_str());
		}
		else
		{
			musicInitialized = true;
		}

		return musicInitialized;
	}

	void AudioManager::playBGM(float currTime)
	{
		if (!musicInitialized)
			return;

		float time = bgmOffset;
		time -= currTime;

		ma_uint64 length = 0;
		ma_result lengthResult = ma_sound_get_length_in_pcm_frames(&bgm, &length);
		if (lengthResult != MA_SUCCESS)
		{
			printf("-ERROR- AudioManager::playBGM(): Failed to get length in pcm frames");
			return;
		}

		if (time * engine.sampleRate * -1 > length)
			return;

		ma_sound_set_start_time_in_milliseconds(&bgm, std::max(0.0f, time * 1000));
		ma_sound_start(&bgm);
	}

	void AudioManager::pauseBGM()
	{
		ma_sound_stop(&bgm);
	}

	void AudioManager::stopBGM()
	{
		ma_sound_stop(&bgm);
		ma_sound_seek_to_pcm_frame(&bgm, 0);
	}

	void AudioManager::setBGMOffset(float time, float msec)
	{
		bgmOffset = msec / 1000.0f;
		float pos = time - bgmOffset;
		ma_sound_seek_to_pcm_frame(&bgm, pos * engine.sampleRate);

		float start = (getEngineAbsTime() + bgmOffset);
		start -= time;

		ma_sound_set_start_time_in_milliseconds(&bgm, std::max(0.0f, start * 1000));
	}

	float AudioManager::getAudioPosition()
	{
		float cursor;
		ma_sound_get_cursor_in_seconds(&bgm, &cursor);

		return cursor;
	}

	void AudioManager::disposeBGM()
	{
		if (musicInitialized)
		{
			ma_sound_stop(&bgm);
			ma_sound_uninit(&bgm);
			musicInitialized = false;
		}
	}

	void AudioManager::seekBGM(float time)
	{
		ma_uint64 seekFrame = (time - bgmOffset) * engine.sampleRate;
		ma_sound_seek_to_pcm_frame(&bgm, seekFrame);

		ma_uint64 length = 0;
		ma_result lengthResult = ma_sound_get_length_in_pcm_frames(&bgm, &length);
		if (lengthResult != MA_SUCCESS)
			return;

		if (ma_sound_at_end(&bgm) && seekFrame < length)
			bgm.atEnd = false;
	}

	void AudioManager::setMasterVolume(float volume)
	{
		ma_engine_set_volume(&engine, volume);
	}

	void AudioManager::setBGMVolume(float volume)
	{
		ma_sound_group_set_volume(&bgmGroup, volume);
	}

	void AudioManager::setSEVolume(float volume)
	{
		ma_sound_group_set_volume(&seGroup, volume);
	}

	void AudioManager::playSound(const char* se, double start, double end)
	{
		if (sounds.find(se) == sounds.end())
			return;

		sounds[se]->playSound(start, end);
	}

	void AudioManager::stopSounds(bool all)
	{
		if (all)
		{
			for (auto& [se, sound] : sounds)
				sound->stopAll();
		}
		else
		{
			sounds[SE_CONNECT]->stopAll();
			sounds[SE_CRITICAL_CONNECT]->stopAll();
		}
	}

	float AudioManager::getEngineAbsTime()
	{
		return ((float)ma_engine_get_time(&engine) / (float)engine.sampleRate) / 1000.0f;
	}

	float AudioManager::getBGMOffset()
	{
		return bgmOffset;
	}

	float AudioManager::getSongEndTime()
	{
		float length = 0.0f;
		ma_sound_get_length_in_seconds(&bgm, &length);

		return length + bgmOffset;
	}

	void AudioManager::reSync()
	{
		ma_engine_set_time(&engine, 0);
	}

	bool AudioManager::isMusicInitialized()
	{
		return musicInitialized;
	}

	bool AudioManager::isMusicAtEnd()
	{
		return ma_sound_at_end(&bgm);
	}
}
