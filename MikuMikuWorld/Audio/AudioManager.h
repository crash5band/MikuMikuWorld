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

	struct AudioData
	{
		ma_format sampleFormat{ ma_format_unknown };
		ma_uint32 sampleRate{};
		ma_uint32 channelCount{};
		ma_uint64 frameCount{};
		float* sampleBuffer{ nullptr };

		void clear()
		{
			sampleFormat = ma_format_unknown;
			sampleRate = {};
			channelCount = {};
			frameCount = {};
			sampleBuffer = nullptr;
		}

		bool isValid() const
		{
			return sampleBuffer != nullptr && frameCount > 0 && sampleRate > 0;
		}
	};

	class AudioManager
	{
	private:
		ma_engine engine;
		ma_engine_config engineConfig;
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
		AudioData musicAudioData;

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