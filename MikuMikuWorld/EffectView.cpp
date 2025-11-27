#include "EffectView.h"
#include "PreviewEngine.h"
#include "ResourceManager.h"
#include "ScoreContext.h"

namespace MikuMikuWorld::Effect
{
	static float getEffectXPos(int lane, int width)
	{
		return (lane - 6 + width / 2.f) * 0.84f;
	}

	static std::pair<float, float> getNoteBound(const Note& note)
	{
		float left = Engine::laneToLeft(note.lane), right = left + note.width;
		//if (config.pvMirrorScore)
		//	std::swap(left *= -1, right *= -1);
		return std::make_pair(left, right);
	}

	static std::pair<float, float> getHoldSegmentBound(const Note& note, const Score& score, int curTick)
	{
		const HoldNote& holdNotes = score.holdNotes.at(note.ID);
		auto curStepIt = std::lower_bound(holdNotes.steps.begin(), holdNotes.steps.end(), curTick, [&score](const HoldStep& step, int tick) { return score.notes.at(step.ID).tick < tick; });
		auto startStepIt = std::find_if(std::make_reverse_iterator(curStepIt), holdNotes.steps.rend(), std::mem_fn(&HoldStep::canEase));
		const HoldStep& startHoldStep = startStepIt == holdNotes.steps.rend() ? holdNotes.start : *startStepIt;

		const Note& startNote = startStepIt == holdNotes.steps.rend() ? note : score.notes.at(startStepIt->ID);
		if (startNote.tick == curTick) return getNoteBound(startNote);
		auto [leftStart, rightStart] = getNoteBound(startNote);

		auto end = std::find_if(curStepIt, holdNotes.steps.end(), std::mem_fn(&HoldStep::canEase));
		const Note& endNote = score.notes.at(end == holdNotes.steps.end() ? holdNotes.end : end->ID);
		if (endNote.tick == curTick) return getNoteBound(endNote);
		auto [leftStop, rightStop] = getNoteBound(endNote);
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
			return getNoteBound(note);

		int startStepIdx = curStepIdx;
		while (startStepIdx != 0 && !holdNotes.steps[startStepIdx - 1].canEase())
			startStepIdx--;
		const HoldStep& lastHoldStep = startStepIdx != 0 ? holdNotes.steps[startStepIdx - 1] : holdNotes.start;
		auto easeFunc = getEaseFunction(lastHoldStep.ease);

		const Note& startNote = score.notes.at(lastHoldStep.ID);
		auto [leftStart, rightStart] = getNoteBound(startNote);

		auto it = std::find_if(holdNotes.steps.begin() + curStepIdx, holdNotes.steps.end(), std::mem_fn(&HoldStep::canEase));
		const Note& endNote = score.notes.at(it == holdNotes.steps.end() ? holdNotes.end : it->ID);
		auto [leftStop, rightStop] = getNoteBound(endNote);

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
		for (auto& instance : pool)
		{
			instance.effectRoot = createEmitterFromParticle(particleId);
			instance.effectRoot.init(ref);
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
			if (isMidHold || isWithinRange(currentTime, noteTime - 0.02f, noteTime + 0.02f))
			{
				addNoteEffects(note, context, noteTime);
				playedEffectsNoteIds.insert(id);
			}
		}
	}

	void EffectView::init()
	{
		if (initialized)
			return;

		effectPools.clear();
		for (int i = 0; i < static_cast<int>(EffectType::fx_count); i++)
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
			}
		}

		playedEffectsNoteIds.clear();
	}

	void EffectView::addNoteEffects(const Note& note, const ScoreContext& context, float time)
	{
		if (note.friction)
		{
			EffectType traceEffect{ EffectType::fx_count };
			if (note.isFlick())
			{
				traceEffect = note.critical ? EffectType::fx_note_critical_flick_flash : EffectType::fx_note_flick_flash;
				if (note.critical)
					addLaneEffect(EffectType::fx_lane_critical_flick, note, context, time);
			}
			else
			{
				traceEffect = note.critical ? EffectType::fx_note_critical_trace_aura : EffectType::fx_note_trace_aura;
			}

			addEffect(traceEffect, note, context, time);
			if (note.getType() == NoteType::Hold)
			{
				addEffect(note.critical ? EffectType::fx_note_critical_long_hold_gen : EffectType::fx_note_long_hold_gen, note, context, time);
				addAuraEffect(note.critical ? EffectType::fx_note_critical_long_hold_gen_aura : EffectType::fx_note_hold_aura, note, context, time);
			}
			return;
		}

		if (note.isFlick())
		{
			EffectType aura{ note.critical ? EffectType::fx_note_critical_flick_aura : EffectType::fx_note_flick_aura };
			EffectType gen{ note.critical ? EffectType::fx_note_critical_flick_gen : EffectType::fx_note_flick_gen };
			EffectType flash{ note.critical ? EffectType::fx_note_critical_flick_flash : EffectType::fx_note_flick_flash };

			addAuraEffect(aura, note, context, time);
			addEffect(gen, note, context, time);
			addEffect(flash, note, context, time);

			if (note.critical)
				addLaneEffect(EffectType::fx_lane_critical_flick, note, context, time);

			return;
		}

		if (note.getType() == NoteType::Hold)
		{
			const HoldNote& hold = context.score.holdNotes.at(note.ID);
			if (hold.isGuide())
				return;

			if (hold.startType == HoldNoteType::Normal)
			{
				EffectType aura{ note.critical ? EffectType::fx_note_critical_long_aura : EffectType::fx_note_long_aura };
				EffectType gen{ note.critical ? EffectType::fx_note_critical_long_gen : EffectType::fx_note_long_gen };
				EffectType lane{ note.critical ? EffectType::fx_lane_critical : EffectType::fx_lane_default };
				
				addAuraEffect(aura, note, context, time);
				addEffect(gen, note, context, time);
				addLaneEffect(lane, note, context, time);
			}

			addEffect(note.critical ? EffectType::fx_note_critical_long_hold_gen : EffectType::fx_note_long_hold_gen, note, context, time);
			addAuraEffect(note.critical ? EffectType::fx_note_critical_long_hold_gen_aura : EffectType::fx_note_hold_aura, note, context, time);
		}
		else if (note.getType() == NoteType::HoldMid)
		{
			const HoldNote& hold = context.score.holdNotes.at(note.parentID);
			const HoldStep& step = hold.steps[findHoldStep(hold, note.ID)];

			if (step.type == HoldStepType::Hidden)
				return;

			addEffect(note.critical ? EffectType::fx_note_critical_long_hold_via_aura : EffectType::fx_note_long_hold_via_aura, note, context, time);
		}
		else if (note.getType() == NoteType::HoldEnd)
		{
			const HoldNote& hold = context.score.holdNotes.at(note.parentID);
			if (hold.isGuide())
				return;

			if (hold.endType == HoldNoteType::Normal)
			{
				EffectType aura{ note.critical ? EffectType::fx_note_critical_long_aura : EffectType::fx_note_long_aura };
				EffectType gen{ note.critical ? EffectType::fx_note_critical_long_gen : EffectType::fx_note_long_gen };
				EffectType lane{ note.critical ? EffectType::fx_lane_critical : EffectType::fx_lane_default };

				addAuraEffect(aura, note, context, time);
				addEffect(gen, note, context, time);
				addLaneEffect(lane, note, context, time);
			}
		}
		else
		{
			EffectType aura{ note.critical ? EffectType::fx_note_critical_normal_aura : EffectType::fx_note_normal_aura };
			EffectType gen{ note.critical ? EffectType::fx_note_critical_normal_gen : EffectType::fx_note_normal_gen };
			EffectType lane{ note.critical ? EffectType::fx_lane_critical : EffectType::fx_lane_default };

			addAuraEffect(aura, note, context, time);
			addEffect(gen, note, context, time);
			addLaneEffect(lane, note, context, time);
		}
	}

	void EffectView::addEffect(EffectType effect, const Note& note, const ScoreContext& context, float time)
	{
		EffectPool& pool = effectPools[effect];
		ParticleController& controller = pool.getNext();

		float xPos = Engine::getNoteCenter(note);
		float zRot = 0.f;
		float start = time;
		float end = -1;

		if (note.isFlick() && (effect == EffectType::fx_note_flick_flash || effect == EffectType::fx_note_critical_flick_flash))
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
		}
		else if (effect == EffectType::fx_note_critical_long_hold_via_aura || effect == EffectType::fx_note_long_hold_via_aura)
		{
			const HoldNote& hold = context.score.holdNotes.at(note.parentID);
			const HoldStep& step = hold.steps[findHoldStep(hold, note.ID)];

			float noteLeft{}, noteRight{};
			std::tie(noteLeft, noteRight) = getHoldStepBound(note, context.score);

			xPos = noteLeft + (noteRight - noteLeft) / 2;
		}
		else if (effect == EffectType::fx_note_critical_long_hold_gen || effect == EffectType::fx_note_long_hold_gen)
		{
			const HoldNote& holdNote = context.score.holdNotes.at(note.ID);
			float noteLeft{}, noteRight{};
			std::tie(noteLeft, noteRight) = getHoldSegmentBound(note, context.score, context.currentTick);

			xPos = noteLeft + (noteRight - noteLeft) / 2;
			start = accumulateDuration(note.tick, TICKS_PER_BEAT, context.score.tempoChanges);
			end = accumulateDuration(context.score.notes.at(holdNote.end).tick, TICKS_PER_BEAT, context.score.tempoChanges);
		}

		controller.worldOffset.position = DirectX::XMVectorSetX(controller.worldOffset.position, xPos * 0.84f);
		controller.worldOffset.rotation = DirectX::XMVectorSetZ(controller.worldOffset.rotation, zRot);
		controller.play(note, start, end);
	}

	void EffectView::addAuraEffect(EffectType effect, const Note& note, const ScoreContext& context, float time)
	{
		if (effect == EffectType::fx_note_hold_aura || effect == EffectType::fx_note_critical_long_hold_gen_aura)
		{
			const HoldNote& holdNote = context.score.holdNotes.at(note.ID);
			float start = accumulateDuration(note.tick, TICKS_PER_BEAT, context.score.tempoChanges);
			float end = accumulateDuration(context.score.notes.at(holdNote.end).tick, TICKS_PER_BEAT, context.score.tempoChanges);
			if (abs(end - start) < 0.001f)
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
			controller.worldOffset.position = DirectX::XMVectorSetX(controller.worldOffset.position, getEffectXPos(i, 1));
			controller.play(note, time, -1);
		}
	}

	// TODO: make this reuse lanes like the game!
	void EffectView::addLaneEffect(EffectType effect, const Note& note, const ScoreContext& context, float time)
	{
		EffectPool& pool = effectPools[effect];
		for (int i = note.lane; i < note.lane + note.width; i++)
		{
			ParticleController& controller = pool.pool[i];
			controller.worldOffset.position = DirectX::XMVectorSetX(controller.worldOffset.position, getEffectXPos(i, 1));
			controller.play(note, time, -1);
		}
	}

	void EffectView::updateEffects(const ScoreContext& context, const Camera& camera, float time)
	{
		for (auto& [type, pool] : effectPools)
		{
			for (auto& controller : pool.pool)
			{
				if (controller.active && time >= controller.time.min)
				{
					if (type == EffectType::fx_note_critical_long_hold_gen ||
						type == EffectType::fx_note_long_hold_gen || 
						type == EffectType::fx_note_hold_aura ||
						type == EffectType::fx_note_critical_long_hold_gen_aura)
					{
						if (time >= controller.time.max)
						{
							controller.active = false;
							continue;
						}

						float noteLeft{}, noteRight{};
						std::tie(noteLeft, noteRight) = getHoldSegmentBound(context.score.notes.at(controller.refID), context.score, context.currentTick);

						if (type == EffectType::fx_note_hold_aura ||
							type == EffectType::fx_note_critical_long_hold_gen_aura)
						{
							controller.worldOffset.scale = DirectX::XMVectorSetX(controller.worldOffset.scale, (noteRight - noteLeft) * 0.95f);
						}

						controller.worldOffset.position = DirectX::XMVectorSetX(controller.worldOffset.position, (noteLeft + (noteRight - noteLeft) / 2) * 0.84f);
					}

					Transform baseTransform{};
					controller.effectRoot.update(time, controller.worldOffset, baseTransform, camera);
				}
			}
		}
	}

	void EffectView::drawEffects(Renderer* renderer, float time)
	{
		for (auto& [type, pool] : effectPools)
		{
			for (auto& controller : pool.pool)
			{
				if (controller.active)
					drawEffectsInternal(controller.effectRoot, renderer, time);
			}
		}
	}
	
	void EffectView::drawEffectsInternal(EmitterInstance& emitter, Renderer* renderer, float time)
	{
		const Particle& ref = ResourceManager::getParticleEffect(emitter.getRefID());
		int flipUVs = ref.renderMode == RenderMode::StretchedBillboard ? 1 : 0;
		for (auto& particle : emitter.particles)
		{
			if (!particle.alive)
				continue;

			float normalizedTime = particle.time / particle.duration;
			if (normalizedTime < 0)
				continue;

			float blend = ref.blend == BlendMode::Additive ? 1.f : 0.f;

			int frame = ref.textureSplitX * ref.textureSplitY * ref.startFrame.evaluate(particle.time, particle.textureStartFrameLerpRatio);
			frame += ref.textureSplitX * ref.textureSplitY * ref.frameOverTime.evaluate(normalizedTime, particle.textureFrameOverTimeLerpRatio);

			Color color = particle.startColor * ref.colorOverLifetime.evaluate(normalizedTime, particle.colorOverLifeTimeLerpRatio);

			// TODO: We should provide a second Z-Index according to the notes order.
			// Maybe use the note's tick?
			renderer->drawQuadWithBlend(particle.matrix, *effectsTex, ref.textureSplitX, ref.textureSplitY, frame, color, ref.order, blend, flipUVs);
		}

		for (auto& child : emitter.children)
		{
			drawEffectsInternal(child, renderer, time);
		}
	}
}