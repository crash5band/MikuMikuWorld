#include "PreviewEngine.h"
#include "Constants.h"

namespace MikuMikuWorld::Engine
{
	Range getNoteVisualTime(Note const& note, Score const& score, float noteSpeed) {
		float targetTime = accumulateScaledDuration(note.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
		float duration = lerp(0.35, 4.f, std::pow(unlerp(12, 1, noteSpeed), 1.31f));
		return {targetTime - duration, targetTime};
	}

	struct TargetTimeComparer {
		bool operator()(const Range& a, const Range& b) const
		{
			return a.max < b.max;
		}
	};

	void DrawData::calculateDrawData(Score const &score)
	{
		this->clear();
		
		std::map<Range, Range, TargetTimeComparer> simBuilder;
		for (auto rit = score.notes.rbegin(), rend = score.notes.rend(); rit != rend; rit++) {
			auto& [id, note] = *rit;
			switch (note.getType()) {
				case NoteType::Tap:
				//case NoteType::Hold:
				//case NoteType::HoldEnd:
				{
					auto visual_tm = getNoteVisualTime(note, score, config.noteSpeed);
					drawingNotes.push_back(DrawingNote{note.ID, visual_tm});

					// Find the max and min lane within the same height (visual_tm.max)
					float center = laneToLeft(note.lane) + note.width / 2.f;
					auto&& [it, has_emplaced] = simBuilder.try_emplace(visual_tm, Range{center, center});
					auto& x_range = it->second;
					if (has_emplaced)
						break;
					if (center < x_range.min)
						x_range.min = center;
					if (center > x_range.max)
						x_range.max = center;
				}
			};
		}

		for (const auto& [visual_tm, x_range] : simBuilder) {
			if (x_range.min == x_range.max)
				continue;
			drawingLines.push_back(DrawingLine{x_range, visual_tm});
		}
	}

	void DrawData::clear() {
		drawingLines.clear();
		drawingNotes.clear();
	}
}

