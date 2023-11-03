#pragma once
#include "Sound.h"
#include <unordered_map>
#include <vector>
#include <memory>

namespace Audio
{
	class AudioManager
	{
	private:
		ma_engine engine;
		ma_sound music;
		ma_sound_group musicGroup;
		ma_sound_group soundEffectsGroup;
		std::unordered_map<std::string_view, std::unique_ptr<SoundPool>> sounds;

		// Offset from chart time in seconds
		float musicOffset{ 0.0f };

		float masterVolume{ 1.0f };
		float musicVolume{ 1.0f };
		float soundEffectsVolume{ 1.0f };

	public:
		SoundBuffer musicBuffer;
		std::vector<SoundInstance> debugSounds;

		void initializeAudioEngine();
		void uninitializeAudioEngine();
		void syncAudioEngineTimer();

		uint32_t getDeviceSampleRate() const;
		uint32_t getDeviceChannelCount() const;
		float getDeviceLatency() const;
		float getAudioEngineAbsoluteTime() const;

		void loadSoundEffects();
		MikuMikuWorld::Result loadMusic(const std::string& filename);

		void setMasterVolume(float volume);
		float getMasterVolume() const;

		void setMusicVolume(float volume);
		float getMusicVolume() const;

		void setSoundEffectsVolume(float volume);
		float getSoundEffectsVolume() const;

		void playMusic(float currentTime);
		void stopMusic();
		void seekMusic(float time);
		void setMusicOffset(float currentTime, float offset);
		float getMusicPosition();
		float getMusicLength();
		float getMusicOffset() const;
		float getMusicEndTime();
		bool isMusicInitialized() const;
		bool isMusicAtEnd() const;
		void disposeMusic();

		void playOneShotSound(std::string_view name);
		void playSoundEffect(std::string_view name, float start, float end);
		void stopSoundEffects(bool all);
		bool isSoundPlaying(std::string_view name) const;

		using SoundPoolPair = std::pair<std::string_view, std::unique_ptr<SoundPool>>;
	};
}