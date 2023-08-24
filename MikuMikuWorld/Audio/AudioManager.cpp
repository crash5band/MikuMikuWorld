#include "../Application.h"
#include "../IO.h"
#include "../UI.h"

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include <filesystem>
#include <execution>
#include "AudioManager.h"
#include "../Constants.h"

#undef STB_VORBIS_HEADER_ONLY

namespace MikuMikuWorld
{
	void AudioManager::initAudio()
	{
		std::string err = "";
		ma_result result = MA_SUCCESS;
		ma_uint32 groupFlags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION;

		try
		{
			result = ma_engine_init(NULL, &engine);
			if (result != MA_SUCCESS)
			{
				err = "FATAL: Failed to start audio engine. Aborting.\n";
				throw(result);
			}

			result = ma_sound_group_init(&engine, groupFlags, NULL, &bgmGroup);
			if (result != MA_SUCCESS)
			{
				err = "Failed to initialize BGM audio group.\n";
				throw(result);
			}

			result = ma_sound_group_init(&engine, groupFlags, NULL, &seGroup);
			if (result != MA_SUCCESS)
			{
				err = "Failed to initialize SE audio group.\n";
				throw(result);
			}
		}
		catch (ma_result)
		{
			err.append(ma_result_description(result));
			IO::messageBox(APP_NAME, err, IO::MessageBoxButtons::Ok, IO::MessageBoxIcon::Error);

			exit(result);
		}

		musicInitialized = false;
		loadSE();

		setMasterVolume(1.0f);
		setBGMVolume(1.0f);
		setSEVolume(1.0f);
	}

	void AudioManager::loadSE()
	{
		std::string path{ Application::getAppDir() + "res/sound/" };
		for (int i = 0; i < sizeof(SE_NAMES) / sizeof(const char*); ++i)
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

		std::wstring wFilename = IO::mbToWideStr(filename);
		if (!std::filesystem::exists(wFilename))
			return false;

		ma_uint32 flags = MA_SOUND_FLAG_NO_PITCH
			| MA_SOUND_FLAG_NO_SPATIALIZATION
			| MA_SOUND_FLAG_DECODE
			| MA_SOUND_FLAG_ASYNC;

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

		if (seekFrame > length)
		{
			// seeking beyond the sound's length
			bgm.atEnd = true;
		}
		else if (ma_sound_at_end(&bgm) && seekFrame < length)
		{
			// sound reached the end but seeked to an earlier frame
			bgm.atEnd = false;
		}
	}

	float AudioManager::getMasterVolume()
	{
		return masterVolume;
	}

	void AudioManager::setMasterVolume(float volume)
	{
		masterVolume = volume;
		ma_engine_set_volume(&engine, volume);
	}

	float AudioManager::getBGMVolume()
	{
		return bgmVolume;
	}

	void AudioManager::setBGMVolume(float volume)
	{
		bgmVolume = volume;
		ma_sound_group_set_volume(&bgmGroup, volume * bgmVolumeFactor);
	}

	float AudioManager::getSEVolume()
	{
		return seVolume;
	}

	void AudioManager::setSEVolume(float volume)
	{
		seVolume = volume;
		ma_sound_group_set_volume(&seGroup, volume * seVolumeFactor);
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
