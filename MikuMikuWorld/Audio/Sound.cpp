#include "../IO.h"
#include "../File.h"
#include "../Math.h"
#include "../Stopwatch.h"

#define MINIAUDIO_IMPLEMENTATION
#include "Sound.h"
#include <algorithm>

namespace Audio
{
	namespace mmw = MikuMikuWorld;

	void Sound::initialize(const std::string& name, ma_uint32 sampleRate, ma_uint32 channelCount, ma_uint64 frameCount, int16_t* samples)
	{
		this->name = name;
		this->sampleFormat = ma_format_s16;
		this->channelCount = channelCount;
		this->frameCount = frameCount;
		this->sampleRate = sampleRate;
		this->samples = std::unique_ptr<int16_t[]>(samples);

		ma_audio_buffer_config bufferConfig = ma_audio_buffer_config_init(this->sampleFormat, channelCount, frameCount, this->samples.get(), nullptr);
		bufferConfig.sampleRate = sampleRate;
		ma_audio_buffer_init(&bufferConfig, &buffer);
	}

	void Sound::dispose()
	{
		name.clear();
		ma_audio_buffer_uninit(&buffer);
		samples.reset();

		sampleFormat	= ma_format_unknown;
		sampleRate		= 0;
		channelCount	= 0;
		frameCount		= 0;
	}

	mmw::Result decodeAudioFile(std::string filename, Sound& sound)
	{
		if (!IO::File::exists(filename))
			return mmw::Result(mmw::ResultStatus::Error, "File not found");

		std::string fileExtension = IO::File::getFileExtension(filename);
		std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);

		if (!isSupportedFileFormat(fileExtension))
			return mmw::Result(mmw::ResultStatus::Error, "Unsupported file format");

		std::string nameWithoutExtension = IO::File::getFilenameWithoutExtension(filename);
		std::wstring wFilename = IO::mbToWideStr(filename);

		IO::File f(wFilename, L"rb");
		std::vector<uint8_t> bytes = f.readAllBytes();
		f.close();

		if (fileExtension == ".mp3")
		{
			ma_dr_mp3_config mp3Config{};
			uint64_t frameCount{};
			int16_t* samples = ma_dr_mp3_open_memory_and_read_pcm_frames_s16(bytes.data(), bytes.size(), &mp3Config, &frameCount, nullptr);
			if (samples == nullptr)
				return mmw::Result(mmw::ResultStatus::Error, "Failed to decode mp3");

			sound.initialize(nameWithoutExtension, mp3Config.sampleRate, mp3Config.channels, frameCount, samples);
			return mmw::Result::Ok();
		}
		else if (fileExtension == ".wav")
		{
			uint32_t channels{};
			uint32_t sampleRate{};
			uint64_t frameCount{};
			int16_t* samples = ma_dr_wav_open_memory_and_read_pcm_frames_s16(bytes.data(), bytes.size(), &channels, &sampleRate, &frameCount, nullptr);
			if (samples == nullptr)
				return mmw::Result(mmw::ResultStatus::Error, "Failed to decode wav");

			sound.initialize(nameWithoutExtension, sampleRate, channels, frameCount, samples);
			return mmw::Result::Ok();
		}
		else if (fileExtension == ".flac")
		{
			uint32_t channels{};
			uint32_t sampleRate{};
			uint64_t frameCount{};
			int16_t* samples = ma_dr_flac_open_memory_and_read_pcm_frames_s16(bytes.data(), bytes.size(), &channels, &sampleRate, &frameCount, nullptr);
			if (samples == nullptr)
				return mmw::Result(mmw::ResultStatus::Error, "Failed to decode flac");

			sound.initialize(nameWithoutExtension, sampleRate, channels, frameCount, samples);
			return mmw::Result::Ok();
		}
		else if (fileExtension == ".ogg")
		{
			int32_t channels{};
			int32_t sampleRate{};
			int16_t* samples;
			int32_t frameCount = stb_vorbis_decode_memory(bytes.data(), bytes.size(), &channels, &sampleRate, &samples);
			if (samples == nullptr)
				return mmw::Result(mmw::ResultStatus::Error, "Failed to decode ogg vorbis");

			sound.initialize(nameWithoutExtension, sampleRate, channels, frameCount, samples);
			return mmw::Result::Ok();
		}

		// Getting here should mean an unsupported file format
		return mmw::Result(mmw::ResultStatus::Error, "Unsupported file format");
	}

	bool isSupportedFileFormat(const std::string_view& fileExtension)
	{
		return std::find(supportedFileFormats.begin(), supportedFileFormats.end(), fileExtension) != supportedFileFormats.end();
	}

	ma_uint64 SoundPool::getDurationInFrames() const
	{
		ma_uint64 length{};
		ma_data_source_get_length_in_pcm_frames(pool[0].source.pDataSource, &length);

		return length;
	}

	void SoundPool::setLoopTime(ma_uint64 startFrames, ma_uint64 endFrames)
	{
		for (auto& instance : pool)
			ma_data_source_set_loop_point_in_pcm_frames(instance.source.pDataSource, startFrames, endFrames);
	}

	bool SoundPool::isLooping() const
	{
		return flags & SoundFlags::LOOP;
	}

	void SoundPool::setVolume(float volume)
	{
		for (auto& instance : pool)
			ma_sound_set_volume(&instance.source, volume);
	}

	float SoundPool::getVolume() const
	{
		return ma_sound_get_volume(&pool[0].source);
	}

	void SoundPool::initialize(const std::string& path, ma_engine* engine, ma_sound_group* group, SoundFlags flags)
	{
		constexpr ma_uint32 maSoundFlags =
			MA_SOUND_FLAG_NO_PITCH |
			MA_SOUND_FLAG_NO_SPATIALIZATION |
			MA_SOUND_FLAG_DECODE |
			MA_SOUND_FLAG_ASYNC;

		std::wstring wPath = IO::mbToWideStr(path);
		for (int i = 0; i < pool.size(); ++i)
		{
			ma_result result = ma_sound_init_from_file_w(engine, wPath.c_str(), maSoundFlags, group, NULL, &pool[i].source);
			if (flags & SoundFlags::LOOP)
				ma_sound_set_looping(&pool[i].source, true);
		}

		this->flags = flags;
		nextIndex = 0;
	}

	void SoundPool::dispose()
	{
		for (auto& instance : pool)
			ma_sound_uninit(&instance.source);
	}

	void SoundPool::extendInstanceDuration(SoundInstance& instance, float newEndTime)
	{
		ma_sound_set_stop_time_in_milliseconds(&instance.source, newEndTime * 1000);
	}

	void SoundPool::play(float start, float end)
	{
		if (flags & SoundFlags::EXTENDABLE)
		{
			// We want to re-use the currently playing instance
			int currentIndex = std::max(nextIndex - 1, 0);
			SoundInstance& currentInstance = pool[currentIndex];

			// If playback is started on a note, the source's time is effectively 0 and the sound isn't marked playing yet
			const bool isCurrentInstancePlaying = ma_sound_is_playing(&currentInstance.source) ||
				(start == currentInstance.lastStartTime && currentInstance.lastStartTime != 0.0f);

			const bool isNewSoundWithinOldRange = 
				mmw::isWithinRange(start, currentInstance.lastStartTime, currentInstance.lastEndTime) &&
				mmw::isWithinRange(end, currentInstance.lastStartTime, currentInstance.lastEndTime);

			if (isNewSoundWithinOldRange && isCurrentInstancePlaying)
				return;

			if (isCurrentInstancePlaying && end > currentInstance.lastEndTime)
			{
				extendInstanceDuration(pool[currentIndex], end);
				return;
			}
		}

		SoundInstance& instance = pool[nextIndex];

		ma_sound_seek_to_pcm_frame(&instance.source, 0);
		ma_sound_set_start_time_in_milliseconds(&instance.source, start * 1000);
		
		if (end > -1)
			ma_sound_set_stop_time_in_milliseconds(&instance.source, end * 1000);

		ma_sound_start(&instance.source);
		instance.lastStartTime = start;
		instance.lastEndTime = end;

		if ((flags & SoundFlags::EXTENDABLE) == 0)
			nextIndex = ++nextIndex % pool.size();
	}

	void SoundPool::stopAll()
	{
		for (auto& instance : pool)
		{
			ma_sound_stop(&instance.source);
			instance.lastStartTime = 0;
			instance.lastEndTime = 0;
		}
	}

	bool SoundPool::isPlaying(const SoundInstance& soundInstance) const
	{
		return ma_sound_is_playing(&soundInstance.source);
	}

	bool SoundPool::isAnyPlaying() const
	{
		for (const auto& instance : pool)
			if (ma_sound_is_playing(&instance.source))
				return true;

		return false;
	}
}
