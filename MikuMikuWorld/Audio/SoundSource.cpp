#include "SoundSource.h"

namespace MikuMikuWorld
{
	SoundSource::SoundSource()
	{

	}

	void SoundSource::init(const std::wstring& filename, ma_engine* engine, ma_sound_group* group, ma_uint32 mFlags, bool loop)
	{
		ma_result result = ma_sound_init_from_file_w(engine, filename.c_str(), mFlags, group, NULL, &source);
		ma_sound_set_looping(&source, loop);
	}

	void SoundSource::setStart(float start)
	{
		ma_sound_set_start_time_in_milliseconds(&source, start * 1000);
	}

	void SoundSource::setEnd(float end)
	{
		ma_sound_set_stop_time_in_milliseconds(&source, end * 1000);
	}

	void SoundSource::play()
	{
		ma_sound_start(&source);
	}

	void SoundSource::stop()
	{
		ma_sound_stop(&source);
	}

	void SoundSource::reset()
	{
		ma_sound_seek_to_pcm_frame(&source, 0);
	}

	void SoundSource::setLoop(bool val)
	{
		ma_sound_set_looping(&source, val);
	}

	void SoundSource::setLoopTime(ma_uint64 start, ma_uint64 end)
	{
		ma_data_source_set_loop_point_in_pcm_frames(source.pDataSource, start, end);
	}

	void SoundSource::dispose()
	{
		ma_sound_uninit(&source);
	}

	ma_uint64 SoundSource::getPosition()
	{
		ma_uint64 cur = 0;
		ma_sound_get_cursor_in_pcm_frames(&source, &cur);

		return cur;
	}

	ma_uint64 SoundSource::getDurationInFrames()
	{
		ma_uint64 cur = 0;
		ma_sound_get_length_in_pcm_frames(&source, &cur);

		return cur;
	}

	float SoundSource::getDurationInSeconds()
	{
		float cur = 0.0f;
		ma_sound_get_length_in_seconds(&source, &cur);

		return cur;
	}

	bool SoundSource::isPlaying()
	{
		return ma_sound_is_playing(&source);
	}

	bool SoundSource::isAtEnd()
	{
		return ma_sound_at_end(&source);
	}
}
