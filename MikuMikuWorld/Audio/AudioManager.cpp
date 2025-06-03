#include "../Application.h"
#include "../IO.h"
#include "../UI.h"

// We need to add the implementation defines BEFORE including miniaudio's header
#define MINIAUDIO_IMPLEMENTATION
#define DR_MP3_IMPLEMENTATION
#define DR_WAV_IMPLEMENTATION
#define DR_FLAC_IMPLEMENTATION
#include "AudioManager.h"
#include <execution>

#undef STB_VORBIS_HEADER_ONLY

namespace Audio
{
	namespace mmw = MikuMikuWorld;

	void AudioManager::initializeAudioEngine()
	{
		std::string err = "";
		ma_result result = MA_SUCCESS;

		try
		{
			result = ma_engine_init(nullptr, &engine);
			if (result != MA_SUCCESS)
			{
				err = "FATAL: Failed to start audio engine. Aborting.\n";
				throw(result);
			}

			result = ma_sound_group_init(&engine, maSoundFlagsDefault, nullptr, &musicGroup);
			if (result != MA_SUCCESS)
			{
				err = "FATAL: Failed to initialize music sound group. Aborting.\n";
				throw(result);
			}

			result = ma_sound_group_init(&engine, maSoundFlagsDefault, nullptr, &soundEffectsGroup);
			if (result != MA_SUCCESS)
			{
				err = "FATAL: Failed to initialize sound effects sound group. Aborting.\n";
				throw(result);
			}
		}
		catch (ma_result)
		{
			err.append(ma_result_description(result));
			IO::messageBox(APP_NAME, err, IO::MessageBoxButtons::Ok, IO::MessageBoxIcon::Error, mmw::Application::windowState.windowHandle);

			exit(result);
		}
	}

	void AudioManager::startEngine()
	{
		ma_engine_start(&engine);
	}

	void AudioManager::stopEngine()
	{
		ma_engine_stop(&engine);
	}

	bool AudioManager::isEngineStarted() const
	{
		return ma_device_get_state(engine.pDevice) == ma_device_state_started;
	}

	void AudioManager::loadSoundEffects()
	{
		constexpr size_t soundEffectsCount = sizeof(mmw::SE_NAMES) / sizeof(const char*);
		constexpr std::array<SoundFlags, soundEffectsCount> soundEffectsFlags =
		{
			NONE, NONE, NONE, NONE, LOOP | EXTENDABLE, NONE, NONE, NONE, NONE, LOOP | EXTENDABLE
		};

		constexpr std::array<float, soundEffectsCount> soundEffectsVolumes =
		{
			0.75f, 0.75f, 0.90f, 0.80f, 0.70f, 0.75f, 0.80f, 0.92f, 0.82f, 0.70f
		};

		debugSounds.resize(soundEffectsCount * soundEffectsProfileCount);
		
		for (size_t index = 0; index < soundEffectsProfileCount; index++)
		{
			std::string path = (mmw::Application::getResDir() / "sound" / IO::formatString("%02d", index + 1) / "").string();
			for (size_t i = 0; i < soundEffectsCount; ++i)
				sounds[index].pool.emplace(std::move(SoundPoolPair(mmw::SE_NAMES[i], std::make_unique<SoundPool>())));

#if defined(_WIN32)
			std::for_each(std::execution::par, sounds[index].pool.begin(), sounds[index].pool.end(), [&](auto& s)
#else
			std::for_each(sounds[index].pool.begin(), sounds[index].pool.end(), [&](auto& s)
#endif
			{
				std::string filename = path + s.first.data() + ".mp3";
				size_t soundNameIndex = mmw::findArrayItem(s.first.data(), mmw::SE_NAMES, mmw::arrayLength(mmw::SE_NAMES));

				std::string name{};
				if (mmw::isArrayIndexInBounds(soundNameIndex, mmw::SE_NAMES))
					name = IO::formatString("%s_%02d", mmw::SE_NAMES[soundNameIndex], index + 1);

				s.second->initialize(name, filename, &engine, &soundEffectsGroup, soundEffectsFlags[soundNameIndex]);
				s.second->setVolume(soundEffectsVolumes[soundNameIndex]);

				SoundInstance& debugSound = debugSounds[soundNameIndex + (index * soundEffectsCount)];
				debugSound.name = name;

				ma_sound_init_from_file(&engine, filename.c_str(), maSoundFlagsDecodeAsync, &soundEffectsGroup, nullptr, &debugSound.source);
			});

			// Adjust hold SE loop times for gapless playback
			ma_uint64 holdNrmDuration = sounds[index].pool[mmw::SE_CONNECT]->getDurationInFrames();
			ma_uint64 holdCrtDuration = sounds[index].pool[mmw::SE_CRITICAL_CONNECT]->getDurationInFrames();
			sounds[index].pool[mmw::SE_CONNECT]->setLoopTime(3000, holdNrmDuration - 3000);
			sounds[index].pool[mmw::SE_CRITICAL_CONNECT]->setLoopTime(3000, holdCrtDuration - 3000);
		}
	}

	void AudioManager::uninitializeAudioEngine()
	{
		disposeMusic();
		for (size_t index = 0; index < soundEffectsProfileCount; index++)
		{
			for (auto& [name, sound] : sounds[index].pool)
				sound->dispose();

			sounds[index].pool.clear();
		}

		ma_engine_uninit(&engine);
	}

	mmw::Result AudioManager::loadMusic(const std::string& filename)
	{
		disposeMusic();
		mmw::Result result = decodeAudioFile(filename, musicBuffer);
		if (result.isOk())
		{
			// We want to always enable pitch here for miniaudio's resampler to work with playback speed
			ma_sound_init_from_data_source(&engine, &musicBuffer.buffer, MA_SOUND_FLAG_NO_SPATIALIZATION, &musicGroup, &music);

			// Sync
			setPlaybackSpeed(playbackSpeed, 0);
		}

		return result;
	}

	void AudioManager::playMusic(float currentTime)
	{
		ma_uint64 length{};
		ma_sound_get_length_in_pcm_frames(&music, &length);

		// Negative time means the sound is midways
		float time = musicOffset - currentTime;

		// Starting past the music end
		if (time * musicBuffer.sampleRate * -1 > length)
			return;

		ma_sound_set_start_time_in_milliseconds(&music, std::max(0.0f, time * 1000));
		ma_sound_start(&music);
	}

	void AudioManager::stopMusic()
	{
		ma_sound_stop(&music);
	}

	void AudioManager::setMusicOffset(float currentTime, float offset)
	{
		musicOffset = offset / 1000.0f;
		float seekTime = currentTime - musicOffset;
		ma_sound_seek_to_pcm_frame(&music, seekTime * musicBuffer.sampleRate);

		float start = getAudioEngineAbsoluteTime() + musicOffset - currentTime;
		ma_sound_set_start_time_in_milliseconds(&music, std::max(0.0f, start * 1000));
	}

	float AudioManager::getMusicPosition()
	{
		float cursor{};
		ma_sound_get_cursor_in_seconds(&music, &cursor);

		return cursor;
	}

	float AudioManager::getMusicLength()
	{
		float length{};
		ma_sound_get_length_in_seconds(&music, &length);

		return length;
	}

	void AudioManager::disposeMusic()
	{
		if (musicBuffer.isValid())
		{
			ma_sound_stop(&music);
			ma_sound_uninit(&music);
			musicBuffer.dispose();
		}
	}

	void AudioManager::seekMusic(float time)
	{
		ma_uint64 seekFrame = (time - musicOffset) * musicBuffer.sampleRate;
		ma_sound_seek_to_pcm_frame(&music, seekFrame);

		ma_uint64 length{};
		ma_result lengthResult = ma_sound_get_length_in_pcm_frames(&music, &length);
		if (lengthResult != MA_SUCCESS)
			return;

		if (seekFrame > length)
		{
			// Seeking beyond the sound's length
			music.atEnd = true;
		}
		else if (ma_sound_at_end(&music) && seekFrame < length)
		{
			// Sound reached the end but sought to an earlier frame
			music.atEnd = false;
		}
	}

	float AudioManager::getMasterVolume() const
	{
		return masterVolume;
	}

	void AudioManager::setMasterVolume(float volume)
	{
		masterVolume = volume;
		ma_engine_set_volume(&engine, volume);
	}

	float AudioManager::getMusicVolume() const
	{
		return musicVolume;
	}

	void AudioManager::setMusicVolume(float volume)
	{
		musicVolume = volume;
		ma_sound_group_set_volume(&musicGroup, volume);
	}

	float AudioManager::getSoundEffectsVolume() const
	{
		return soundEffectsVolume;
	}

	void AudioManager::setSoundEffectsVolume(float volume)
	{
		soundEffectsVolume = volume;
		ma_sound_group_set_volume(&soundEffectsGroup, volume);
	}

	float AudioManager::getPlaybackSpeed() const
	{
		return playbackSpeed;
	}

	void AudioManager::setPlaybackSpeed(float speed, float currentTime)
	{
		const ma_uint32 speedAdjustedSampleRate = static_cast<ma_uint32>(speed * musicBuffer.sampleRate);
		musicBuffer.effectiveSampleRate = speedAdjustedSampleRate;
		music.engineNode.sampleRate = speedAdjustedSampleRate;

		ma_uint32 sampleRateIn = speedAdjustedSampleRate;
		ma_uint32 sampleRateOut = engine.sampleRate;
		ma_uint32 gcf = mmw::gcf(sampleRateIn, sampleRateOut);
		sampleRateIn /= gcf;
		sampleRateOut /= gcf;

		ma_linear_resampler& resampler = music.engineNode.resampler;
		resampler.lpf.sampleRate = std::max(sampleRateIn, sampleRateOut);
		resampler.inAdvanceInt = sampleRateIn / sampleRateOut;
		resampler.inAdvanceFrac = sampleRateIn % sampleRateOut;
		resampler.config.sampleRateIn = sampleRateIn;
		resampler.config.sampleRateOut = sampleRateOut;

		// Adjust timing of extendable sounds
		for (auto& [name, sound] : sounds[soundEffectsProfileIndex].pool)
		{
			if ((sound->flags & EXTENDABLE) != 0)
			{
				for (auto& instance : sound->pool)
				{
					if (instance.isPlaying())
						instance.extendDuration(currentTime, instance.absoluteEnd, speed);
				}
			}
		}

		playbackSpeed = speed;
	}

	void AudioManager::playOneShotSound(std::string_view name)
	{
		if (sounds[soundEffectsProfileIndex].pool.find(name) == sounds[soundEffectsProfileIndex].pool.end())
			return;

		sounds[soundEffectsProfileIndex].pool.at(name)->play(0, -1);
	}

	void AudioManager::playSoundEffect(std::string_view name, float start, float end, float currentTime)
	{
		if (sounds[soundEffectsProfileIndex].pool.find(name) == sounds[soundEffectsProfileIndex].pool.end())
			return;

		SoundPool* soundPool = sounds[soundEffectsProfileIndex].pool.at(name).get();
		const float absoluteStart = start + lastPlaybackTime;
		const float absoluteEnd = end + lastPlaybackTime;
		const int poolIndex = soundPool->getCurrentIndex();

		if (soundPool->flags & SoundFlags::EXTENDABLE)
		{
			// We want to re-use the currently playing instance
			SoundInstance& currentInstance = soundPool->pool[poolIndex];

			// If the start time is immediate, the source's time is effectively 0 and the sound isn't marked playing yet
			const bool isCurrentInstancePlaying = ma_sound_is_playing(&currentInstance.source) ||
				(absoluteStart == currentInstance.absoluteStart && currentInstance.lastStartTime != 0.0f);

			const bool isNewSoundWithinOldRange =
				mmw::isWithinRange(absoluteStart, currentInstance.absoluteStart, currentInstance.absoluteEnd) &&
				mmw::isWithinRange(absoluteEnd, currentInstance.absoluteStart, currentInstance.absoluteEnd);

			if (isNewSoundWithinOldRange && isCurrentInstancePlaying)
				return;

			if (isCurrentInstancePlaying && absoluteEnd > currentInstance.absoluteEnd)
			{
				currentInstance.extendDuration(currentTime, absoluteEnd, playbackSpeed);
				return;
			}
		}

		const float scaledEnd = ((absoluteEnd - absoluteStart) / playbackSpeed) + getAudioEngineAbsoluteTime();

		soundPool->pool[poolIndex].absoluteStart = absoluteStart;
		soundPool->pool[poolIndex].absoluteEnd = absoluteEnd;
		soundPool->play(start, end == -1 ? end : scaledEnd);
	}

	void AudioManager::stopSoundEffects(bool all)
	{
		if (all)
		{
			for (size_t index = 0; index < soundEffectsProfileCount; index++)
				for (auto& [se, sound] : sounds[index].pool)
					sound->stopAll();
		}
		else
		{
			sounds[soundEffectsProfileIndex].pool[mmw::SE_CONNECT]->stopAll();
			sounds[soundEffectsProfileIndex].pool[mmw::SE_CRITICAL_CONNECT]->stopAll();

			// Also stop any scheduled sounds
			for (size_t index = 0; index < soundEffectsProfileCount; index++)
			{
				for (auto& [se, sound] : sounds[index].pool)
				{
					for (auto& instance : sound->pool)
					{
						ma_uint64 cursor{};
						ma_sound_get_cursor_in_pcm_frames(&instance.source, &cursor);
						if (cursor <= 0)
							ma_sound_stop(&instance.source);
					}
				}
			}
		}
	}

	uint32_t AudioManager::getDeviceChannelCount() const
	{
		return engine.pDevice->playback.channels;
	}

	float AudioManager::getDeviceLatency() const
	{
		return engine.pDevice->playback.internalPeriodSizeInFrames / static_cast<float>(engine.pDevice->playback.internalSampleRate);
	}

	uint32_t AudioManager::getDeviceSampleRate() const
	{
		return engine.pDevice->playback.internalSampleRate;
	}

	float AudioManager::getAudioEngineAbsoluteTime() const
	{
		return static_cast<float>(ma_engine_get_time_in_milliseconds(&engine)) / 1000.0f;
	}

	float AudioManager::getMusicOffset() const
	{
		return musicOffset;
	}

	float AudioManager::getMusicEndTime() const
	{
		if (!isMusicInitialized())
			return 0.0f;

		float length = 0.0f;
		ma_data_source_get_length_in_seconds(music.pDataSource, &length);

		return length + musicOffset;
	}

	void AudioManager::syncAudioEngineTimer()
	{
		ma_engine_set_time(&engine, 0);
	}

	bool AudioManager::isMusicInitialized() const
	{
		return musicBuffer.isValid();
	}

	bool AudioManager::isMusicAtEnd() const
	{
		return ma_sound_at_end(&music);
	}

	bool AudioManager::isSoundPlaying(std::string_view name) const
	{
		if (sounds[soundEffectsProfileIndex].pool.find(name) == sounds[soundEffectsProfileIndex].pool.end())
			return false;

		return sounds[soundEffectsProfileIndex].pool.at(name)->isAnyPlaying();
	}

	size_t AudioManager::getSoundEffectsProfileIndex() const
	{
		return soundEffectsProfileIndex;
	}

	void AudioManager::setSoundEffectsProfileIndex(size_t index)
	{
		soundEffectsProfileIndex = index;
	}

	float AudioManager::getLastPlaybackTime() const
	{
		return lastPlaybackTime;
	}

	void AudioManager::setLastPlaybackTime(float time)
	{
		lastPlaybackTime = time;
	}
}
