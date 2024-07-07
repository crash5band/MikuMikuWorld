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

	void SoundBuffer::initialize(const std::string& name, ma_uint32 sampleRate,
	                             ma_uint32 channelCount, ma_uint64 frameCount, int16_t* samples)
	{
		this->name = name;
		this->sampleFormat = ma_format_s16;
		this->channelCount = channelCount;
		this->frameCount = frameCount;
		this->sampleRate = sampleRate;
		this->samples = std::unique_ptr<int16_t[]>(samples);
		this->effectiveSampleRate = sampleRate;

		ma_audio_buffer_config bufferConfig = ma_audio_buffer_config_init(
		    this->sampleFormat, channelCount, frameCount, this->samples.get(), nullptr);
		bufferConfig.sampleRate = sampleRate;
		ma_audio_buffer_init(&bufferConfig, &buffer);
	}

	void SoundBuffer::dispose()
	{
		name.clear();
		ma_audio_buffer_uninit(&buffer);
		samples.reset();

		sampleFormat = ma_format_unknown;
		sampleRate = 0;
		channelCount = 0;
		frameCount = 0;
		effectiveSampleRate = 0;
	}

	mmw::Result decodeAudioFile(std::string filename, SoundBuffer& sound)
	{
		if (!IO::File::exists(filename))
			return mmw::Result(mmw::ResultStatus::Error, "File not found");

		std::string fileExtension = IO::File::getFileExtension(filename);
		std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(),
		               ::tolower);

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
			int16_t* samples = ma_dr_mp3_open_memory_and_read_pcm_frames_s16(
			    bytes.data(), bytes.size(), &mp3Config, &frameCount, nullptr);
			if (samples == nullptr)
				return mmw::Result(mmw::ResultStatus::Error, "Failed to decode mp3");

			sound.initialize(nameWithoutExtension, mp3Config.sampleRate, mp3Config.channels,
			                 frameCount, samples);
			return mmw::Result::Ok();
		}
		else if (fileExtension == ".wav")
		{
			uint32_t channels{};
			uint32_t sampleRate{};
			uint64_t frameCount{};
			int16_t* samples = ma_dr_wav_open_memory_and_read_pcm_frames_s16(
			    bytes.data(), bytes.size(), &channels, &sampleRate, &frameCount, nullptr);
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
			int16_t* samples = ma_dr_flac_open_memory_and_read_pcm_frames_s16(
			    bytes.data(), bytes.size(), &channels, &sampleRate, &frameCount, nullptr);
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
			int32_t frameCount = stb_vorbis_decode_memory(bytes.data(), bytes.size(), &channels,
			                                              &sampleRate, &samples);
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
		return std::find(supportedFileFormats.begin(), supportedFileFormats.end(), fileExtension) !=
		       supportedFileFormats.end();
	}

	ma_uint64 SoundPool::getDurationInFrames() const
	{
		ma_uint64 length{};
		ma_data_source_get_length_in_pcm_frames(pool[0].source.pDataSource, &length);

		return length;
	}

	float SoundPool::getDurationInSeconds() const
	{
		float length{};
		ma_data_source_get_length_in_seconds(pool[0].source.pDataSource, &length);

		return length;
	}

	void SoundPool::setLoopTime(ma_uint64 startFrames, ma_uint64 endFrames)
	{
		for (auto& instance : pool)
			ma_data_source_set_loop_point_in_pcm_frames(instance.source.pDataSource, startFrames,
			                                            endFrames);
	}

	bool SoundPool::isLooping() const { return flags & SoundFlags::LOOP; }

	void SoundPool::setVolume(float volume)
	{
		for (auto& instance : pool)
			ma_sound_set_volume(&instance.source, volume);
	}

	float SoundPool::getVolume() const { return ma_sound_get_volume(&pool[0].source); }

	void SoundPool::initialize(const std::string& path, ma_engine* engine, ma_sound_group* group,
	                           SoundFlags flags)
	{
		std::wstring wPath = IO::mbToWideStr(path);
		for (int i = 0; i < pool.size(); i++)
		{
			ma_result result = ma_sound_init_from_file_w(
			    engine, wPath.c_str(), maSoundFlagsDecodeAsync, group, NULL, &pool[i].source);
			if (flags & SoundFlags::LOOP)
				ma_sound_set_looping(&pool[i].source, true);
		}

		this->flags = flags;
		currentIndex = 0;
	}

	void SoundPool::initialize(const std::string& name, const std::string& path, ma_engine* engine,
	                           ma_sound_group* group, SoundFlags flags)
	{
		this->name = name;
		initialize(path, engine, group, flags);
	}

	void SoundPool::initialize(SoundBuffer& sound, ma_engine* engine, ma_sound_group* group,
	                           SoundFlags flags)
	{
		for (int i = 0; i < pool.size(); i++)
		{
			ma_result result = ma_sound_init_from_data_source(
			    engine, &sound.buffer, maSoundFlagsDefault, group, &pool[i].source);
			if (flags & SoundFlags::LOOP)
				ma_sound_set_looping(&pool[i].source, true);
		}

		this->flags = flags;
		currentIndex = 0;
	}

	void SoundPool::dispose()
	{
		for (auto& instance : pool)
			ma_sound_uninit(&instance.source);
	}

	void SoundPool::extendInstanceDuration(SoundInstance& instance, float newEndTime)
	{
		ma_sound_set_stop_time_in_milliseconds(&instance.source, newEndTime * 1000);
		instance.lastEndTime = newEndTime;
	}

	void SoundPool::play(float start, float end)
	{
		SoundInstance& instance = pool[currentIndex];

		instance.seek(0);
		ma_sound_set_start_time_in_milliseconds(&instance.source, start * 1000);

		if (end > -1)
			ma_sound_set_stop_time_in_milliseconds(&instance.source, end * 1000);

		instance.play();
		instance.lastStartTime = start;
		instance.lastEndTime = end;

		if ((flags & SoundFlags::EXTENDABLE) == 0)
			currentIndex = ++currentIndex % pool.size();
	}

	void SoundPool::stopAll()
	{
		for (auto& instance : pool)
		{
			instance.stop();
			instance.lastStartTime = 0;
			instance.lastEndTime = 0;
		}
	}

	bool SoundPool::isPlaying(const SoundInstance& soundInstance) const
	{
		return soundInstance.isPlaying();
	}

	bool SoundPool::isAnyPlaying() const
	{
		return std::any_of(pool.begin(), pool.end(),
		                   [this](const SoundInstance& a) { return isPlaying(a); });
	}
}
