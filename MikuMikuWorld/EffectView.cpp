#include "EffectView.h"
#include "PreviewEngine.h"
#include "ResourceManager.h"
#include "ScoreContext.h"
#include "ApplicationConfiguration.h"

namespace MikuMikuWorld::Effect
{
	static const std::map<EffectType, int> effectPoolSizes =
	{
		{ fx_note_normal_gen, 6 },
		{ fx_note_critical_normal_gen, 6 },
		{ fx_note_flick_gen, 6 },
		{ fx_note_critical_flick_gen, 6 },
		{ fx_note_long_gen, 6 },
		{ fx_note_critical_long_gen, 6 },
		{ fx_note_flick_flash, 6 },
		{ fx_note_critical_flick_flash, 6 },
		{ fx_note_trace_aura, 6 },
		{ fx_note_critical_trace_aura, 6 },
		{ fx_note_long_hold_via_aura, 6 },
		{ fx_note_critical_long_hold_via_aura, 6 },
	};

	static float getEffectXPos(int lane, int width, bool flip)
	{
		if (flip)
			lane = MAX_LANE - lane - width + 1;
		return (lane - 6 + width / 2.f) * 0.84f;
	}

	static float getEffectNoteCenter(const Note& note, bool flip)
	{
		float lane = note.lane;
		if (flip)
			lane = MAX_LANE - lane - note.width + 1;
		return Engine::laneToLeft(lane) + note.width / 2.f;
	}

	static std::pair<float, float> getNoteBound(const Note& note, bool flip)
	{
		float left = Engine::laneToLeft(note.lane), right = left + note.width;
		if (flip)
			std::swap(left *= -1, right *= -1);
		return std::make_pair(left, right);
	}

	static std::pair<float, float> getHoldSegmentBound(const Note& note, const Score& score, int curTick)
	{
		const HoldNote& holdNotes = score.holdNotes.at(note.ID);
		auto curStepIt = std::lower_bound(holdNotes.steps.begin(), holdNotes.steps.end(), curTick, [&score](const HoldStep& step, int tick) { return score.notes.at(step.ID).tick < tick; });
		auto startStepIt = std::find_if(std::make_reverse_iterator(curStepIt), holdNotes.steps.rend(), std::mem_fn(&HoldStep::canEase));
		const HoldStep& startHoldStep = startStepIt == holdNotes.steps.rend() ? holdNotes.start : *startStepIt;

		const Note& startNote = startStepIt == holdNotes.steps.rend() ? note : score.notes.at(startStepIt->ID);
		if (startNote.tick == curTick) return getNoteBound(startNote, config.pvMirrorScore);
		auto [leftStart, rightStart] = getNoteBound(startNote, config.pvMirrorScore);

		auto end = std::find_if(curStepIt, holdNotes.steps.end(), std::mem_fn(&HoldStep::canEase));
		const Note& endNote = score.notes.at(end == holdNotes.steps.end() ? holdNotes.end : end->ID);
		if (endNote.tick == curTick) return getNoteBound(endNote, config.pvMirrorScore);
		auto [leftStop, rightStop] = getNoteBound(endNote, config.pvMirrorScore);
		auto easeFunc = getEaseFunction(startHoldStep.ease);

		float start_tm = accumulateDuration(startNote.tick, TICKS_PER_BEAT, score.tempoChanges);
		float end_tm = accumulateDuration(endNote.tick, TICKS_PER_BEAT, score.tempoChanges);
		float current_tm = accumulateDuration(curTick, TICKS_PER_BEAT, score.tempoChanges);
		float progress = unlerp(start_tm, end_tm, current_tm);

		return std::make_pair(
			easeFunc(leftStart, leftStop, progress),
			easeFunc(rightStart, rightStop, progress)
		);
	}

	static std::pair<float, float> getHoldStepBound(const Note& note, const Score& score)
	{
		auto& holdNotes = score.holdNotes.at(note.parentID);
		int curStepIdx = findHoldStep(holdNotes, note.ID);
		if (holdNotes.steps[curStepIdx].canEase())
			return getNoteBound(note, config.pvMirrorScore);

		int startStepIdx = curStepIdx;
		while (startStepIdx != 0 && !holdNotes.steps[startStepIdx - 1].canEase())
			startStepIdx--;
		const HoldStep& lastHoldStep = startStepIdx != 0 ? holdNotes.steps[startStepIdx - 1] : holdNotes.start;
		auto easeFunc = getEaseFunction(lastHoldStep.ease);

		const Note& startNote = score.notes.at(lastHoldStep.ID);
		auto [leftStart, rightStart] = getNoteBound(startNote, config.pvMirrorScore);

		auto it = std::find_if(holdNotes.steps.begin() + curStepIdx, holdNotes.steps.end(), std::mem_fn(&HoldStep::canEase));
		const Note& endNote = score.notes.at(it == holdNotes.steps.end() ? holdNotes.end : it->ID);
		auto [leftStop, rightStop] = getNoteBound(endNote, config.pvMirrorScore);

		float start_tm = accumulateDuration(startNote.tick, TICKS_PER_BEAT, score.tempoChanges);
		float end_tm = accumulateDuration(endNote.tick, TICKS_PER_BEAT, score.tempoChanges);
		float current_tm = accumulateDuration(note.tick, TICKS_PER_BEAT, score.tempoChanges);
		float progress = unlerp(start_tm, end_tm, current_tm);

		return std::make_pair(
			easeFunc(leftStart, leftStop, progress),
			easeFunc(rightStart, rightStop, progress)
		);
	}

	void ParticleController::play(const Note& note, float start, float end)
	{
		active = true;
		time.min = start;
		time.max = end;
		refID = note.ID;

		effectRoot.stop(true);
		effectRoot.start(start);
	}

	void EffectPool::setup(EffectType type, int count)
	{
		this->count = count;
		this->type = type;

		pool.reserve(count);
		pool.insert(pool.end(), count, {});

		const int particleId = ResourceManager::getRootParticleIdByName(effectNames[static_cast<int>(type)]);
		const Particle& ref = ResourceManager::getParticleEffect(particleId);
		const Transform transform{};
		for (auto& instance : pool)
		{
			instance.effectRoot = createEmitterFromParticle(particleId);
			instance.effectRoot.init(ref, transform);
		}
	}

	EmitterInstance EffectPool::createEmitterFromParticle(int particleId)
	{
		const Effect::Particle& p = ResourceManager::getParticleEffect(particleId);

		EmitterInstance emitter{};
		for (const int child : p.children)
		{
			emitter.children.emplace_back(createEmitterFromParticle(child));
		}

		return emitter;
	}

	void EffectView::update(const ScoreContext& context)
	{
		const float currentTick = context.currentTick;
		const float currentTime = context.getTimeAtCurrentTick();

		for (auto it = context.score.notes.rbegin(); it != context.score.notes.rend(); it++)
		{
			const auto& [id, note] = *it;
			if (isNoteEffectPlayed(id))
				continue;

			bool isMidHold = false;
			if (note.getType() == NoteType::Hold)
			{
				const HoldNote& hold = context.score.holdNotes.at(note.ID);
				const Note& end = context.score.notes.at(hold.end);
				isMidHold = !hold.isGuide() && isWithinRange(currentTick, note.tick, end.tick);
			}

			float noteTime = accumulateDuration(note.tick, TICKS_PER_BEAT, context.score.tempoChanges);
			if (isMidHold || isWithinRange(currentTime, noteTime - 0.02f, noteTime + 0.04f))
			{
				addNoteEffects(note, context, noteTime);
				playedEffectsNoteIds.insert(id);
			}
		}
	}

	void EffectView::init()
	{
		effectPools.clear();
		for (int i = 0; i < fx_count; i++)
		{
			EffectType type = static_cast<EffectType>(i);
			effectPools.insert_or_assign(type, EffectPool());

			EffectPool& effPool = effectPools[type];
			int size = 12;
			auto poolSizeIt = effectPoolSizes.find(type);

			if (poolSizeIt != effectPoolSizes.end())
				size = poolSizeIt->second;

			effPool.setup(type, size);
		}

		int texId = ResourceManager::getTexture("tex_note_common_all_v2");
		effectsTex = &ResourceManager::textures[texId];

		initialized = true;
	}

	void EffectView::reset()
	{
		for (auto& type : effectPools)
		{
			for (auto& controller : type.second.pool)
			{
				controller.active = false;
				controller.time = { -1, -1 };
				controller.effectRoot.stop(true);
			}
		}

		playedEffectsNoteIds.clear();
	}

	void EffectView::addNoteEffects(const Note& note, const ScoreContext& context, float time)
	{
		if (note.friction)
		{
			EffectType traceEffect{ fx_count };
			if (note.isFlick())
			{
				traceEffect = note.critical ? fx_note_critical_flick_flash : fx_note_flick_flash;
				if (note.critical)
					addLaneEffect(fx_lane_critical_flick, note, context, time);
			}
			else
			{
				traceEffect = note.critical ? fx_note_critical_trace_aura : fx_note_trace_aura;
			}

			addEffect(traceEffect, note, context, time);
			if (note.getType() == NoteType::Hold)
			{
				addEffect(note.critical ? fx_note_critical_long_hold_gen : fx_note_long_hold_gen, note, context, time);
				addAuraEffect(note.critical ? fx_note_critical_long_hold_gen_aura : fx_note_hold_aura, note, context, time);
			}
			return;
		}

		if (note.isFlick())
		{
			EffectType aura{ note.critical ? fx_note_critical_flick_aura : fx_note_flick_aura };
			EffectType gen{ note.critical ? fx_note_critical_flick_gen : fx_note_flick_gen };
			EffectType flash{ note.critical ? fx_note_critical_flick_flash : fx_note_flick_flash };

			addAuraEffect(aura, note, context, time);
			addEffect(gen, note, context, time);
			addEffect(flash, note, context, time);

			if (note.critical)
				addLaneEffect(fx_lane_critical_flick, note, context, time);

			return;
		}

		if (note.getType() == NoteType::Hold)
		{
			const HoldNote& hold = context.score.holdNotes.at(note.ID);
			if (hold.isGuide())
				return;

			if (hold.startType == HoldNoteType::Normal)
			{
				EffectType aura{ note.critical ? fx_note_critical_long_aura : fx_note_long_aura };
				EffectType gen{ note.critical ? fx_note_critical_long_gen : fx_note_long_gen };
				EffectType lane{ note.critical ? fx_lane_critical : fx_lane_default };
				
				addAuraEffect(aura, note, context, time);
				addEffect(gen, note, context, time);
				addLaneEffect(lane, note, context, time);
			}

			addEffect(note.critical ? fx_note_critical_long_hold_gen : fx_note_long_hold_gen, note, context, time);
			addAuraEffect(note.critical ? fx_note_critical_long_hold_gen_aura : fx_note_hold_aura, note, context, time);
		}
		else if (note.getType() == NoteType::HoldMid)
		{
			const HoldNote& hold = context.score.holdNotes.at(note.parentID);
			const HoldStep& step = hold.steps[findHoldStep(hold, note.ID)];

			if (step.type == HoldStepType::Hidden)
				return;

			addEffect(note.critical ? fx_note_critical_long_hold_via_aura : fx_note_long_hold_via_aura, note, context, time);
		}
		else if (note.getType() == NoteType::HoldEnd)
		{
			const HoldNote& hold = context.score.holdNotes.at(note.parentID);
			if (hold.isGuide())
				return;

			if (hold.endType == HoldNoteType::Normal)
			{
				EffectType aura{ note.critical ? fx_note_critical_long_aura : fx_note_long_aura };
				EffectType gen{ note.critical ? fx_note_critical_long_gen : fx_note_long_gen };
				EffectType lane{ note.critical ? fx_lane_critical : fx_lane_default };

				addAuraEffect(aura, note, context, time);
				addEffect(gen, note, context, time);
				addLaneEffect(lane, note, context, time);
			}
		}
		else
		{
			EffectType aura{ note.critical ? fx_note_critical_normal_aura : fx_note_normal_aura };
			EffectType gen{ note.critical ? fx_note_critical_normal_gen : fx_note_normal_gen };
			EffectType lane{ note.critical ? fx_lane_critical : fx_lane_default };

			addAuraEffect(aura, note, context, time);
			addEffect(gen, note, context, time);
			addLaneEffect(lane, note, context, time);
		}
	}

	void EffectView::addEffect(EffectType effect, const Note& note, const ScoreContext& context, float time)
	{
		EffectPool& pool = effectPools[effect];
		ParticleController& controller = pool.getNext();

		float xPos = getEffectNoteCenter(note, config.pvMirrorScore);
		float zRot = 0.f;
		float start = time;
		float end = -1;

		if (note.isFlick() && (effect == fx_note_flick_flash || effect == fx_note_critical_flick_flash))
		{
			switch (note.flick)
			{
			case FlickType::Left:
				zRot = 15.f;
				break;
			case FlickType::Right:
				zRot = -15.f;
				break;
			default:
				break;
			}

			if (config.pvMirrorScore) zRot *= -1;
		}
		else if (effect == fx_note_critical_long_hold_via_aura || effect == fx_note_long_hold_via_aura)
		{
			const HoldNote& hold = context.score.holdNotes.at(note.parentID);
			const HoldStep& step = hold.steps[findHoldStep(hold, note.ID)];

			float noteLeft{}, noteRight{};
			std::tie(noteLeft, noteRight) = getHoldStepBound(note, context.score);

			xPos = noteLeft + (noteRight - noteLeft) / 2;
		}
		else if (effect == fx_note_critical_long_hold_gen || effect == fx_note_long_hold_gen)
		{
			const HoldNote& holdNote = context.score.holdNotes.at(note.ID);
			start = accumulateDuration(note.tick, TICKS_PER_BEAT, context.score.tempoChanges);
			end = accumulateDuration(context.score.notes.at(holdNote.end).tick, TICKS_PER_BEAT, context.score.tempoChanges);
			if (abs(end - start) < 0.01f)
				return;

			float noteLeft{}, noteRight{};
			std::tie(noteLeft, noteRight) = getHoldSegmentBound(note, context.score, context.currentTick);

			xPos = noteLeft + (noteRight - noteLeft) / 2;
		}

		controller.worldOffset.position = DirectX::XMVectorSetX(controller.worldOffset.position, xPos * 0.84f);
		controller.worldOffset.rotation = DirectX::XMVectorSetZ(controller.worldOffset.rotation, zRot);
		controller.play(note, start, end);
	}

	void EffectView::addAuraEffect(EffectType effect, const Note& note, const ScoreContext& context, float time)
	{
		if (effect == fx_note_hold_aura || effect == fx_note_critical_long_hold_gen_aura)
		{
			const HoldNote& holdNote = context.score.holdNotes.at(note.ID);
			float start = accumulateDuration(note.tick, TICKS_PER_BEAT, context.score.tempoChanges);
			float end = accumulateDuration(context.score.notes.at(holdNote.end).tick, TICKS_PER_BEAT, context.score.tempoChanges);
			if (abs(end - start) < 0.01f)
				return;

			float noteLeft{}, noteRight{};
			std::tie(noteLeft, noteRight) = getHoldSegmentBound(note, context.score, context.currentTick);

			ParticleController& controller = effectPools[effect].getNext();
			controller.worldOffset.position = DirectX::XMVectorSetX(controller.worldOffset.position, noteLeft + (noteRight - noteLeft) / 2);
			controller.play(note, start, end);

			return;
		}

		EffectPool& pool = effectPools[effect];
		for (int i = note.lane; i < note.lane + note.width; i++)
		{
			ParticleController& controller = pool.getNext();
			controller.worldOffset.position = DirectX::XMVectorSetX(controller.worldOffset.position, getEffectXPos(i, 1, config.pvMirrorScore));
			controller.play(note, time, -1);
		}
	}

	void EffectView::addLaneEffect(EffectType effect, const Note& note, const ScoreContext& context, float time)
	{
		EffectPool& pool = effectPools[effect];
		for (int i = note.lane; i < note.lane + note.width; i++)
		{
			ParticleController& controller = pool.pool[i];
			controller.worldOffset.position = DirectX::XMVectorSetX(controller.worldOffset.position, getEffectXPos(i, 1, config.pvMirrorScore));
			controller.play(note, time, -1);
		}
	}

	void EffectView::updateEffects(const ScoreContext& context, const Camera& camera, float time)
	{
		for (size_t i = 0; i < fx_note_hold_aura; i++)
		{
			for (auto& controller : effectPools[static_cast<EffectType>(i)].pool)
			{
				if (!controller.active)
					continue;

				controller.effectRoot.update(time, controller.worldOffset, camera);
			}
		}

		for (size_t i = fx_note_hold_aura; i < fx_count; i++)
		{
			for (auto& controller : effectPools[static_cast<EffectType>(i)].pool)
			{
				if (time >= controller.time.max)
					controller.active = false;

				if (!controller.active)
					continue;

				float noteLeft{}, noteRight{};
				std::tie(noteLeft, noteRight) = getHoldSegmentBound(context.score.notes.at(controller.refID), context.score, context.currentTick);

				if (static_cast<EffectType>(i) == fx_note_hold_aura ||
					static_cast<EffectType>(i) == fx_note_critical_long_hold_gen_aura)
				{
					controller.worldOffset.scale = DirectX::XMVectorSetX(controller.worldOffset.scale, (noteRight - noteLeft) * 0.95f);
				}

				controller.worldOffset.position = DirectX::XMVectorSetX(controller.worldOffset.position, (noteLeft + (noteRight - noteLeft) / 2) * 0.84f);
				controller.effectRoot.update(time, controller.worldOffset, camera);
			}
		}
	}

	void EffectView::drawUnderNoteEffects(Renderer* renderer, float time)
	{
		static const EffectType underNoteEffects[] =
		{
			fx_lane_critical,
			fx_lane_critical_flick,
			fx_lane_default,
			fx_note_critical_flick_aura,
			fx_note_critical_long_aura,
			fx_note_critical_normal_aura,
			fx_note_flick_aura,
			fx_note_long_aura,
			fx_note_normal_aura
		};

		for (auto& effect : underNoteEffects)
		{
			for (auto& controller : effectPools[effect].pool)
			{
				if (controller.active)
					drawUnderNoteEffectsInternal(controller.effectRoot, renderer, time);
			}
		}
	}

	void EffectView::drawEffects(Renderer* renderer, float time)
	{
		std::vector<ParticleController*> drawingControllers;
		for (int i = 3; i < fx_count; i++)
		{
			for (auto& controller : effectPools[static_cast<EffectType>(i)].pool)
			{
				if (controller.active)
					drawingControllers.push_back(&controller);
			}
		}

		std::stable_sort(drawingControllers.begin(), drawingControllers.end(),
			[](const ParticleController* c1, const ParticleController* c2) { return c1->time.min < c2->time.min; });

		for (auto c : drawingControllers)
			drawEffectsInternal(c->effectRoot, renderer, time);
	}

	void EffectView::drawUnderNoteEffectsInternal(EmitterInstance& emitter, Renderer* renderer, float time)
	{
		const Particle& ref = ResourceManager::getParticleEffect(emitter.getRefID());
		if (ref.order <= 5)
			drawParticles(emitter.particles, ref, renderer, time);

		for (auto& child : emitter.children)
		{
			drawUnderNoteEffectsInternal(child, renderer, time);
		}
	}
	
	void EffectView::drawEffectsInternal(EmitterInstance& emitter, Renderer* renderer, float time)
	{
		const Particle& ref = ResourceManager::getParticleEffect(emitter.getRefID());
		if (ref.order > 5)
			drawParticles(emitter.particles, ref, renderer, time);

		for (auto& child : emitter.children)
		{
			drawEffectsInternal(child, renderer, time);
		}
	}

	void EffectView::drawParticles(const std::vector<ParticleInstance>& particles, const Particle& ref, Renderer* renderer, float time)
	{
		int flipUVs = ref.renderMode == RenderMode::StretchedBillboard ? 1 : 0;
		float blend = ref.blend == BlendMode::Additive ? 1.f : 0.f;
		for (auto& p : particles)
		{
			if (!p.alive)
				continue;

			float normalizedTime = p.time / p.duration;
			if (time < p.startTime || time > p.startTime + p.duration)
				continue;

			int frame = ref.textureSplitX * ref.textureSplitY * ref.startFrame.evaluate(p.time, p.spriteSheetLerpRatio);
			frame += ref.textureSplitX * ref.textureSplitY * ref.frameOverTime.evaluate(normalizedTime, p.spriteSheetLerpRatio);

			Color color = p.startColor * ref.colorOverLifetime.evaluate(normalizedTime, p.colorLerpRatio);

			renderer->drawQuadWithBlend(p.matrix, *effectsTex, ref.textureSplitX, ref.textureSplitY, frame, color, ref.order, blend, flipUVs);
		}
	}
}