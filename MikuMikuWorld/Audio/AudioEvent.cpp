#include "AudioEvent.h"

namespace MikuMikuWorld
{
	AudioEvent::AudioEvent(ma_engine* engine, ma_sound_group* group, ma_sound* data, float _start, bool _loop, float _end) :
		start{ _start }, loop{ _loop }, end{ _end }
	{
		ma_uint32 flags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION;
		ma_result result = ma_sound_init_copy(engine, data, flags, group, &sound);

		ma_data_source_get_length_in_seconds(sound.pDataSource, &duration);
		ma_data_source_get_length_in_pcm_frames(sound.pDataSource, &durationInFrames);
	}

	void AudioEvent::play()
	{
		ma_sound_set_looping(&sound, loop);
		ma_sound_set_start_time_in_milliseconds(&sound, start * 1000);
		ma_sound_start(&sound);
		
		if (end != -1)
			ma_sound_set_stop_time_in_milliseconds(&sound, end * 1000);

		if (loop)
		{
			// skip silence frames
			ma_uint64 duration;
			ma_sound_get_length_in_pcm_frames(&sound, &duration);
			ma_data_source_set_loop_point_in_pcm_frames(sound.pDataSource, 3000, durationInFrames - 3000);
		}
	}

	void AudioEvent::stop()
	{
		ma_sound_stop(&sound);
	}

	void AudioEvent::dispose()
	{
		ma_sound_uninit(&sound);
	}
}