#pragma once
#include "Sound.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <memory>

namespace MikuMikuWorld
{
	class Result;

	class AudioManager
	{
	private:
		ma_engine engine;
		ma_sound_group bgmGroup;
		ma_sound_group seGroup;
		ma_sound bgm;
		std::unordered_map<std::string, std::unique_ptr<Sound>> sounds;

		float bgmOffset = 0.0f;
		bool musicInitialized = false;

		float masterVolume;
		float bgmVolume;
		float seVolume;

		float bgmVolumeFactor = 1.0f;
		float seVolumeFactor = 0.74f;

	public:
		void initAudio();
		void loadSE();
		Result changeBGM(const std::string& filename);
		void setMasterVolume(float volume);
		void setBGMVolume(float volume);
		void setSEVolume(float volume);
		void playBGM(float currTime);
		void pauseBGM();
		void stopBGM();
		void seekBGM(float time);
		void disposeBGM();
		void uninitAudio();
		void setBGMOffset(float time, float sec);
		void reSync();
		void playSound(const char* se, double start, double end);
		void stopSounds(bool all);
		float getAudioPosition();
		float getBGMOffset();
		float getEngineAbsTime();
		float getSongEndTime();
		bool isMusicInitialized();
		bool isMusicAtEnd();

		float getMasterVolume();
		float getBGMVolume();
		float getSEVolume();
	};
}