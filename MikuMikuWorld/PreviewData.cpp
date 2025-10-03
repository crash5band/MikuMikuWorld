#include <queue>
#include <stdexcept>
#include "PreviewData.h"
#include "ApplicationConfiguration.h"
#include "Constants.h"
#include "ResourceManager.h"
#include "ScoreContext.h"

namespace MikuMikuWorld::Engine
{
	std::default_random_engine rng;

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

	struct DrawingHoldStep
	{
		int tick;
		double time;
		float left;
		float right;
		EaseType ease;
	};

	static void addHoldNote(DrawData& drawData, const HoldNote& holdNote, Score const &score);
	static bool ensureValidParticle(ParticleEffectType& demandType);
	static void addParticleEffect(DrawData& drawData, ParticleEffectType type, NoteEffectType noteEffectType, DrawingParticleType drawType, const Note &note, Score const &score, float effectDuration = NAN);
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
			std::map<int, Range> simBuilder;
			for (auto rit = score.notes.rbegin(), rend = score.notes.rend(); rit != rend; rit++)
			{
				auto& [id, note] = *rit;
				maxTicks = std::max(note.tick, maxTicks);
				NoteType type = note.getType();
				if (type == NoteType::HoldMid
					|| (type == NoteType::Hold && score.holdNotes.at(id).startType != HoldNoteType::Normal)
					|| (type == NoteType::HoldEnd && score.holdNotes.at(note.parentID).endType != HoldNoteType::Normal))
					continue;
				if (type == NoteType::HoldMid)
					continue;
					
				auto visual_tm = getNoteVisualTime(note, score, noteSpeed);
				drawingNotes.push_back(DrawingNote{note.ID, visual_tm});

				// Find the max and min lane within the same height (visual_tm.max)
				float center = getNoteCenter(note);
				auto&& [it, has_emplaced] = simBuilder.try_emplace(note.tick, Range{center, center});
				auto& x_range = it->second;
				if (has_emplaced)
					continue;
				if (center < x_range.min)
					x_range.min = center;
				if (center > x_range.max)
					x_range.max = center;
				continue;
			}

			float noteDuration = getNoteDuration(noteSpeed);
			for (const auto& [line_tick, x_range] : simBuilder)
			{
				if (x_range.min != x_range.max)
				{
					double targetTime = accumulateScaledDuration(line_tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
					drawingLines.push_back(DrawingLine{ x_range, Range{ targetTime - getNoteDuration(noteSpeed), targetTime } });
				}
			}

			for (auto rit = score.holdNotes.rbegin(), rend = score.holdNotes.rend(); rit != rend; rit++)
			{
				addHoldNote(*this, rit->second, score);
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
		drawingNoteEffects.clear();
		clearEffectPools();

		maxTicks = 1;
		rng.seed((uint32_t)time(nullptr));
	}

	void DrawData::clearEffectPools()
	{
		for (auto& [type, pool] : normalEffectsPools)
			pool.clear();

		for (auto& [type, pool] : criticalEffectsPools)
			pool.clear();

		drawingNoteEffects.clear();
	}

	void addHoldNote(DrawData &drawData, const HoldNote &holdNote, Score const &score)
	{
		float noteDuration = getNoteDuration(drawData.noteSpeed);
		const Note& startNote = score.notes.at(holdNote.start.ID), endNote = score.notes.at(holdNote.end);
		float activeTime = accumulateDuration(startNote.tick, TICKS_PER_BEAT, score.tempoChanges);
		float startTime = activeTime;
		DrawingHoldStep head = {
			startNote.tick,
			accumulateScaledDuration(startNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges),
			Engine::laneToLeft(startNote.lane),
			Engine::laneToLeft(startNote.lane) + startNote.width,
			holdNote.start.ease
		};
		for (ptrdiff_t headIdx = -1, tailIdx = 0, stepSz = holdNote.steps.size(); headIdx < stepSz; ++tailIdx)
		{
			if (tailIdx < stepSz && holdNote.steps[tailIdx].type == HoldStepType::Skip)
				continue;
			HoldStep tailStep = tailIdx == stepSz ? HoldStep{ holdNote.end, HoldStepType::Hidden } : holdNote.steps[tailIdx];
			const Note& tailNote = score.notes.at(tailStep.ID);
			auto easeFunction = getEaseFunction(head.ease);
			DrawingHoldStep tail = {
				tailNote.tick,
				accumulateScaledDuration(tailNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges),
				Engine::laneToLeft(tailNote.lane),
				Engine::laneToLeft(tailNote.lane) + tailNote.width,
				tailStep.ease
			};
			float endTime = accumulateDuration(tailNote.tick, TICKS_PER_BEAT, score.tempoChanges);
			drawData.drawingHoldSegments.push_back(DrawingHoldSegment {
				holdNote.end, 
				head.ease,
				holdNote.isGuide(),
				tailIdx,
				head.time, tail.time,
				head.left, head.right,
				tail.left, tail.right,
				startTime, endTime,
				activeTime,
			});
			startTime = endTime;
			while ((headIdx + 1) < tailIdx)
			{
				const HoldStep& skipStep = holdNote.steps[headIdx + 1];
				assert(skipStep.type == HoldStepType::Skip);
				const Note& skipNote = score.notes.at(skipStep.ID);
				if (skipNote.tick > tail.tick)
					break;
				double tickTime = accumulateScaledDuration(skipNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
				double tick_t = unlerpD(head.time, tail.time, tickTime);
				float skipLeft = easeFunction(head.left, tail.left, tick_t);
				float skipRight = easeFunction(head.right, tail.right, tick_t);
				drawData.drawingHoldTicks.push_back(DrawingHoldTick{
					skipStep.ID,
					skipLeft + (skipRight - skipLeft) / 2,
					Range{tickTime - noteDuration, tickTime}
				});
				++headIdx;
			}
			if (tailStep.type != HoldStepType::Hidden)
			{
				double tickTime = accumulateScaledDuration(tailNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
				drawData.drawingHoldTicks.push_back(DrawingHoldTick{
					tailNote.ID,
					getNoteCenter(tailNote),
					{tickTime - noteDuration, tickTime}
				});
			}
			head = tail;
			++headIdx;
		}
	}

	static bool ensureValidParticle(ParticleEffectType &effectType)
	{
		return effectType != ParticleEffectType::Invalid && ResourceManager::particleEffects[(size_t)effectType].particles.size() != 0;
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

	static void addParticleEffect(DrawData& drawData, ParticleEffectType type, NoteEffectType noteEffectType, DrawingParticleType drawType, const Note &note, Score const &score, float effectDuration)
	{
		std::uniform_real_distribution<float> generator;
		std::array<float, 8> randomValues;
		int effectTypeId = static_cast<int>(type);
		ParticleEffect& effect = ResourceManager::particleEffects[effectTypeId];
		static int textureId = ResourceManager::getTexture("particles");
		static const std::vector<Sprite>* spriteList = textureId >= 0 ? &ResourceManager::textures[textureId].sprites : nullptr;
		if (!spriteList) return;
		float spawn_tm = accumulateDuration(note.tick, TICKS_PER_BEAT, score.tempoChanges);
		if (std::isnan(effectDuration))
		{
			auto maxEffect = std::max_element(effect.particles.begin(), effect.particles.end(), [](const Particle& particleA, const Particle& particleB) { return (particleA.start + particleA.duration) < (particleB.start + particleB.duration); });
			effectDuration = std::min(1.f, maxEffect->start + maxEffect->duration) * particleEffectDuration[type];
		}

		bool isRepeatEffect =
			noteEffectType == NoteEffectType::Aura ||
			noteEffectType == NoteEffectType::AuraBg ||
			noteEffectType == NoteEffectType::Lane ||
			noteEffectType == NoteEffectType::Slot;

		std::map<NoteEffectType, EffectPool>& effectPoolMap = note.critical ? drawData.criticalEffectsPools : drawData.normalEffectsPools;
		EffectPool& effectPool = effectPoolMap.at(noteEffectType);

		size_t effectCount = isRepeatEffect ? note.lane + note.width : note.lane + 1;
		for (size_t i = note.lane; i < effectCount; i++)
		{
			int effectIndex = effectPool.findIndex(note.ID, i);
			DrawingEffect& drawingEffect = effectIndex > -1 ?
				effectPool.pool[effectIndex] : effectPool.getNext(note.ID, true);

			if (effectIndex == -1)
			{
				drawingEffect.refID = note.ID;
				drawingEffect.lane = i;
				drawingEffect.time = { FLT_MAX, FLT_MIN };
				drawingEffect.particles.clear();
			}

			auto pStart = effect.particles.begin();
			for (size_t groupID = 0; groupID < effect.groupSizes.size(); groupID++)
			{
				auto [pBegin, pEnd] = std::equal_range(pStart, effect.particles.end(), (int)groupID, GroupIDComparer());
				if (pBegin == pEnd) continue;
				auto it = pBegin;

				size_t count = effect.groupSizes[groupID];
				for (size_t toSpawn = count; toSpawn != 0;)
				{
					if (it == pBegin)
					{
						for (auto& val : randomValues)
							val = generator(rng);
					}

					if (isArrayIndexInBounds(it->spriteID, *spriteList))
					{
						const auto& spawning = *it;
						auto const& props = spawning.xywhtau1u2;
						auto propsValue = spawning.compute(randomValues);
						std::array<DrawingParticleProperty, 8> xywhtau1u2;
						for (size_t i = 0; i < xywhtau1u2.size(); i++) xywhtau1u2[i] = DrawingParticleProperty{propsValue[i], Engine::getEaseFunc(props[i].easing)};
					
						Range time{ spawn_tm, spawn_tm + effectDuration };
						drawingEffect.time.min = std::min(drawingEffect.time.min, time.min);
						drawingEffect.time.max = std::max(drawingEffect.time.max, time.max);
						DrawingParticle drawingParticle {
							effectTypeId,
							(int)std::distance(effect.particles.begin(), it),
							time,
							xywhtau1u2,
							drawType
						};

						drawingEffect.particles.emplace_back(std::move(drawingParticle));
					}

					if (++it == pEnd) {
						it = pBegin;
						toSpawn--;
					}
				}
				pStart = pEnd;
			}
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
					addParticleEffect(drawData, effectType, NoteEffectType::Lane, DrawingParticleType::Lane, score.notes.at(std::prev(it)->ID), score);
					return;
				}
			}
		}
		addParticleEffect(drawData, effectType, NoteEffectType::Lane, DrawingParticleType::Lane, note, score);
	}

	static void addHoldSegmentNoteEffect(DrawData &drawData, const Note &startNote, Score const &score)
	{		
		ParticleEffectType
			circular = !startNote.critical ? ParticleEffectType::NoteLongSegmentCircular : ParticleEffectType::NoteLongCriticalSegmentCircular,
			circularEx = !startNote.critical ? ParticleEffectType::NoteLongSegmentCircularEx : ParticleEffectType::NoteLongCriticalSegmentCircularEx,
			linear = !startNote.critical ? ParticleEffectType::NoteLongSegmentLinear : ParticleEffectType::NoteLongCriticalSegmentLinear;
		const Note& endNote = score.notes.at(score.holdNotes.at(startNote.ID).end);
		float startTime = accumulateDuration(startNote.tick, TICKS_PER_BEAT, score.tempoChanges);
		float endTime = accumulateDuration(endNote.tick, TICKS_PER_BEAT, score.tempoChanges);
		if (endTime - startTime < 0.01f)
			return;

		if (ensureValidParticle(circular))
			addParticleEffect(drawData, circular, NoteEffectType::GenHold, DrawingParticleType::Circular, startNote, score, endTime - startTime);
		if (ensureValidParticle(circularEx))
			addParticleEffect(drawData, circularEx, NoteEffectType::GenHold, DrawingParticleType::Linear, startNote, score, endTime - startTime);
		if (ensureValidParticle(linear))
			addParticleEffect(drawData, linear, NoteEffectType::AuraHold, DrawingParticleType::Linear, startNote, score, endTime - startTime);
	}

	static void addNoteEffect(DrawData &drawData, const Note &note, Score const &score)
	{
		bool hasEffect = true;
		if (note.getType() == NoteType::Hold)
		{
			HoldNoteType holdType = score.holdNotes.at(note.ID).startType;
			if (holdType != HoldNoteType::Guide)
				addHoldSegmentNoteEffect(drawData, note, score);
			hasEffect = (holdType == HoldNoteType::Normal);
		}
		else if (note.getType() == NoteType::HoldEnd)
		{
			HoldNoteType holdType = score.holdNotes.at(note.parentID).endType;
			hasEffect = (holdType == HoldNoteType::Normal);
		}

		ParticleEffectType
			circular = ParticleEffectType::Invalid,
			linear = ParticleEffectType::Invalid,
			linearAdd = ParticleEffectType::Invalid;

		if (note.getType() == NoteType::HoldMid)
		{
			auto& holdNotes = score.holdNotes.at(note.parentID);
			// Don't need to check for index bound
			auto& holdStep = holdNotes.steps[findHoldStep(holdNotes, note.ID)];
			if (holdStep.type == HoldStepType::Hidden)
				return;
			circular = !note.critical ? ParticleEffectType::NoteLongAmongCircular : ParticleEffectType::NoteLongAmongCriticalCircular;
		}
		else if (hasEffect)
		{
			constexpr auto toCrit = [](ParticleEffectType effect)
			{
				return static_cast<ParticleEffectType>(static_cast<int>(effect) + static_cast<int>(ParticleEffectType::NoteCriticalCircular) - static_cast<int>(ParticleEffectType::NoteTapCircular));
			};
			if (!note.friction)
			{
				bool isTap = note.getType() == NoteType::Tap || note.isFlick();
				linearAdd = isTap ? ParticleEffectType::NoteTapLinearAdd : ParticleEffectType::NoteLongLinearAdd;
				if (note.isFlick())
					circular = ParticleEffectType::NoteFlickCircular;
				else
					circular = isTap ? ParticleEffectType::NoteTapCircular : ParticleEffectType::NoteLongCircular;
			}
			else
			{
				if (note.flick == FlickType::None)
					circular = ParticleEffectType::NoteFrictionCircular;
				else
					circular = ParticleEffectType::Invalid;
			}
			if (circular != ParticleEffectType::Invalid)
			{
				linear = static_cast<ParticleEffectType>(static_cast<int>(circular) + 1);
				if (note.critical)
				{
					circular = toCrit(circular);
					linear = toCrit(linear);
					if (linearAdd != ParticleEffectType::Invalid)
						linearAdd = toCrit(linearAdd);

					linearAdd = note.critical && note.isFlick() ? ParticleEffectType::NoteCriticalFlickLinearAdd : linearAdd;
				}
			}
		}

		if (ensureValidParticle(circular))
			switch(circular)
			{
			case ParticleEffectType::NoteLongAmongCircular: 
			case ParticleEffectType::NoteLongAmongCriticalCircular:
			case ParticleEffectType::NoteFrictionCircular:
			case ParticleEffectType::NoteFrictionCriticalCircular:
				addParticleEffect(drawData, circular, NoteEffectType::Gen, DrawingParticleType::Flat, note, score);
				break;
			default:
				addParticleEffect(drawData, circular, NoteEffectType::Gen, DrawingParticleType::Circular, note, score);
				break;
			}
		if (ensureValidParticle(linear))
			addParticleEffect(drawData, linear, NoteEffectType::Gen, DrawingParticleType::Linear, note, score);
		if (ensureValidParticle(linearAdd))
			addParticleEffect(drawData, linearAdd, NoteEffectType::Aura, DrawingParticleType::Linear, note, score);
		if (note.isFlick())
		{
			ParticleEffectType directional = !note.critical ? ParticleEffectType::NoteFlickDirectional : ParticleEffectType::NoteCriticalDirectional;
			if (ensureValidParticle(directional))
				addParticleEffect(drawData, directional, NoteEffectType::Gen, DrawingParticleType::Linear, note, score);

      ParticleEffectType flare = ParticleEffectType::NoteCriticalFlickFlare;
			if (note.critical && ensureValidParticle(flare))
        addParticleEffect(drawData, ParticleEffectType::NoteCriticalFlickFlare, NoteEffectType::Gen, DrawingParticleType::Linear, note, score);
		}
	}

	void addSlotEffect(DrawData &drawData, const Note &note, Score const &score)
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
				addParticleEffect(drawData, slotSegmentGlow, NoteEffectType::AuraBgHold, DrawingParticleType::Aura, note, score, endTime - startTime);
			}
			if (note.friction) return;
			if (holdNote.startType == HoldNoteType::Normal)
			{
				slot = !note.critical ? ParticleEffectType::SlotNoteLong : ParticleEffectType::SlotNoteCritical;
				slotGlow = !note.critical ? ParticleEffectType::SlotGlowNoteLong : ParticleEffectType::SlotGlowNoteCriticalLong;
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
				: ParticleEffectType::SlotGlowNoteCriticalLong;
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
			addParticleEffect(drawData, slot, NoteEffectType::Slot, DrawingParticleType::Slot, note, score);
		if (ensureValidParticle(slotGlow))
			addParticleEffect(drawData, slotGlow, NoteEffectType::AuraBg, DrawingParticleType::Aura, note, score);
	}

	DrawingEffect& EffectPool::getNext(int noteId, bool advance)
	{
		DrawingEffect& effect = pool[nextIndex];

		if (advance)
			nextIndex = (nextIndex + 1) % pool.size();
		return effect;
	}

	void EffectPool::clear()
	{
		for (auto& effect : pool)
		{
			effect.time = { -1, -1 };
			effect.lane = -1;
			effect.refID = 0;
		}
	}

	int EffectPool::findIndex(int noteId, int lane)
	{
		for (int i = 0; i < pool.size(); i++)
		{
			if (pool[i].refID == noteId && pool[i].lane == lane)
				return i;
		}

		return -1;
	}

	void DrawData::initEffectPools()
	{
		for (int i = 0; i < static_cast<int>(NoteEffectType::NoteEffectTypeCount); i++)
		{
			NoteEffectType effectType = static_cast<NoteEffectType>(i);
			normalEffectsPools[effectType] = {};
			criticalEffectsPools[effectType] = {};
		}
	}

	void DrawData::updateNoteEffects(ScoreContext& context)
	{
		if (normalEffectsPools.empty() || criticalEffectsPools.empty())
			initEffectPools();

		const float currentTick = context.currentTick;
		const float currentTime = context.getTimeAtCurrentTick();
		constexpr float effectTimeAddBefore = 0;
		constexpr float effectTimeAddAfter = 0.75f;
		constexpr float effectTimeRemoveBefore = 0.5f;
		constexpr float effectTimeRemoveAfter = 0.8f;

		for (auto it = context.score.notes.rbegin(); it != context.score.notes.rend(); it++)
		{
			const auto& [id, note] = *it;
			const auto noteEffectIdIt = drawingNoteEffects.find(id);
			if (noteEffectIdIt != drawingNoteEffects.end())
				continue;

			bool isMidHold = false;
			if (note.getType() == NoteType::Hold)
			{
				const HoldNote& hold = context.score.holdNotes.at(note.ID);
				const Note& end = context.score.notes.at(hold.end);
				isMidHold = !hold.isGuide() && isWithinRange(currentTick, note.tick, end.tick);
			}

			if (isMidHold || isWithinRange(currentTick, note.tick - 64, note.tick + 1920))
			{
				if (hasNoteEffect)
					addNoteEffect(*this, note, context.score);
				if (hasSlotEffect)
					addSlotEffect(*this, note, context.score);

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
					addLaneEffect(*this, note, context.score);

				drawingNoteEffects.insert(id);
			}
		}
	}
}
