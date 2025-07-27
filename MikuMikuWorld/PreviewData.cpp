#include <stack>
#include <stdexcept>
#include "PreviewData.h"
#include "ApplicationConfiguration.h"
#include "Constants.h"
#include "ResourceManager.h"

namespace MikuMikuWorld::Engine
{
    struct TargetTimeComparer
	{
		bool operator()(const Range& a, const Range& b) const
		{
			return a.max < b.max;
		}
	};

	struct GroupIDComparer
	{
		bool operator()(const Particle& particle, int groupID) {
			return particle.groupID < groupID;
		}
		bool operator()(int groupID, const Particle& particle) {
			return groupID < particle.groupID;
		}
		bool operator()(const Particle& particleA, const Particle& particleB) {
			return particleA.groupID < particleB.groupID;
		}
	};

	static bool ensureValidParticle(ParticleEffectType& demandType);
    static void addParticleEffect(DrawData& drawData, ParticleEffectType type, const Note& note, Score const &score, float effectDuration = NAN);
    static void addLaneEffect(DrawData& drawData, const Note& note, Score const &score);
	static void addNoteEffect(DrawData& drawData, const Note& note, Score const &score);

    void DrawData::calculateDrawData(Score const &score)
	{
		this->clear();
		try
		{
			this->noteSpeed = config.pvNoteSpeed;
			this->hasLaneEffect = config.pvLaneEffect;
			this->hasNoteEffect = config.pvNoteEffect;
			std::map<Range, Range, TargetTimeComparer> simBuilder;
			for (auto rit = score.notes.rbegin(), rend = score.notes.rend(); rit != rend; rit++)
			{
				auto& [id, note] = *rit;
				maxTicks = std::max(note.tick, maxTicks);
				NoteType type = note.getType();
				bool hidden = false;
				if (hasNoteEffect)
					addNoteEffect(*this, note, score);
				if (type == NoteType::Hold)
					hidden = score.holdNotes.at(id).startType != HoldNoteType::Normal;
				else if (type == NoteType::HoldEnd)
					hidden = score.holdNotes.at(note.parentID).endType != HoldNoteType::Normal;
				if (hidden)
					continue;
				if (type == NoteType::HoldMid)
					continue;
				if (hasLaneEffect)
					addLaneEffect(*this, note, score);
					
				auto visual_tm = getNoteVisualTime(note, score, noteSpeed);
				drawingNotes.push_back(DrawingNote{note.ID, visual_tm});

				// Find the max and min lane within the same height (visual_tm.max)
				float center = getNoteCenter(note);
				auto&& [it, has_emplaced] = simBuilder.try_emplace(visual_tm, Range{center, center});
				auto& x_range = it->second;
				if (has_emplaced)
					continue;
				if (center < x_range.min)
					x_range.min = center;
				if (center > x_range.max)
					x_range.max = center;
				continue;
			}

			for (const auto& [visual_tm, x_range] : simBuilder)
			{
				if (x_range.min != x_range.max)
					drawingLines.push_back(DrawingLine{x_range, visual_tm});
			}

			std::stack<HoldStep const*> skipStepIDs;
			float noteDuration = getNoteDuration(noteSpeed);
			for (auto rit = score.holdNotes.rbegin(), rend = score.holdNotes.rend(); rit != rend; rit++)
			{
				auto& [id, holdNote] = *rit;
				const Note& startNote = score.notes.at(holdNote.start.ID), endNote = score.notes.at(holdNote.end);
				float start_tm = accumulateDuration(startNote.tick, TICKS_PER_BEAT, score.tempoChanges);
				auto it = holdNote.steps.begin(), end = holdNote.steps.end();
				for (const HoldStep* head = &holdNote.start, *tail; head != nullptr; head = tail, ++it)
				{
					while (it != end && it->type == HoldStepType::Skip)
					{
						skipStepIDs.push(&(*it));
						++it;
					}
					tail = it != end ? &(*it) : nullptr;
					const Note& headNote = score.notes.at(head->ID), tailNote = score.notes.at(tail ? tail->ID : holdNote.end);
					float head_scaled_tm = accumulateScaledDuration(headNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
					float tail_scaled_tm = accumulateScaledDuration(tailNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);

					drawingHoldSegments.push_back(DrawingHoldSegment {
						holdNote.end,
						headNote.ID,
						tailNote.ID,
						head_scaled_tm,
						tail_scaled_tm,
						start_tm,
						head->ease,
						holdNote.isGuide(),
					});

					while(skipStepIDs.size())
					{
						const HoldStep& skipStep = *skipStepIDs.top();
						const Note& skipNote = score.notes.at(skipStep.ID);
						float step_scaled_tm = accumulateScaledDuration(skipNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
						auto ease = getEaseFunction(head->ease);
						float headLeft = Engine::laneToLeft(headNote.lane);
						float headRight = Engine::laneToLeft(headNote.lane) + headNote.width;
						float tailLeft = Engine::laneToLeft(tailNote.lane);
						float tailRight = Engine::laneToLeft(tailNote.lane) + tailNote.width;
						float stepT = unlerp(head_scaled_tm, tail_scaled_tm, step_scaled_tm);
						float stepLeft = ease(headLeft, tailLeft, stepT);
						float stepRight = ease(headRight, tailRight, stepT);
						drawingHoldTicks.push_back(DrawingHoldTick{
							skipStep.ID,
							stepLeft + (stepRight - stepLeft) / 2,
							{step_scaled_tm - noteDuration, step_scaled_tm}
						});
						skipStepIDs.pop();
					}
					if (tail && tail->type != HoldStepType::Hidden)
					{
						float tick_scaled_tm = accumulateScaledDuration(tailNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
						drawingHoldTicks.push_back(DrawingHoldTick{
							tailNote.ID,
							getNoteCenter(tailNote),
							{tick_scaled_tm - noteDuration, tick_scaled_tm}
						});
					}
				}
			}
		}
		catch(const std::out_of_range& ex)
		{
			this->clear();
		}
	}

	void DrawData::clear()
	{
		drawingLines.clear();
		drawingNotes.clear();
		drawingHoldTicks.clear();
		drawingHoldSegments.clear();
		drawingParticles.clear();
		maxTicks = 1;
		rng.seed((uint32_t)time(nullptr));
	}

    static bool ensureValidParticle(ParticleEffectType &effectType)
    {
		int effectID = static_cast<int>(effectType);
		decltype(particleEffectFallback)::iterator it;
		if (effectType == ParticleEffectType::Invalid)
			return false;
        if (effectID >= ResourceManager::particleEffects.size() || ResourceManager::particleEffects[effectID].particles.size() == 0)
		{
			ParticleEffectType originalEffect = effectType;
			while ((it = particleEffectFallback.find(effectType)) != particleEffectFallback.end())
			{
				effectType = it->second;
				effectID = static_cast<int>(effectType);
				if (effectID < ResourceManager::particleEffects.size() && ResourceManager::particleEffects[effectID].particles.size() != 0)
					return true;
			}
			#ifndef NDEBUG
			fprintf(stderr, "Fail attempt to spawn particle effect id %d\n", (int)originalEffect);
			#endif
			return false;
		}
		return true;
    }

    static void addParticleEffect(DrawData &drawData, ParticleEffectType type, const Note &note, Score const &score, float effectDuration)
    {
		std::uniform_real_distribution<float> generator;
		std::array<float, 8> randomValues;
        int effectTypeId = static_cast<int>(type);
		const ParticleEffect& effect = ResourceManager::particleEffects[effectTypeId];
		float spawn_tm = accumulateDuration(note.tick, TICKS_PER_BEAT, score.tempoChanges);
		auto pStart = effect.particles.begin();
		if (std::isnan(effectDuration))
		{
			auto maxEffect = std::max_element(effect.particles.begin(), effect.particles.end(), [](const Particle& particleA, const Particle& particleB) { return (particleA.start + particleA.duration) < (particleB.start + particleB.duration); });
			effectDuration = std::min(1.f, maxEffect->start + maxEffect->duration) * particleEffectDuration[type];
		}
		for (size_t groupID = 0; groupID < effect.groupSizes.size(); groupID++)
		{
			auto [pBegin, pEnd] = std::equal_range(pStart, effect.particles.end(), (int)groupID, GroupIDComparer());
			if (pBegin == pEnd) continue;
			auto it = pBegin;
			for (size_t toSpawn = effect.groupSizes[groupID]; toSpawn != 0;)
			{
				if (it == pBegin)
					for (auto& val : randomValues)
						val = generator(drawData.rng);
				const auto& spawning = *it;
				auto const& props = spawning.xywhta;
				auto propsValue = spawning.compute(randomValues);
				std::array<DrawingParticleProperty, 6> xywhta;
				for (size_t i = 0; i < 6; i++) xywhta[i] = DrawingParticleProperty{propsValue[i], Engine::getEaseFunc(props[i].easing)};
				drawData.drawingParticles.push_back(DrawingParticle {
					effectTypeId,
					(int)std::distance(effect.particles.begin(), it),
					note.ID,
					Range{spawn_tm, spawn_tm + effectDuration},
					xywhta
				});
				if (++it == pEnd) {
					it = pBegin;
					toSpawn--;
				}
			}
			pStart = pEnd;
		}
	}

	static void addLaneEffect(DrawData& drawData, const Note& note, Score const &score)
	{
		if ((!note.critical || note.flick == FlickType::None) && (note.flick != FlickType::None || note.friction))
			return;
		ParticleEffectType effectType =
			note.critical ?
				note.flick != FlickType::None
				? ParticleEffectType::NoteCriticalFlickLane
				: ParticleEffectType::NoteCriticalLane
			: ParticleEffectType::NoteTapLane;
		if (!ensureValidParticle(effectType))
			return;
		if (note.getType() == NoteType::Hold)
		{
			// The lane effect is played a few tick after the note is hit
			// So if the hold switch to another step, use that instead
			auto& steps = score.holdNotes.at(note.ID).steps;
			if (steps.size())
			{
				auto it = std::lower_bound(steps.begin(), steps.end(), note.tick, [&](const HoldStep& step, int tick) { return step.type == HoldStepType::Hidden && (score.notes.at(step.ID).tick - tick) < 5; });
				if (it != steps.begin())
				{
					addParticleEffect(drawData, effectType, score.notes.at(std::prev(it)->ID), score);
					return;
				}
			}
		}
		addParticleEffect(drawData, effectType, note, score);
	}

	static void addHoldSegmentNoteEffect(DrawData &drawData, const Note &startNote, Score const &score)
	{		
		ParticleEffectType
			circular = !startNote.critical ? ParticleEffectType::NoteLongSegmentCircular : ParticleEffectType::NoteLongCriticalSegmentCircular,
			linear = !startNote.critical ? ParticleEffectType::NoteLongSegmentLinear : ParticleEffectType::NoteLongCriticalSegmentLinear;
		const Note& endNote = score.notes.at(score.holdNotes.at(startNote.ID).end);
		float startTime = accumulateDuration(startNote.tick, TICKS_PER_BEAT, score.tempoChanges);
		float endTime = accumulateDuration(endNote.tick, TICKS_PER_BEAT, score.tempoChanges);
		if (ensureValidParticle(circular))
			addParticleEffect(drawData, circular, startNote, score, endTime - startTime);
		if (ensureValidParticle(linear))
			addParticleEffect(drawData, linear, startNote, score, endTime - startTime);
	}

    static void addNoteEffect(DrawData &drawData, const Note &note, Score const &score)
    {
		ParticleEffectType circular = ParticleEffectType::Invalid, linear = ParticleEffectType::Invalid;
		switch (note.getType())
		{
			case NoteType::Hold:
			{
				HoldNoteType startStepType = score.holdNotes.at(note.ID).startType;
				if (startStepType != HoldNoteType::Guide)
					addHoldSegmentNoteEffect(drawData, note, score);
				if (startStepType == HoldNoteType::Normal)
					if (!note.friction)
						circular = !note.critical ? ParticleEffectType::NoteLongCircular : ParticleEffectType::NoteLongCriticalCircular;
					else
						circular = !note.critical ? ParticleEffectType::NoteFrictionCircular : ParticleEffectType::NoteFrictionCriticalCircular;
				linear = static_cast<ParticleEffectType>(static_cast<int>(circular) + 1);
				break;
			}
			case NoteType::HoldMid:
			{
				auto& holdNotes = score.holdNotes.at(note.parentID);
				// Don't need to check for index bound
				auto& holdStep = holdNotes.steps[findHoldStep(holdNotes, note.ID)];
				if (holdStep.type == HoldStepType::Hidden)
					return;
				circular = !note.critical ? ParticleEffectType::NoteLongAmongCircular : ParticleEffectType::NoteLongAmongCriticalCircular;
				break;
			}
			case NoteType::HoldEnd:
				if (score.holdNotes.at(note.parentID).endType != HoldNoteType::Normal)
					break;
			case NoteType::Tap:
				if (!note.friction)
					if (note.flick == FlickType::None)
						circular = !note.critical ? ParticleEffectType::NoteTapCircular : ParticleEffectType::NoteCriticalCircular;
					else
						circular = !note.critical ? ParticleEffectType::NoteFlickCircular : ParticleEffectType::NoteCriticalFlickCircular;
				else
					if (note.flick == FlickType::None)
						circular = !note.critical ? ParticleEffectType::NoteFrictionCircular : ParticleEffectType::NoteFrictionCriticalCircular;
					else
						break;
				linear = static_cast<ParticleEffectType>(static_cast<int>(circular) + 1);
				break;
			default:
				return;
		}
		if (ensureValidParticle(circular))
			addParticleEffect(drawData, circular, note, score);
		if (ensureValidParticle(linear))
			addParticleEffect(drawData, linear, note, score);
		if (note.isFlick())
		{
			ParticleEffectType directional = !note.critical ? ParticleEffectType::NoteFlickDirectional : ParticleEffectType::NoteCriticalDirectional;
			addParticleEffect(drawData, directional, note, score);
		}
    }
}