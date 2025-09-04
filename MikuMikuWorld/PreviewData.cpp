#include <stack>
#include <stdexcept>
#include "PreviewData.h"
#include "ApplicationConfiguration.h"
#include "Constants.h"
#include "ResourceManager.h"
#include "ScoreContext.h"

namespace MikuMikuWorld::Engine
{
	std::default_random_engine rng;

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
	static void addParticleEffect(DrawingEffect &drawList, std::default_random_engine &rng, ParticleEffectType type, DrawingParticleType drawType, const Note &note, Score const &score, float effectDuration = NAN);
	static void addLaneEffect(DrawData& drawData, DrawingEffect& effect, const Note& note, Score const &score);
	static void addNoteEffect(DrawData& drawData, DrawingEffect& effect, const Note& note, Score const &score);
	static void addSlotEffect(DrawData& drawData, DrawingEffect& effect, const Note& note, Score const &score);

	void DrawData::calculateDrawData(Score const &score)
	{
		this->clear();
		try
		{
			this->noteSpeed = config.pvNoteSpeed;
			this->hasLaneEffect = config.pvLaneEffect;
			this->hasNoteEffect = config.pvNoteEffect;
			this->hasSlotEffect = config.pvNoteGlow;
			std::map<Range, Range, TargetTimeComparer> simBuilder;
			for (auto rit = score.notes.rbegin(), rend = score.notes.rend(); rit != rend; rit++)
			{
				auto& [id, note] = *rit;
				maxTicks = std::max(note.tick, maxTicks);
				NoteType type = note.getType();
				bool hidden = false;
				if (type == NoteType::Hold)
					hidden = score.holdNotes.at(id).startType != HoldNoteType::Normal;
				else if (type == NoteType::HoldEnd)
					hidden = score.holdNotes.at(note.parentID).endType != HoldNoteType::Normal;
				if (hidden)
					continue;
				if (type == NoteType::HoldMid)
					continue;
					
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
				for (const HoldStep* head = &holdNote.start, *tail; head != nullptr; head = tail)
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
					if (it != end) ++it;
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
		drawingEffects.clear();
		maxTicks = 1;
		rng.seed((uint32_t)time(nullptr));
	}

	static bool ensureValidParticle(ParticleEffectType &effectType)
	{
		return effectType != ParticleEffectType::Invalid;
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

	static void addParticleEffect(DrawingEffect &drawList, std::default_random_engine &rng, ParticleEffectType type, DrawingParticleType drawType, const Note &note, Score const &score, float effectDuration)
	{
		std::uniform_real_distribution<float> generator;
		std::array<float, 8> randomValues;
		int effectTypeId = static_cast<int>(type);
		ParticleEffect& effect = ResourceManager::particleEffects[effectTypeId];
		static int textureId = ResourceManager::getTexture("particles");
		static const std::vector<Sprite>* spriteList = textureId >= 0 ? &ResourceManager::textures[textureId].sprites : nullptr;
		if (!spriteList) return;
		float spawn_tm = accumulateDuration(note.tick, TICKS_PER_BEAT, score.tempoChanges);
		auto pStart = effect.particles.begin();
		if (std::isnan(effectDuration))
		{
			auto maxEffect = std::max_element(effect.particles.begin(), effect.particles.end(), [](const Particle& particleA, const Particle& particleB) { return (particleA.start + particleA.duration) < (particleB.start + particleB.duration); });
			effectDuration = std::min(1.f, maxEffect->start + maxEffect->duration) * particleEffectDuration[type];
		}

		bool isDyanmicSizeEffect = type == ParticleEffectType::NoteTapLinearAdd ||
			type == ParticleEffectType::NoteCriticalLinearAdd ||
			type == ParticleEffectType::NoteLongLinearAdd ||
			type == ParticleEffectType::NoteLongCriticalLinearAdd;

		for (size_t groupID = 0; groupID < effect.groupSizes.size(); groupID++)
		{
			auto [pBegin, pEnd] = std::equal_range(pStart, effect.particles.end(), (int)groupID, GroupIDComparer());
			if (pBegin == pEnd) continue;
			auto it = pBegin;

			size_t count = effect.groupSizes[groupID];
			if (isDyanmicSizeEffect)
				count *= note.width;

			for (size_t toSpawn = count; toSpawn != 0;)
			{
				if (it == pBegin)
				{
					for (auto& val : randomValues)
						val = generator(rng);
				}

				if (isDyanmicSizeEffect)
				{
					it->xyCoeff.sinr5_8->r->m128_f32[0] = note.width * 0.5f;
					it->xyCoeff.sinr5_8->r->m128_f32[1] = note.width * 0.5f;
				}

				if (isArrayIndexInBounds(it->spriteID, *spriteList))
				{
					const auto& spawning = *it;
					auto const& props = spawning.xywhta;
					auto propsValue = spawning.compute(randomValues);
					std::array<DrawingParticleProperty, 6> xywhta;
					for (size_t i = 0; i < 6; i++) xywhta[i] = DrawingParticleProperty{propsValue[i], Engine::getEaseFunc(props[i].easing)};
					
					Range time{ spawn_tm, spawn_tm + effectDuration };
					drawList.time.min = std::min(drawList.time.min, time.min);
					drawList.time.max = std::max(drawList.time.max, time.max);
					DrawingParticle drawingParticle{
						effectTypeId,
						(int)std::distance(effect.particles.begin(), it),
						time,
						xywhta,
						drawType };

					switch (drawType)
					{
					case DrawingParticleType::Slot:
						drawList.slotParticle = std::make_unique<DrawingParticle>(drawingParticle);
						break;
					case DrawingParticleType::Lane:
						drawList.laneParticles.emplace_back(std::move(drawingParticle));
						break;
					case DrawingParticleType::BlendCircular:
						drawList.blendParticles.emplace_back(std::move(drawingParticle));
						break;
					default:
						drawList.additiveAlphaBlendParticles.emplace_back(std::move(drawingParticle));
					}
				}

				if (++it == pEnd) {
					it = pBegin;
					toSpawn--;
				}
			}
			pStart = pEnd;
		}
	}

	static void addLaneEffect(DrawData& drawData, DrawingEffect& effect, const Note& note, Score const &score)
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
					addParticleEffect(effect, rng, effectType, DrawingParticleType::Lane, score.notes.at(std::prev(it)->ID), score);
					return;
				}
			}
		}
		addParticleEffect(effect, rng, effectType, DrawingParticleType::Lane, note, score);
	}

	static void addHoldSegmentNoteEffect(DrawData &drawData, DrawingEffect& effect, const Note &startNote, Score const &score)
	{		
		ParticleEffectType
			circular = !startNote.critical ? ParticleEffectType::NoteLongSegmentCircular : ParticleEffectType::NoteLongCriticalSegmentCircular,
			circularEx = !startNote.critical ? ParticleEffectType::NoteLongSegmentCircularEx : ParticleEffectType::NoteLongCriticalSegmentCircularEx,
			linear = !startNote.critical ? ParticleEffectType::NoteLongSegmentLinear : ParticleEffectType::NoteLongCriticalSegmentLinear;
		const Note& endNote = score.notes.at(score.holdNotes.at(startNote.ID).end);
		float startTime = accumulateDuration(startNote.tick, TICKS_PER_BEAT, score.tempoChanges);
		float endTime = accumulateDuration(endNote.tick, TICKS_PER_BEAT, score.tempoChanges);
		if (ensureValidParticle(circular))
			addParticleEffect(effect, rng, circular, DrawingParticleType::BlendCircular, startNote, score, endTime - startTime);
		if (ensureValidParticle(circularEx))
			addParticleEffect(effect, rng, circularEx, DrawingParticleType::Linear, startNote, score, endTime - startTime);
		if (ensureValidParticle(linear))
			addParticleEffect(effect, rng, linear, DrawingParticleType::Linear, startNote, score, endTime - startTime);
	}

	static void addNoteEffect(DrawData &drawData, DrawingEffect& effect, const Note &note, Score const &score)
	{
		ParticleEffectType
			circular = ParticleEffectType::Invalid,
			linear = ParticleEffectType::Invalid,
			linearAdd = ParticleEffectType::Invalid;

		switch (note.getType())
		{
		case NoteType::Hold:
		{
			HoldNoteType startStepType = score.holdNotes.at(note.ID).startType;
			if (startStepType != HoldNoteType::Guide)
				addHoldSegmentNoteEffect(drawData, effect, note, score);
			if (startStepType != HoldNoteType::Normal)
				break;
			if (!note.friction)
			{
				circular = !note.critical ? ParticleEffectType::NoteLongCircular : ParticleEffectType::NoteLongCriticalCircular;
				linearAdd = !note.critical ? ParticleEffectType::NoteLongLinearAdd : ParticleEffectType::NoteLongCriticalLinearAdd;
			}
			else
			{
				circular = !note.critical ? ParticleEffectType::NoteFrictionCircular : ParticleEffectType::NoteFrictionCriticalCircular;
			}
			linear = static_cast<ParticleEffectType>(static_cast<int>(circular) + 1);
			if (note.critical && !note.friction)
				linear = ParticleEffectType::NoteCriticalLinear;
			
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
			{
				linearAdd = note.critical ? ParticleEffectType::NoteCriticalLinearAdd : ParticleEffectType::NoteTapLinearAdd;
				if (!note.isFlick())
				{
					if (note.getType() == NoteType::HoldEnd)
					{
						circular = !note.critical ? ParticleEffectType::NoteLongCircular : ParticleEffectType::NoteLongCriticalCircular;
						linearAdd = note.critical ? ParticleEffectType::NoteLongCriticalLinearAdd : ParticleEffectType::NoteLongLinearAdd;
					}
					else
					{
						circular = !note.critical ? ParticleEffectType::NoteTapCircular : ParticleEffectType::NoteCriticalCircular;
					}
				}
				else
				{
					circular = !note.critical ? ParticleEffectType::NoteFlickCircular : ParticleEffectType::NoteCriticalFlickCircular;
				}
			}
			else
			{
				if (!note.isFlick())
					circular = !note.critical ? ParticleEffectType::NoteFrictionCircular : ParticleEffectType::NoteFrictionCriticalCircular;
				else
					break;
			}
			linear = static_cast<ParticleEffectType>(static_cast<int>(circular) + 1);
			if (note.isFlick() && note.critical && !note.friction)
				linear = ParticleEffectType::NoteCriticalFlickLinear;
			else if (note.getType() == NoteType::HoldEnd && !note.isFlick() && !note.friction && note.critical)
				linear = ParticleEffectType::NoteCriticalLinear;

			break;
		default:
			return;
		}

		if (ensureValidParticle(circular))
			switch(circular)
			{
			case ParticleEffectType::NoteLongAmongCircular: 
			case ParticleEffectType::NoteLongAmongCriticalCircular:
			case ParticleEffectType::NoteFrictionCircular:
			case ParticleEffectType::NoteFrictionCriticalCircular:
				addParticleEffect(effect, rng, circular, DrawingParticleType::Flat, note, score);
				break;
			default:
				addParticleEffect(effect, rng, circular, DrawingParticleType::Circular, note, score);
				break;
			}
		if (ensureValidParticle(linear))
			addParticleEffect(effect, rng, linear, DrawingParticleType::Linear, note, score);
		if (ensureValidParticle(linearAdd))
			addParticleEffect(effect, rng, linearAdd, DrawingParticleType::Linear, note, score);
		if (note.isFlick())
		{
			ParticleEffectType directional = !note.critical ? ParticleEffectType::NoteFlickDirectional : ParticleEffectType::NoteCriticalDirectional;
			addParticleEffect(effect, rng, directional, DrawingParticleType::Linear, note, score);
		}
	}

	void addSlotEffect(DrawData &drawData, DrawingEffect& effect, const Note &note, Score const &score)
	{
		ParticleEffectType slot = ParticleEffectType::Invalid, slotGlow = ParticleEffectType::Invalid;
		switch (note.getType())
		{
		case NoteType::Hold:
		{
			const auto& holdNote = score.holdNotes.at(note.ID);
			if (holdNote.startType != HoldNoteType::Guide)
			{
				ParticleEffectType slotSegmentGlow = !note.critical ? ParticleEffectType::SlotGlowNoteLongSegment : ParticleEffectType::SlotGlowNoteLongCriticalSegment;
				const Note& endNote = score.notes.at(holdNote.end);
				float startTime = accumulateDuration(note.tick, TICKS_PER_BEAT, score.tempoChanges);
				float endTime = accumulateDuration(endNote.tick, TICKS_PER_BEAT, score.tempoChanges);
				addParticleEffect(effect, rng, slotSegmentGlow, DrawingParticleType::SlotGlow, note, score, endTime - startTime);
			}
			if (note.friction) return;
			if (holdNote.startType == HoldNoteType::Normal)
			{
				slot = !note.critical ? ParticleEffectType::SlotNoteLong : ParticleEffectType::SlotNoteCritical;
				slotGlow = !note.critical ? ParticleEffectType::SlotGlowNoteLong : ParticleEffectType::SlotGlowNoteCritical;
			}
			break;
		}
		case NoteType::HoldEnd:
			if (note.friction) return;
			if (score.holdNotes.at(note.parentID).endType == HoldNoteType::Normal)
			{
				slot = !note.critical ?
					note.flick != FlickType::None
					? ParticleEffectType::SlotNoteFlick
					: ParticleEffectType::SlotNoteLong
				: ParticleEffectType::SlotNoteCritical;
				slotGlow = !note.critical ?
					note.flick != FlickType::None
					? ParticleEffectType::SlotGlowNoteFlick
					: ParticleEffectType::SlotGlowNoteLong
				: ParticleEffectType::SlotGlowNoteCritical;
			}
			break;
		case NoteType::Tap:
			if (note.friction) return;
			slot = !note.critical 
				? note.flick != FlickType::None
					? ParticleEffectType::SlotNoteFlick
					: ParticleEffectType::SlotNoteTap
				: ParticleEffectType::SlotNoteCritical;
			slotGlow = static_cast<ParticleEffectType>(static_cast<int>(slot) + 4);
			break;
		default:
		case NoteType::HoldMid:
			return;
		}

		if (note.critical && note.isFlick())
			slotGlow = ParticleEffectType::SlotGlowNoteCriticalFlick;

		if (ensureValidParticle(slot))
			addParticleEffect(effect, rng, slot, DrawingParticleType::Slot, note, score);
		if (ensureValidParticle(slotGlow))
			addParticleEffect(effect, rng, slotGlow, DrawingParticleType::SlotGlow, note, score);
	}

	void DrawData::updateNoteEffects(ScoreContext& context)
	{
		const float currentTick = context.currentTick;
		const float currentTime = context.getTimeAtCurrentTick();
		constexpr float effectTimeAddBefore = 0;
		constexpr float effectTimeAddAfter = 0.75f;
		constexpr float effectTimeRemoveBefore = 0.5f;
		constexpr float effectTimeRemoveAfter = 0.8f;

		for (auto it = context.score.notes.rbegin(); it != context.score.notes.rend(); it++)
		{
			const auto& [id, note] = *it;
			auto effectIt = drawingEffects.find(id);
			if (effectIt != drawingEffects.end())
			{
				if (!isWithinRange(currentTime, effectIt->second.time.min - effectTimeRemoveBefore, effectIt->second.time.max + effectTimeRemoveAfter))
					drawingEffects.erase(effectIt);

				continue;
			}

			bool isMidHold = false;
			if (note.getType() == NoteType::Hold)
			{
				const HoldNote& hold = context.score.holdNotes.at(note.ID);
				const Note& end = context.score.notes.at(hold.end);
				isMidHold = !hold.isGuide() && isWithinRange(currentTick, note.tick, end.tick);
			}

			const float noteTime = accumulateDuration(note.tick, TICKS_PER_BEAT, context.score.tempoChanges);
			if (isMidHold || isWithinRange(currentTime, noteTime - effectTimeAddBefore, noteTime + effectTimeAddAfter))
			{
				DrawingEffect drawingEffect{ id, {INT32_MAX, 0}, {} };
				drawingEffect.additiveAlphaBlendParticles.reserve(192);
				drawingEffect.blendParticles.reserve(24);

				if (hasNoteEffect)
					addNoteEffect(*this, drawingEffect, note, context.score);
				if (hasSlotEffect)
					addSlotEffect(*this, drawingEffect, note, context.score);

				bool noteHasLaneEffect = true;
				const NoteType type = note.getType();
				switch (type)
				{
				case NoteType::Hold:
					noteHasLaneEffect = context.score.holdNotes.at(id).startType == HoldNoteType::Normal;
					break;
				case NoteType::HoldEnd:
					noteHasLaneEffect = context.score.holdNotes.at(note.parentID).endType == HoldNoteType::Normal;
					break;
				case NoteType::HoldMid:
					noteHasLaneEffect = false;
					break;
				default:
					break;
				}

				if (hasLaneEffect && noteHasLaneEffect)
					addLaneEffect(*this, drawingEffect, note, context.score);

				drawingEffects[id] = std::move(drawingEffect);
			}
		}
	}
}