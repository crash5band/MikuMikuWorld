#include "ScoreConverter.h"
#include "SUS.h"
#include "Score.h"
#include "IO.h"
#include "Constants.h"
#include <unordered_set>
#include <algorithm>

using json = nlohmann::json;

namespace MikuMikuWorld
{
	std::string ScoreConverter::noteKey(const SUSNote& note)
	{
		return IO::formatString("%d-%d", note.tick, note.lane);
	}

	std::string ScoreConverter::noteKey(const Note& note)
	{
		return IO::formatString("%d-%d", note.tick, note.lane);
	}

	std::pair<int, int> ScoreConverter::barLengthToFraction(float length, float fractionDenom)
	{
		int factor = 1;
		for (int i = 2; i < 10; ++i)
		{
			if (fmodf(factor * length, 1) == 0)
				return std::pair<int, int>(factor * length, pow(2, i));

			factor *= 2;
		}

		return std::pair<int, int>(4, 4);
	}

	Score ScoreConverter::susToScore(const SUS& sus)
	{
		ScoreMetadata metadata
		{
			sus.metadata.data.at("title"),
			sus.metadata.data.at("artist"),
			sus.metadata.data.at("designer"),
			"",
			"",
			sus.metadata.waveOffset * 1000 // seconds -> milliseconds
		};

		std::unordered_map<std::string, FlickType> flicks;
		std::unordered_set<std::string> criticals;
		std::unordered_set<std::string> stepIgnore;
		std::unordered_set<std::string> easeIns;
		std::unordered_set<std::string> easeOuts;
		std::unordered_set<std::string> slideKeys;
		std::unordered_set<std::string> frictions;
		std::unordered_set<std::string> hiddenHolds;

		for (const auto& slides : {sus.slides, sus.guides})
      for (const auto& slide : slides)
      {
        for (const auto& note : slide)
        {
          switch (note.type)
          {
          case 1:
          case 2:
          case 3:
          case 5:
            slideKeys.insert(noteKey(note));
          }
        }
      }

		for (const auto& dir : sus.directionals)
		{
			const std::string key = noteKey(dir);
			switch (dir.type)
			{
			case 1:
				flicks.insert_or_assign(key, FlickType::Default);
				break;
			case 3:
				flicks.insert_or_assign(key, FlickType::Left);
				break;
			case 4:
				flicks.insert_or_assign(key, FlickType::Right);
				break;
			case 2:
				easeIns.insert(key);
				break;
			case 5:
			case 6:
				easeOuts.insert(key);
				break;
			default:
				break;
			}
		}

		for (const auto& tap : sus.taps)
		{
			const std::string key = noteKey(tap);
			switch (tap.type)
			{
			case 2:
				criticals.insert(key);
				break;
			case 3:
				stepIgnore.insert(key);
				break;
      case 4:
        hiddenHolds.insert(key);
        break;
			case 5:
				frictions.insert(key);
				break;
			case 6:
				criticals.insert(key);
				frictions.insert(key);
				break;
			case 7:
				hiddenHolds.insert(key);
				break;
			case 8:
				hiddenHolds.insert(key);
				criticals.insert(key);
				break;
			default:
				break;
			}
		}

		std::unordered_map<int, Note> notes;
		notes.reserve(sus.taps.size());

		std::unordered_map<int, HoldNote> holds;
		holds.reserve(sus.slides.size());

		std::vector<SkillTrigger> skills;
		Fever fever{ -1, -1 };

    // Cyanvas extension: disable fever and skills

		for (const auto& note : sus.taps)
		{
			/* if (note.type == 4) */
			/* { */
        /* skills.push_back(SkillTrigger{ nextSkillID++, note.tick }); */
			/* } */
			/* else if (note.lane == 15 && note.width == 1) */
			/* { */
			/* 	if (note.type == 1) */
			/* 		fever.startTick = note.tick; */
			/* 	else if (note.type == 2) */
			/* 		fever.endTick = note.tick; */
			/* } */
			/* else if (note.type == 7 || note.type == 8) */
			/* { */
			/* 	// Invisible slide tap point */
			/* 	continue; */
			/* } */

			if (!sus.sideLane && (note.lane - 2 < MIN_LANE || note.lane - 2 > MAX_LANE))
				continue;

			const std::string key = noteKey(note);

			// Conflict with skip slide steps and hidden holds
			if (slideKeys.find(key) != slideKeys.end())
				continue;

      Note n;
			if (note.type == 4) {
        n = Note(NoteType::Damage, note.tick, note.lane - 2, note.width);
        n.critical = false;
        n.friction = false;
        n.flick = FlickType::None;
      } else {
        n = Note(NoteType::Tap, note.tick, note.lane - 2, note.width);
        n.critical = criticals.find(key) != criticals.end();
        n.friction = frictions.find(key) != frictions.end() || stepIgnore.find(key) != stepIgnore.end();
        n.flick = flicks.find(key) != flicks.end() ? flicks[key] : FlickType::None;
      }
			n.ID = nextID++;

			notes[n.ID] = n;
		}

		// Not the best way to handle this but it will do the job
		static auto slideFillFunc = [&](const std::vector<std::vector<SUSNote>>& slides, bool isGuideSlides)
		{
			for (const auto& slide : slides)
			{
        bool isGuide = isGuideSlides;
				const std::string key = noteKey(slide[0]);

				auto start = std::find_if(slide.begin(), slide.end(),
					[](const SUSNote& a) { return a.type == 1 || a.type == 2; });

				if (start == slide.end() || slide.size() < 2)
					continue;

				bool critical = criticals.find(key) != criticals.end();

				HoldNote hold;
				int startID = nextID++;
				hold.steps.reserve(slide.size() - 2);

        if (isGuide && critical) {
          hold.guideColor = GuideColor::Yellow;
        }

				for (const auto& note : slide)
				{
					const std::string key = noteKey(note);

					EaseType ease = EaseType::Linear;
					if (easeIns.find(key) != easeIns.end())
					{
						ease = EaseType::EaseIn;
					}
					else if (easeOuts.find(key) != easeOuts.end())
					{
						ease = EaseType::EaseOut;
					}

					switch (note.type)
					{
					// start
					case 1:
					{
						Note n(NoteType::Hold, note.tick, note.lane - 2, note.width);
						n.critical = critical;
						n.ID = startID;

						if (isGuide || (hiddenHolds.find(noteKey(note)) != hiddenHolds.end() && stepIgnore.find(key) != stepIgnore.end()))
						{
              isGuide = true;
							hold.startType = HoldNoteType::Guide;
						}
						else
						{
							n.friction = frictions.find(noteKey(note)) != frictions.end() || stepIgnore.find(key) != stepIgnore.end();
							hold.startType = hiddenHolds.find(noteKey(note)) != hiddenHolds.end() ? HoldNoteType::Hidden : HoldNoteType::Normal;
						}

						notes[n.ID] = n;
						hold.start = HoldStep{ n.ID, HoldStepType::Normal, ease };
					}
					break;
					// end
					case 2:
					{
						Note n(NoteType::HoldEnd, note.tick, note.lane - 2, note.width);
						n.critical = (critical ? true : (criticals.find(key) != criticals.end()));
						n.ID = nextID++;
						n.parentID = startID;

						if (isGuide)
						{
							hold.endType = HoldNoteType::Guide;
              hold.fadeType = hiddenHolds.find(noteKey(note)) != hiddenHolds.end() ? FadeType::None : FadeType::Out;
						}
						else
						{
							n.flick = flicks.find(key) != flicks.end() ? flicks[key] : FlickType::None;
							n.friction = frictions.find(noteKey(note)) != frictions.end() || stepIgnore.find(key) != stepIgnore.end();
							hold.endType = hiddenHolds.find(noteKey(note)) != hiddenHolds.end() ? HoldNoteType::Hidden : HoldNoteType::Normal;
						}

						notes[n.ID] = n;
						hold.end = n.ID;
					}
					break;
					// mid
					case 3:
					case 5:
					{
						Note n(NoteType::HoldMid, note.tick, note.lane - 2, note.width);
						n.critical = critical;
						n.friction = false;
						n.ID = nextID++;
						n.parentID = startID;

						if (n.friction)
							printf("Note at %d-%d is friction", n.tick, n.lane);

						HoldStepType type = note.type == 3 ? HoldStepType::Normal : HoldStepType::Hidden;
						if (stepIgnore.find(key) != stepIgnore.end()) {
							type = HoldStepType::Skip;
            }

						notes[n.ID] = n;
						hold.steps.push_back(HoldStep{ n.ID, type, ease });
					}
					break;
					default:
						break;
					}
				}

				if (hold.start.ID == 0 || hold.end == 0)
					throw std::runtime_error("Invalid hold note");

				holds[startID] = hold;
			}
		};

		slideFillFunc(sus.slides, false);
		slideFillFunc(sus.guides, true);

		std::vector<Tempo> tempos;
		tempos.reserve(sus.bpms.size());
		for (const auto& tempo : sus.bpms)
			tempos.push_back(Tempo(tempo.tick, tempo.bpm));

		std::map<int, TimeSignature> timeSignatures;
		for (const auto& sign : sus.barlengths)
		{
			auto fraction = barLengthToFraction(sign.length, 4.0f);
			timeSignatures.insert(std::pair<int, TimeSignature>(sign.bar,
				TimeSignature{ sign.bar, fraction.first, fraction.second }));
		}

		std::vector<HiSpeedChange> hiSpeedChanges;
		hiSpeedChanges.reserve(sus.hiSpeeds.size());
		for (const auto& speed : sus.hiSpeeds)
			hiSpeedChanges.push_back({ speed.tick, speed.speed });

		Score score;
		score.metadata = metadata;
		score.notes = notes;
		score.holdNotes = holds;
		score.tempoChanges = tempos;
		score.timeSignatures = timeSignatures;
		score.hiSpeedChanges = hiSpeedChanges;
		score.skills = skills;
		score.fever = fever;

		return score;
	}

	SUS ScoreConverter::scoreToSus(const Score& score)
	{
		std::unordered_map<FlickType, int> flickToType;
		flickToType[FlickType::Default] = 1;
		flickToType[FlickType::Left] = 3;
		flickToType[FlickType::Right] = 4;

    int offset = score.metadata.laneExtension == 0 ? 2 : score.metadata.laneExtension;

		std::vector<SUSNote> taps, directionals;
		std::vector<std::vector<SUSNote>> slides;
		std::vector<std::vector<SUSNote>> guides;
		std::vector<BPM> bpms;
		std::vector<BarLength> barlengths;
		std::vector<HiSpeed> hiSpeeds;

		std::unordered_set<std::string> criticalKeys;
		for (const auto& [id, note] : score.notes)
		{
			if (note.getType() == NoteType::Tap)
			{
				int type = note.friction ? 5 : 1;
				if (note.critical)
				{
					type++;
					criticalKeys.insert(noteKey(note));
				}
				taps.push_back(SUSNote{ note.tick, note.lane + offset, note.width, type });

				if (note.isFlick())
					directionals.push_back(SUSNote{ note.tick, note.lane + offset, note.width, flickToType[note.flick] });
			}
			if (note.getType() == NoteType::Damage)
				taps.push_back(SUSNote{ note.tick, note.lane + offset, note.width, 4 });
		}

		/*
			Generally guides can be placed on the same tick and lane (key) as other notes.
			If one of those notes or the guide is critical however, it will cause all the notes with the same key to be critical.
		*/
		std::vector<HoldNote> exportHolds;
		exportHolds.reserve(score.holdNotes.size());

		for (const auto& [_, hold] : score.holdNotes)
			exportHolds.emplace_back(hold);

		std::sort(exportHolds.begin(), exportHolds.end(), [&score](const HoldNote& a, const HoldNote& b)
		{
			return score.notes.at(a.start.ID).tick < score.notes.at(b.start.ID).tick;
		});

		for (const auto& hold : exportHolds)
		{
			std::vector<SUSNote> slide;
			slide.reserve(hold.steps.size() + 2);

			const Note& start = score.notes.at(hold.start.ID);

			slide.push_back(SUSNote{ start.tick, start.lane + offset, start.width, 1 });

			bool hasEase = hold.start.ease != EaseType::Linear;
			bool invisibleTapPoint =
				hold.startType == HoldNoteType::Hidden ||
				hold.startType != HoldNoteType::Normal && hasEase ||
				hold.startType != HoldNoteType::Normal && start.critical;

			if (hasEase)
				directionals.push_back(SUSNote{ start.tick, start.lane + offset, start.width, hold.start.ease == EaseType::EaseIn ? 2 : 6 });

			bool guideAlreadyOnCritical = hold.isGuide() && criticalKeys.find(noteKey(start)) != criticalKeys.end();

			// We'll use type 1 to indicate it's a normal note
			int type = invisibleTapPoint ? 7 : start.friction ? 5 : 1;
			if ((start.critical || hold.guideColor == GuideColor::Yellow) && criticalKeys.find(noteKey(start)) == criticalKeys.end())
			{
				type++;
				criticalKeys.insert(noteKey(start));
			}

			if (type > 1 && !((type == 7 || type == 8) && guideAlreadyOnCritical))
				taps.push_back({ start.tick, start.lane + offset, start.width, type });

			for (const auto& step : hold.steps)
			{
				const Note& midNote = score.notes.at(step.ID);

				slide.push_back(SUSNote{ midNote.tick, midNote.lane + offset, midNote.width, step.type == HoldStepType::Hidden ? 5 : 3 });
				if (step.type == HoldStepType::Skip)
				{
					taps.push_back(SUSNote{ midNote.tick, midNote.lane + offset, midNote.width, 3 });
				}
				else if (step.ease != EaseType::Linear)
				{
					int stepTapType = hold.isGuide() ? start.critical ? 8 : 7 : 1;
					taps.push_back(SUSNote{ midNote.tick, midNote.lane + offset, midNote.width, stepTapType });
					directionals.push_back(SUSNote{ midNote.tick, midNote.lane + offset, midNote.width, step.ease == EaseType::EaseIn ? 2 : 6 });
				}
			}

			const Note& end = score.notes.at(hold.end);

			slide.push_back(SUSNote{ end.tick, end.lane + offset, end.width, 2 });

			// Hidden and guide slides do not have flicks
			if (end.isFlick() && hold.endType == HoldNoteType::Normal)
			{
				directionals.push_back(SUSNote{ end.tick, end.lane + offset, end.width, flickToType[end.flick] });

				// Critical friction notes use type 6 not 2
				if (end.critical && !start.critical && !end.friction)
					taps.push_back(SUSNote{ end.tick, end.lane + offset, end.width, 2 });
			}

			int endType = hold.fadeType == FadeType::None ? 4 : hold.endType == HoldNoteType::Hidden ? 7 : end.friction ? 5 : 1;
			if (end.critical)
			{
				endType++;
				criticalKeys.insert(noteKey(end));
			}

			if (endType != 1 && endType != 2)
				taps.push_back(SUSNote{ end.tick, end.lane + offset, end.width, endType });

			if (hold.isGuide())
			{
				guides.push_back(slide);
			}
			else
			{
				slides.push_back(slide);
			}
		}

		for (const auto& skill : score.skills)
			taps.push_back(SUSNote{ skill.tick, 0, 1, 4 });

		if (score.fever.startTick != -1)
			taps.push_back(SUSNote{ score.fever.startTick, 15, 1, 1 });

		if (score.fever.endTick != -1)
			taps.push_back(SUSNote{ score.fever.endTick, 15, 1, 2 });

		for (const auto& tempo : score.tempoChanges)
			bpms.push_back(BPM{ tempo.tick, tempo.bpm });

		if (!bpms.size())
			bpms.push_back(BPM{ 0, 120 });

		for (const auto& [measure, ts] : score.timeSignatures)
			barlengths.push_back(BarLength{ ts.measure, ((float)ts.numerator / (float)ts.denominator) * 4 });

		hiSpeeds.reserve(score.hiSpeedChanges.size());
		for (const auto& hiSpeed : score.hiSpeedChanges)
			hiSpeeds.push_back(HiSpeed{ hiSpeed.tick, hiSpeed.speed });

		SUSMetadata metadata;
		metadata.data["title"] = score.metadata.title;
		metadata.data["artist"] = score.metadata.artist;
		metadata.data["designer"] = score.metadata.author;
		metadata.requests.push_back("ticks_per_beat 480");
		metadata.requests.push_back("side_lane true");
    int laneOffset;
    if (score.metadata.laneExtension == 0) {
      laneOffset = 0;
    } else {
      laneOffset = -score.metadata.laneExtension + 2;
    }
    metadata.requests.push_back("lane_offset " + std::to_string(laneOffset));

		// milliseconds -> seconds
		metadata.waveOffset = score.metadata.musicOffset / 1000.0f;

		return SUS{ metadata, taps, directionals, slides, guides, bpms, barlengths, hiSpeeds, laneOffset };
	}

  json ScoreConverter::scoreToUsc(const Score& score)
  {
    json vusc;
    json usc;

    usc["offset"] = score.metadata.musicOffset * 1000;

    std::vector<json> objects;

    for (const auto& bpm : score.tempoChanges) {
      json obj;
      obj["type"] = "bpm";
      obj["beat"] = bpm.tick / (float)TICKS_PER_BEAT;
      obj["bpm"] = bpm.bpm;
      objects.push_back(obj);
    }

    json timeScaleGroup;
    timeScaleGroup["type"] = "timeScaleGroup";
    std::vector<json> timeScaleObjects;
    for (const auto& hs : score.hiSpeedChanges) {
      json obj;
      obj["beat"] = hs.tick / (float)TICKS_PER_BEAT;
      obj["timeScale"] = hs.speed;
      timeScaleObjects.push_back(obj);
    }
    timeScaleGroup["changes"] = timeScaleObjects;
    objects.push_back(timeScaleGroup);

    for (const auto& [_, note] : score.notes) {
      if (note.getType() == NoteType::Tap) {
        json obj;
        obj["type"] = "single";
        obj["beat"] = note.tick / (float)TICKS_PER_BEAT;
        obj["size"] = note.width / 2.0;
        obj["lane"] = note.lane - 6 + (note.width / 2.0);
        obj["critical"] = note.critical;
        obj["trace"] = note.friction;
        if (note.flick != FlickType::None) {
          obj["flick"] = note.flick == FlickType::Default ? "up" : note.flick == FlickType::Left ? "left" : "right";
        }
        objects.push_back(obj);
      } else if (note.getType() == NoteType::Damage) {
        json obj;
        obj["type"] = "damage";
        obj["beat"] = note.tick / (float)TICKS_PER_BEAT;
        obj["size"] = note.width / 2.0;
        obj["lane"] = note.lane - 6 + (note.width / 2.0);
        objects.push_back(obj);
      }
    }
    for (const auto& [_, note] : score.holdNotes) {
      json obj;
      if (note.isGuide()) {
        obj["type"] = "guide";
        auto& start = score.notes.at(note.start.ID);
        obj["color"] = guideColors[(int)note.guideColor];
        obj["fade"] = note.fadeType == FadeType::None ? "none" : note.fadeType == FadeType::In ? "in" : "out";

        std::vector<json> steps;
        steps.reserve(note.steps.size() + 1);

        json startStep;
        startStep["beat"] = start.tick / (float)TICKS_PER_BEAT;
        startStep["size"] = start.width / 2.0;
        startStep["lane"] = start.lane - 6 + (start.width / 2.0);
        startStep["ease"] = note.start.ease == EaseType::EaseIn ? "in" : note.start.ease == EaseType::EaseOut ? "out" : "linear";
        steps.push_back(startStep);

        for (const auto& step : note.steps) {
          json stepObj;
          auto& stepNote = score.notes.at(step.ID);
          stepObj["beat"] = stepNote.tick / (float)TICKS_PER_BEAT;
          stepObj["size"] = stepNote.width / 2.0;
          stepObj["lane"] = stepNote.lane - 6 + (stepNote.width / 2.0);
          stepObj["ease"] = note.start.ease == EaseType::EaseIn ? "in" : note.start.ease == EaseType::EaseOut ? "out" : "linear";
          steps.push_back(stepObj);
        }

        json endStep;
        auto& end = score.notes.at(note.end);
        endStep["beat"] = end.tick / (float)TICKS_PER_BEAT;
        endStep["size"] = end.width / 2.0;
        endStep["lane"] = end.lane - 6 + (end.width / 2.0);
        endStep["ease"] = "linear";
        steps.push_back(endStep);

        obj["midpoints"] = steps;
        objects.push_back(obj);
        continue;
      }
      obj["type"] = "slide";
      auto& start = score.notes.at(note.start.ID);
      obj["critical"] = start.critical;

      std::vector<json> steps;
      steps.reserve(note.steps.size() + 1);
      json startStep;
      startStep["type"] = "start";
      startStep["beat"] = start.tick / (float)TICKS_PER_BEAT;
      startStep["size"] = start.width / 2.0;
      startStep["lane"] = start.lane - 6 + (start.width / 2.0);
      startStep["critical"] = start.critical;
      startStep["ease"] = note.start.ease == EaseType::EaseIn ? "in" : note.start.ease == EaseType::EaseOut ? "out" : "linear";
      startStep["judgeType"] = note.startType == HoldNoteType::Hidden ? "none" : start.friction ? "trace" : "normal";
      steps.push_back(startStep);

      for (const auto& step : note.steps) {
        json stepObj;
        auto& stepNote = score.notes.at(step.ID);
        stepObj["type"] = step.type == HoldStepType::Skip ? "attach" : "tick";
        stepObj["beat"] = stepNote.tick / (float)TICKS_PER_BEAT;
        stepObj["size"] = stepNote.width / 2.0;
        stepObj["lane"] = stepNote.lane - 6 + (stepNote.width / 2.0);
        if (step.type != HoldStepType::Hidden) {
          stepObj["critical"] = stepNote.critical;
        }
        stepObj["ease"] = note.start.ease == EaseType::EaseIn ? "in" : note.start.ease == EaseType::EaseOut ? "out" : "linear";
        steps.push_back(startStep);
      }

      json endStep;
      auto& end = score.notes.at(note.end);
      endStep["type"] = "end";
      endStep["beat"] = end.tick / (float)TICKS_PER_BEAT;
      endStep["size"] = end.width / 2.0;
      endStep["lane"] = end.lane - 6 + (end.width / 2.0);
      endStep["critical"] = end.critical;
      endStep["judgeType"] = note.endType == HoldNoteType::Hidden ? "none" : end.friction ? "trace" : "normal";
      if (end.flick != FlickType::None) {
        endStep["direction"] = end.flick == FlickType::Default ? "up" : end.flick == FlickType::Left ? "left" : "right";
      }
      steps.push_back(endStep);

      obj["connections"] = steps;
      objects.push_back(obj);
    }

    usc["objects"] = objects;

    vusc["version"] = 2;
    vusc["usc"] = usc;
    return vusc;
  }
}
