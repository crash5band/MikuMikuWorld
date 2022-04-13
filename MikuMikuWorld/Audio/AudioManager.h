#pragma once
#include "AudioEvent.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <memory>

namespace MikuMikuWorld
{

	class AudioManager
	{
	private:
		ma_engine engine;
		ma_sound_group_config bgmGroupConfig;
		ma_sound_group bgmGroup;
		ma_sound_group seGroup;
		ma_sound_config bgmConfig;
		ma_sound bgm;
		std::unordered_map<std::string, ma_sound> seMap;
		std::vector<std::unique_ptr<AudioEvent>> audioEvents;

		float bgmOffset = 0.0f;
		bool musicInitialized = false;

	public:
		void initAudio();
		void loadSE();
		void changeBGM(const std::string& filename);
		void setMasterVolume(float volume);
		void setBGMVolume(float volume);
		void setSEVolume(float volume);
		void playBGM(float currTime);
		void pauseBGM();
		void stopBGM();
		void seekBGM(float time);
		void disposeBGM();
		void uninitAudio();
		void playSE(const char* se, float time);
		void setBGMOffset(float time, float sec);
		void reSync();
		void pushAudioEvent(const char* se, double start, double end, bool loop);
		void clearEvents();
		void stopAllEvents();
		float getAudioPosition();
		float getBGMOffset();
		float getEngineAbsTime();
		float getSongEndTime();
		bool isMusicInitialized();
	};
}