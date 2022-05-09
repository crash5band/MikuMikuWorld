#include <Windows.h>
#include "../Application.h"
#include "../StringOperations.h"
#include "../UI.h"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include <filesystem>
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
		audioEvents.reserve(2000);
		loadSE();
	}

	void AudioManager::loadSE()
	{
		std::string path{ Application::getAppDir() + "res/sound/" };
		ma_uint32 flags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC;

		ma_sound perfect;
		ma_sound great;
		ma_sound good;
		ma_sound flick;
		ma_sound connect;
		ma_sound tick;
		ma_sound critical_tap;
		ma_sound critical_flick;
		ma_sound critical_connect;
		ma_sound critical_tick;

		seMap.insert(std::pair(SE_PERFECT, perfect));
		seMap.insert(std::pair(SE_GREAT, great));
		seMap.insert(std::pair(SE_GOOD, good));
		seMap.insert(std::pair(SE_FLICK, flick));
		seMap.insert(std::pair(SE_CONNECT, connect));
		seMap.insert(std::pair(SE_TICK, tick));
		seMap.insert(std::pair(SE_CRITICAL_TAP, critical_tap));
		seMap.insert(std::pair(SE_CRITICAL_FLICK, critical_flick));
		seMap.insert(std::pair(SE_CRITICAL_CONNECT, critical_connect));
		seMap.insert(std::pair(SE_CRITICAL_TICK, critical_tick));

		for (auto& it : seMap)
		{
			std::wstring filename = mbToWideStr(path + it.first + ".mp3");
			ma_sound_init_from_file_w(&engine, filename.c_str(), flags, NULL, NULL, &it.second);
		}
	}

	void AudioManager::uninitAudio()
	{
		if (musicInitialized)
			ma_sound_uninit(&bgm);

		clearEvents();
		for (auto& it : seMap)
			ma_sound_uninit(&it.second);

		ma_engine_uninit(&engine);
	}

	bool AudioManager::changeBGM(const std::string& filename)
	{
		disposeBGM();

		std::wstring wFilename = mbToWideStr(filename);
		if (!std::filesystem::exists(wFilename))
			return false;

		ma_uint32 flags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_DECODE;
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

		float time = (getEngineAbsTime() + bgmOffset);
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

		if (time > 0.0f)
			ma_sound_set_start_time_in_milliseconds(&bgm, time * 1000);
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

	void AudioManager::pushAudioEvent(const char* se, double start, double end, bool loop)
	{
		if (seMap.find(se) == seMap.end())
			return;

		if (!seMap[se].pDataSource)
			return;

		audioEvents.push_back(std::make_unique<AudioEvent>(&engine, &seGroup, &seMap[se], start, loop, end));
		audioEvents[audioEvents.size() - 1]->play();
	}

	void AudioManager::clearEvents()
	{
		for (auto& event : audioEvents)
		{
			event->stop();
			event->dispose();
		}

		audioEvents.clear();
	}

	void AudioManager::stopAllEvents()
	{
		for (auto& event : audioEvents)
			event->stop();
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

	void AudioManager::playSE(const char* se, float time)
	{
		std::string path = Application::getAppDir() + "res/sound/" + se + ".mp3";
		if (seMap.find(se) != seMap.end())
			ma_engine_play_sound(&engine, path.c_str(), &seGroup);
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
