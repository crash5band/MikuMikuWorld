#include "PreviewEngine.h"
#include "Constants.h"

namespace MikuMikuWorld
{
	Engine::Range Engine::getNoteVisualTime(Note const& note, Score const& score, float noteSpeed) {
		float targetTime = accumulateScaledDuration(note.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
		float duration = lerp(0.35, 4.f, std::pow(unlerp(12, 1, noteSpeed), 1.31f));
		return {targetTime - duration, targetTime};
	}
}


