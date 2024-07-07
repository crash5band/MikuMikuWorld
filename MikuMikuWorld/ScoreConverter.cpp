#include "ScoreConverter.h"
#include "Constants.h"
#include "IO.h"
#include "SUS.h"
#include "Score.h"
#include <algorithm>
#include <array>
#include <unordered_set>

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
		ScoreMetadata metadata{
			sus.metadata.data.at("title"),
			sus.metadata.data.at("artist"),
			sus.metadata.data.at("designer"),
			"",
			"",
			sus.metadata.waveOffset * 1000 // seconds -> milliseconds
		};

		std::vector<std::string> hiSpeedGroupNames;

		for (const auto& group : sus.hiSpeedGroups)
			hiSpeedGroupNames.push_back(group.name);

		std::unordered_map<std::string, FlickType> flicks;
		std::unordered_set<std::string> criticals;
		std::unordered_set<std::string> stepIgnore;
		std::unordered_set<std::string> easeIns;
		std::unordered_set<std::string> easeOuts;
		std::unordered_set<std::string> slideKeys;
		std::unordered_set<std::string> frictions;
		std::unordered_set<std::string> hiddenHolds;

		for (const auto& slides : { sus.slides, sus.guides })
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

		std::unordered_set<std::string> cyanvasStyleCriticalTraces;

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
			if (note.type == 4)
			{
				n = Note(NoteType::Damage, note.tick, note.lane - 2, note.width);
				n.critical = false;
				n.friction = false;
				n.flick = FlickType::None;
			}
			else
			{
				n = Note(NoteType::Tap, note.tick, note.lane - 2, note.width);
				n.critical = criticals.find(key) != criticals.end();
				n.friction = frictions.find(key) != frictions.end() ||
				             stepIgnore.find(key) != stepIgnore.end();
				n.flick = flicks.find(key) != flicks.end() ? flicks[key] : FlickType::None;
				if (n.critical && n.friction)
				{
					if (cyanvasStyleCriticalTraces.find(key) != cyanvasStyleCriticalTraces.end())
					{
						continue;
					}
					cyanvasStyleCriticalTraces.insert(key);
				}
			}
			n.layer = std::distance(
			    hiSpeedGroupNames.begin(),
			    std::find(hiSpeedGroupNames.begin(), hiSpeedGroupNames.end(), note.hiSpeedGroup));
			n.ID = nextID++;

			notes[n.ID] = n;
		}

		// Not the best way to handle this but it will do the job
		auto slideFillFunc =
		    [&](const std::vector<std::vector<SUSNote>>& slides, bool isGuideSlides)
		{
			for (const auto& slide : slides)
			{
				bool isGuide = isGuideSlides;
				const std::string key = noteKey(slide[0]);

				auto start =
				    std::find_if(slide.begin(), slide.end(),
				                 [](const SUSNote& a) { return a.type == 1 || a.type == 2; });

				if (start == slide.end() || slide.size() < 2)
					continue;

				bool critical = criticals.find(key) != criticals.end();

				HoldNote hold;
				int startID = nextID++;
				hold.steps.reserve(slide.size() - 2);

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
						n.layer =
						    std::distance(hiSpeedGroupNames.begin(),
						                  std::find(hiSpeedGroupNames.begin(),
						                            hiSpeedGroupNames.end(), note.hiSpeedGroup));
						n.ID = startID;

						if (isGuide || (hiddenHolds.find(noteKey(note)) != hiddenHolds.end() &&
						                stepIgnore.find(key) != stepIgnore.end()))
						{
							isGuide = true;
							if (critical)
							{
								hold.guideColor = GuideColor::Yellow;
							}
							hold.startType = HoldNoteType::Guide;
						}
						else
						{
							n.friction = frictions.find(noteKey(note)) != frictions.end() ||
							             stepIgnore.find(key) != stepIgnore.end();
							hold.startType = hiddenHolds.find(noteKey(note)) != hiddenHolds.end()
							                     ? HoldNoteType::Hidden
							                     : HoldNoteType::Normal;
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
						n.layer =
						    std::distance(hiSpeedGroupNames.begin(),
						                  std::find(hiSpeedGroupNames.begin(),
						                            hiSpeedGroupNames.end(), note.hiSpeedGroup));
						n.ID = nextID++;
						n.parentID = startID;

						if (isGuide)
						{
							hold.endType = HoldNoteType::Guide;
							hold.fadeType = hiddenHolds.find(noteKey(note)) != hiddenHolds.end()
							                    ? FadeType::None
							                    : FadeType::Out;
						}
						else
						{
							n.flick =
							    flicks.find(key) != flicks.end() ? flicks[key] : FlickType::None;
							n.friction = frictions.find(noteKey(note)) != frictions.end() ||
							             stepIgnore.find(key) != stepIgnore.end();
							hold.endType = hiddenHolds.find(noteKey(note)) != hiddenHolds.end()
							                   ? HoldNoteType::Hidden
							                   : HoldNoteType::Normal;
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
						n.layer =
						    std::distance(hiSpeedGroupNames.begin(),
						                  std::find(hiSpeedGroupNames.begin(),
						                            hiSpeedGroupNames.end(), note.hiSpeedGroup));
						n.friction = false;
						n.ID = nextID++;
						n.parentID = startID;

						if (n.friction)
							printf("Note at %d-%d is friction", n.tick, n.lane);

						HoldStepType type =
						    note.type == 3 ? HoldStepType::Normal : HoldStepType::Hidden;
						if (stepIgnore.find(key) != stepIgnore.end())
						{
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
		if (sus.bpms.size() == 0)
			tempos.push_back(Tempo(0, 120));
		for (const auto& tempo : sus.bpms)
			tempos.push_back(Tempo(tempo.tick, tempo.bpm));

		std::map<int, TimeSignature> timeSignatures;
		for (const auto& sign : sus.barlengths)
		{
			auto fraction = barLengthToFraction(sign.length, 4.0f);
			timeSignatures.insert(std::pair<int, TimeSignature>(
			    sign.bar, TimeSignature{ sign.bar, fraction.first, fraction.second }));
		}

		std::vector<Layer> layers;
		std::unordered_map<int, HiSpeedChange> hiSpeedChanges;
		int hiSpeedChangesCount = 0;
		for (const auto& group : sus.hiSpeedGroups)
			for (const auto& change : group.hiSpeeds)
				hiSpeedChangesCount++;
		hiSpeedChanges.reserve(hiSpeedChangesCount);
		int hiSpeedGroupIndex = -1;
		for (const auto& speed : sus.hiSpeedGroups)
		{
			hiSpeedGroupIndex++;
			layers.push_back(Layer{ IO::formatString("#%d", hiSpeedGroupIndex) });
			for (const auto& change : speed.hiSpeeds)
			{
				int id = nextHiSpeedID++;
				hiSpeedChanges[id] = { id, change.tick, change.speed, hiSpeedGroupIndex };
			}
		}
		if (layers.size() == 0)
			layers.push_back(Layer{ "default" });

		Score score;
		score.metadata = metadata;
		score.notes = notes;
		score.holdNotes = holds;
		score.tempoChanges = tempos;
		score.timeSignatures = timeSignatures;
		score.layers = layers;
		score.hiSpeedChanges = hiSpeedChanges;
		score.skills = skills;
		score.fever = fever;

		return score;
	}

	SUS ScoreConverter::scoreToSus(const Score& score)
	{
		constexpr std::array<int, static_cast<int>(FlickType::FlickTypeCount)> flickToType{ 0, 1, 3,
			                                                                                4 };

		int offset = score.metadata.laneExtension == 0 ? 2 : score.metadata.laneExtension;

		std::vector<SUSNote> taps, directionals;
		std::vector<std::vector<SUSNote>> slides;
		std::vector<std::vector<SUSNote>> guides;
		std::vector<BPM> bpms;
		std::vector<BarLength> barlengths;
		std::vector<HiSpeed> hiSpeeds;

		std::unordered_map<int, std::string> hiSpeedGroupNames;

		for (int i = 0; i < score.layers.size(); i++)
		{
			char b36[36];
			IO::tostringBaseN(b36, i, 36);

			std::string b36Str(b36);

			if (b36Str.size() == 1)
				b36Str = "0" + b36Str;

			hiSpeedGroupNames[i] = b36Str;
		}

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
				taps.push_back(SUSNote{ note.tick, (int)note.lane + offset, (int)note.width, type,
				                        hiSpeedGroupNames[note.layer] });

				if (note.isFlick())
					directionals.push_back(SUSNote{ note.tick, (int)note.lane + offset,
					                                (int)note.width,
					                                flickToType[static_cast<int>(note.flick)] });
			}
			if (note.getType() == NoteType::Damage)
				taps.push_back(SUSNote{ note.tick, (int)note.lane + offset, (int)note.width, 4,
				                        hiSpeedGroupNames[note.layer] });
		}

		/*
		    Generally guides can be placed on the same tick and lane (key) as other notes.
		    If one of those notes or the guide is critical however, it will cause all the notes with
		   the same key to be critical.
		*/
		std::vector<HoldNote> exportHolds;
		exportHolds.reserve(score.holdNotes.size());

		for (const auto& [_, hold] : score.holdNotes)
			exportHolds.emplace_back(hold);

		std::sort(exportHolds.begin(), exportHolds.end(),
		          [&score](const HoldNote& a, const HoldNote& b)
		          { return score.notes.at(a.start.ID).tick < score.notes.at(b.start.ID).tick; });

		for (const auto& hold : exportHolds)
		{
			std::vector<SUSNote> slide;
			slide.reserve(hold.steps.size() + 2);

			const Note& start = score.notes.at(hold.start.ID);

			slide.push_back(SUSNote{ start.tick, (int)start.lane + offset, (int)start.width, 1,
			                         hiSpeedGroupNames[start.layer] });

			bool hasEase = hold.start.ease != EaseType::Linear;
			bool invisibleTapPoint = hold.startType == HoldNoteType::Hidden ||
			                         hold.startType != HoldNoteType::Normal && hasEase ||
			                         hold.startType != HoldNoteType::Normal && start.critical;

			if (hasEase)
				directionals.push_back(SUSNote{ start.tick, (int)start.lane + offset,
				                                (int)start.width,
				                                hold.start.ease == EaseType::EaseIn ? 2 : 6 });

			bool guideAlreadyOnCritical =
			    hold.isGuide() && criticalKeys.find(noteKey(start)) != criticalKeys.end();

			// We'll use type 1 to indicate it's a normal note
			int type = invisibleTapPoint ? 7 : start.friction ? 5 : 1;
			if ((start.critical || hold.guideColor == GuideColor::Yellow) &&
			    criticalKeys.find(noteKey(start)) == criticalKeys.end())
			{
				type++;
				criticalKeys.insert(noteKey(start));
			}

			if (type > 1 && !((type == 7 || type == 8) && guideAlreadyOnCritical))
				taps.push_back({ start.tick, (int)start.lane + offset, (int)start.width, type });

			for (const auto& step : hold.steps)
			{
				const Note& midNote = score.notes.at(step.ID);

				slide.push_back(SUSNote{
				    midNote.tick, (int)midNote.lane + offset, (int)midNote.width,
				    step.type == HoldStepType::Hidden ? 5 : 3, hiSpeedGroupNames[midNote.layer] });
				if (step.type == HoldStepType::Skip)
				{
					taps.push_back(
					    SUSNote{ midNote.tick, (int)midNote.lane + offset, (int)midNote.width, 3 });
				}
				else if (step.ease != EaseType::Linear)
				{
					int stepTapType = hold.isGuide() ? start.critical ? 8 : 7 : 1;
					taps.push_back(SUSNote{ midNote.tick, (int)midNote.lane + offset,
					                        (int)midNote.width, stepTapType });
					directionals.push_back(SUSNote{
					    midNote.tick, (int)midNote.lane + offset, (int)midNote.width,
					    step.ease == EaseType::EaseIn ? 2 : 6, hiSpeedGroupNames[midNote.layer] });
				}
			}

			const Note& end = score.notes.at(hold.end);

			slide.push_back(SUSNote{ end.tick, (int)end.lane + offset, (int)end.width, 2 });

			// Hidden and guide slides do not have flicks
			if (end.isFlick() && hold.endType == HoldNoteType::Normal)
			{
				directionals.push_back(SUSNote{ end.tick, (int)end.lane + offset, (int)end.width,
				                                flickToType[static_cast<int>(end.flick)] });
				// Critical friction notes use type 6 not 2
				if (end.critical && !start.critical && !end.friction)
					taps.push_back(SUSNote{ end.tick, (int)end.lane + offset, (int)end.width, 2 });
			}

			int endType = hold.endType == HoldNoteType::Hidden ? 7 : end.friction ? 5 : 1;
			if (end.critical)
			{
				endType++;
				criticalKeys.insert(noteKey(end));
			}
			if (hold.fadeType == FadeType::None)
				endType = 4;

			if (endType != 1 && endType != 2)
				taps.push_back(
				    SUSNote{ end.tick, (int)end.lane + offset, (int)end.width, endType });

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
			barlengths.push_back(
			    BarLength{ ts.measure, ((float)ts.numerator / (float)ts.denominator) * 4 });

		hiSpeeds.reserve(score.hiSpeedChanges.size());
		for (const auto& [_, hiSpeed] : score.hiSpeedChanges)
			hiSpeeds.push_back(HiSpeed{ hiSpeed.tick, hiSpeed.speed });
		SUSMetadata metadata;
		metadata.data["title"] = score.metadata.title;
		metadata.data["artist"] = score.metadata.artist;
		metadata.data["designer"] = score.metadata.author;
		metadata.requests.push_back("ticks_per_beat 480");
		metadata.requests.push_back("side_lane true");
		int laneOffset;
		if (score.metadata.laneExtension == 0)
		{
			laneOffset = 0;
		}
		else
		{
			laneOffset = -score.metadata.laneExtension + 2;
		}
		metadata.requests.push_back("lane_offset " + std::to_string(laneOffset));
		// milliseconds -> seconds
		metadata.waveOffset = score.metadata.musicOffset / 1000.0f;

		std::vector<HiSpeedGroup> hiSpeedGroup;

		for (int i = 0; i < score.layers.size(); i++)
		{
			std::vector<HiSpeed> hiSpeeds;

			for (const auto& [_, hiSpeed] : score.hiSpeedChanges)
			{
				if (hiSpeed.layer == i)
				{
					hiSpeeds.push_back(HiSpeed{ hiSpeed.tick, hiSpeed.speed });
				}
			}
			hiSpeedGroup.push_back(HiSpeedGroup{ hiSpeedGroupNames[i], hiSpeeds });
		}

		return SUS{ metadata, taps,       directionals, slides,    guides,
			        bpms,     barlengths, hiSpeedGroup, laneOffset };
	}

	json ScoreConverter::scoreToUsc(const Score& score)
	{
		json vusc;
		json usc;

		usc["offset"] = score.metadata.musicOffset / -1000.0f;

		std::vector<json> objects;

		for (const auto& bpm : score.tempoChanges)
		{
			json obj;
			obj["type"] = "bpm";
			obj["beat"] = bpm.tick / (double)TICKS_PER_BEAT;
			obj["bpm"] = bpm.bpm;
			objects.push_back(obj);
		}

		for (int i = 0; i < score.layers.size(); ++i)
		{
			json timeScaleGroup;
			timeScaleGroup["type"] = "timeScaleGroup";
			std::vector<json> timeScaleObjects;
			for (const auto& [_, hs] : score.hiSpeedChanges)
			{
				if (hs.layer != i)
				{
					continue;
				}
				json obj;
				obj["beat"] = hs.tick / (double)TICKS_PER_BEAT;
				obj["timeScale"] = hs.speed;
				timeScaleObjects.push_back(obj);
			}
			timeScaleGroup["changes"] = timeScaleObjects;
			objects.push_back(timeScaleGroup);
		}

		for (const auto& [_, note] : score.notes)
		{
			if (note.getType() == NoteType::Tap)
			{
				json obj;
				obj["type"] = "single";
				obj["beat"] = note.tick / (double)TICKS_PER_BEAT;
				obj["size"] = note.width / 2.0;
				obj["lane"] = note.lane - 6 + (note.width / 2.0);
				obj["critical"] = note.critical;
				obj["trace"] = note.friction;
				if (note.flick != FlickType::None)
				{
					obj["direction"] = note.flick == FlickType::Default ? "up"
					                   : note.flick == FlickType::Left  ? "left"
					                                                    : "right";
				}
				obj["timeScaleGroup"] = note.layer;
				objects.push_back(obj);
			}
			else if (note.getType() == NoteType::Damage)
			{
				json obj;
				obj["type"] = "damage";
				obj["beat"] = note.tick / (double)TICKS_PER_BEAT;
				obj["size"] = note.width / 2.0;
				obj["lane"] = note.lane - 6 + (note.width / 2.0);
				obj["timeScaleGroup"] = note.layer;
				objects.push_back(obj);
			}
		}
		for (const auto& [_, note] : score.holdNotes)
		{
			json obj;
			if (note.isGuide())
			{
				obj["type"] = "guide";
				auto& start = score.notes.at(note.start.ID);
				obj["color"] = guideColors[(int)note.guideColor];
				obj["fade"] = note.fadeType == FadeType::None ? "none"
				              : note.fadeType == FadeType::In ? "in"
				                                              : "out";

				std::vector<json> steps;
				steps.reserve(note.steps.size() + 1);

				json startStep;
				startStep["beat"] = start.tick / (double)TICKS_PER_BEAT;
				startStep["size"] = start.width / 2.0;
				startStep["lane"] = start.lane - 6 + (start.width / 2.0);
				startStep["ease"] = easeNames[(int)note.start.ease];
				startStep["timeScaleGroup"] = start.layer;
				steps.push_back(startStep);

				for (const auto& step : note.steps)
				{
					json stepObj;
					auto& stepNote = score.notes.at(step.ID);
					stepObj["beat"] = stepNote.tick / (double)TICKS_PER_BEAT;
					stepObj["size"] = stepNote.width / 2.0;
					stepObj["lane"] = stepNote.lane - 6 + (stepNote.width / 2.0);
					stepObj["ease"] = easeNames[(int)step.ease];
					stepObj["timeScaleGroup"] = stepNote.layer;
					steps.push_back(stepObj);
				}

				json endStep;
				auto& end = score.notes.at(note.end);
				endStep["beat"] = end.tick / (double)TICKS_PER_BEAT;
				endStep["size"] = end.width / 2.0;
				endStep["lane"] = end.lane - 6 + (end.width / 2.0);
				endStep["ease"] = "linear";
				endStep["timeScaleGroup"] = end.layer;
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
			startStep["beat"] = start.tick / (double)TICKS_PER_BEAT;
			startStep["size"] = start.width / 2.0;
			startStep["lane"] = start.lane - 6 + (start.width / 2.0);
			startStep["critical"] = start.critical;
			startStep["ease"] = easeNames[(int)note.start.ease];
			startStep["judgeType"] = note.startType == HoldNoteType::Hidden ? "none"
			                         : start.friction                       ? "trace"
			                                                                : "normal";
			startStep["timeScaleGroup"] = start.layer;
			steps.push_back(startStep);

			for (const auto& step : note.steps)
			{
				json stepObj;
				auto& stepNote = score.notes.at(step.ID);
				stepObj["type"] = step.type == HoldStepType::Skip ? "attach" : "tick";
				stepObj["beat"] = stepNote.tick / (double)TICKS_PER_BEAT;
				stepObj["size"] = stepNote.width / 2.0;
				stepObj["lane"] = stepNote.lane - 6 + (stepNote.width / 2.0);
				if (step.type != HoldStepType::Hidden)
				{
					stepObj["critical"] = stepNote.critical;
				}
				stepObj["ease"] = easeNames[(int)step.ease];
				stepObj["timeScaleGroup"] = stepNote.layer;
				steps.push_back(stepObj);
			}

			json endStep;
			auto& end = score.notes.at(note.end);
			endStep["type"] = "end";
			endStep["beat"] = end.tick / (double)TICKS_PER_BEAT;
			endStep["size"] = end.width / 2.0;
			endStep["lane"] = end.lane - 6 + (end.width / 2.0);
			endStep["critical"] = end.critical;
			endStep["judgeType"] = note.endType == HoldNoteType::Hidden ? "none"
			                       : end.friction                       ? "trace"
			                                                            : "normal";
			if (end.flick != FlickType::None)
			{
				endStep["direction"] = end.flick == FlickType::Default ? "up"
				                       : end.flick == FlickType::Left  ? "left"
				                                                       : "right";
			}
			endStep["timeScaleGroup"] = end.layer;
			steps.push_back(endStep);

			obj["connections"] = steps;
			objects.push_back(obj);
		}

		usc["objects"] = objects;

		vusc["version"] = 2;
		vusc["usc"] = usc;
		return vusc;
	}

	Score ScoreConverter::uscToScore(const json& vusc)
	{
		Score score;
		if (vusc["version"] != 2)
		{
			throw std::runtime_error("Invalid version");
		}
		json usc = vusc["usc"];
		score.layers.clear();
		score.hiSpeedChanges.clear();
		score.tempoChanges.clear();

		score.metadata.musicOffset = usc["offset"].get<float>() * -1000.0f;

		for (const auto& obj : usc["objects"].get<std::vector<json>>())
		{
			if (obj["type"] == "bpm")
			{
				score.tempoChanges.push_back(Tempo{
				    (int)(obj["beat"].get<double>() * TICKS_PER_BEAT), obj["bpm"].get<float>() });
			}
			else if (obj["type"] == "timeScaleGroup")
			{
				int index = score.layers.size();
				score.layers.push_back(Layer{ IO::formatString("#%d", index) });
				for (const auto& change : obj["changes"])
				{
					int id = nextID++;
					score.hiSpeedChanges[id] =
					    HiSpeedChange{ id, (int)(change["beat"].get<double>() * TICKS_PER_BEAT),
						               change["timeScale"].get<float>(), index };
				}
			}
			else if (obj["type"] == "single")
			{
				Note note(NoteType::Tap);
				note.tick = obj["beat"].get<double>() * TICKS_PER_BEAT;
				note.width = obj["size"].get<float>() * 2;
				note.lane = obj["lane"].get<float>() + 6 - obj["size"].get<float>();
				note.critical = obj["critical"].get<bool>();
				note.friction = obj["trace"].get<bool>();
				if (obj.contains("direction"))
				{
					std::string dir = obj["direction"].get<std::string>();
					note.flick = dir == "up"     ? FlickType::Default
					             : dir == "left" ? FlickType::Left
					                             : FlickType::Right;
				}
				else
				{
					note.flick = FlickType::None;
				}
				note.layer = obj["timeScaleGroup"].get<int>();
				note.ID = nextID++;
				score.notes[note.ID] = note;
			}
			else if (obj["type"] == "damage")
			{
				Note note(NoteType::Damage);
				note.tick = obj["beat"].get<double>() * TICKS_PER_BEAT;
				note.width = obj["size"].get<float>() * 2;
				note.lane = obj["lane"].get<float>() + 6 - obj["size"].get<float>();
				note.layer = obj["timeScaleGroup"].get<int>();
				note.ID = nextID++;
				score.notes[note.ID] = note;
			}
			else if (obj["type"] == "guide")
			{
				HoldNote hold;

				auto color = obj["color"].get<std::string>();

				if (color == "neutral")
				{
					hold.guideColor = GuideColor::Neutral;
				}
				else if (color == "red")
				{
					hold.guideColor = GuideColor::Red;
				}
				else if (color == "green")
				{
					hold.guideColor = GuideColor::Green;
				}
				else if (color == "blue")
				{
					hold.guideColor = GuideColor::Blue;
				}
				else if (color == "yellow")
				{
					hold.guideColor = GuideColor::Yellow;
				}
				else if (color == "purple")
				{
					hold.guideColor = GuideColor::Purple;
				}
				else if (color == "cyan")
				{
					hold.guideColor = GuideColor::Cyan;
				}
				else if (color == "black")
				{
					hold.guideColor = GuideColor::Black;
				}
				hold.fadeType = obj["fade"].get<std::string>() == "none" ? FadeType::None
				                : obj["fade"].get<std::string>() == "in" ? FadeType::In
				                                                         : FadeType::Out;

				for (int i = 0; i < obj["midpoints"].size(); i++)
				{
					const auto& step = obj["midpoints"][i];
					if (i == 0)
					{
						Note startNote(NoteType::Hold);
						startNote.tick = step["beat"].get<double>() * TICKS_PER_BEAT;
						startNote.lane = step["lane"].get<float>() + 6 - step["size"].get<float>();
						startNote.layer = step["timeScaleGroup"].get<int>();
						startNote.ID = nextID++;
						startNote.width = step["size"].get<float>() * 2;
						score.notes[startNote.ID] = startNote;
						hold.start.ID = startNote.ID;
						if (step["ease"].get<std::string>() == "in")
						{
							hold.start.ease = EaseType::EaseIn;
						}
						else if (step["ease"].get<std::string>() == "out")
						{
							hold.start.ease = EaseType::EaseOut;
						}
						else if (step["ease"].get<std::string>() == "inout")
						{
							hold.start.ease = EaseType::EaseInOut;
						}
						else if (step["ease"].get<std::string>() == "outin")
						{
							hold.start.ease = EaseType::EaseOutIn;
						}
						else
						{
							hold.start.ease = EaseType::Linear;
						}
						hold.startType = HoldNoteType::Guide;
					}
					else if (i == obj["midpoints"].size() - 1)
					{
						Note endNote(NoteType::HoldEnd);
						endNote.tick = step["beat"].get<double>() * TICKS_PER_BEAT;
						endNote.lane = step["lane"].get<float>() + 6 - step["size"].get<float>();
						endNote.layer = step["timeScaleGroup"].get<int>();
						endNote.ID = nextID++;
						endNote.parentID = hold.start.ID;
						endNote.width = step["size"].get<float>() * 2;
						score.notes[endNote.ID] = endNote;
						hold.end = endNote.ID;
						hold.endType = HoldNoteType::Guide;
					}
					else
					{
						HoldStep s;
						Note mid(NoteType::HoldMid);
						mid.tick = step["beat"].get<double>() * TICKS_PER_BEAT;
						mid.lane = step["lane"].get<float>() + 6 - step["size"].get<float>();
						mid.layer = step["timeScaleGroup"].get<int>();
						mid.ID = nextID++;
						mid.parentID = hold.start.ID;
						mid.width = step["size"].get<float>() * 2;
						score.notes[mid.ID] = mid;
						s.ID = mid.ID;
						if (step["ease"].get<std::string>() == "in")
						{
							s.ease = EaseType::EaseIn;
						}
						else if (step["ease"].get<std::string>() == "out")
						{
							s.ease = EaseType::EaseOut;
						}
						else if (step["ease"].get<std::string>() == "inout")
						{
							s.ease = EaseType::EaseInOut;
						}
						else if (step["ease"].get<std::string>() == "outin")
						{
							s.ease = EaseType::EaseOutIn;
						}
						else
						{
							s.ease = EaseType::Linear;
						}
						s.type = HoldStepType::Hidden;
						hold.steps.push_back(s);
					}
				}
				score.holdNotes[hold.start.ID] = hold;
			}
			else if (obj["type"] == "slide")
			{
				HoldNote hold;
				hold.fadeType = FadeType::None;

				auto connections = obj["connections"].get<std::vector<json>>();
				std::stable_sort(connections.begin(), connections.end(),
				                 [](const json& a, const json& b)
				                 {
					                 if (a["type"] == "start")
					                 {
						                 return true;
					                 }
					                 else if (b["type"] == "start")
					                 {
						                 return false;
					                 }
					                 else if (a["type"] == "end")
					                 {
						                 return false;
					                 }
					                 else if (b["type"] == "end")
					                 {
						                 return true;
					                 }
					                 return a["beat"].get<double>() < b["beat"].get<float>();
				                 });

				bool isCritical;
				for (const auto& step : connections)
				{
					auto type = step["type"].get<std::string>();
					if (type == "start")
					{
						Note startNote(NoteType::Hold);
						startNote.tick = step["beat"].get<double>() * TICKS_PER_BEAT;
						startNote.lane = step["lane"].get<float>() + 6 - step["size"].get<float>();
						startNote.layer = step["timeScaleGroup"].get<int>();
						startNote.critical = step["critical"].get<bool>();
						isCritical = startNote.critical;
						startNote.width = step["size"].get<float>() * 2;
						if (step["judgeType"].get<std::string>() == "trace")
						{
							startNote.friction = true;
							hold.startType = HoldNoteType::Normal;
						}
						else if (step["judgeType"].get<std::string>() == "none")
						{
							hold.startType = HoldNoteType::Hidden;
						}
						else
						{
							hold.startType = HoldNoteType::Normal;
						}
						startNote.ID = nextID++;
						score.notes[startNote.ID] = startNote;
						hold.start.ID = startNote.ID;
						if (step["ease"].get<std::string>() == "in")
						{
							hold.start.ease = EaseType::EaseIn;
						}
						else if (step["ease"].get<std::string>() == "out")
						{
							hold.start.ease = EaseType::EaseOut;
						}
						else if (step["ease"].get<std::string>() == "inout")
						{
							hold.start.ease = EaseType::EaseInOut;
						}
						else if (step["ease"].get<std::string>() == "outin")
						{
							hold.start.ease = EaseType::EaseOutIn;
						}
						else
						{
							hold.start.ease = EaseType::Linear;
						}
					}
					else if (type == "end")
					{
						Note endNote(NoteType::HoldEnd);
						endNote.tick = step["beat"].get<double>() * TICKS_PER_BEAT;
						endNote.lane = step["lane"].get<float>() + 6 - step["size"].get<float>();
						endNote.layer = step["timeScaleGroup"].get<int>();
						endNote.width = step["size"].get<float>() * 2;
						endNote.critical = isCritical || step["critical"].get<bool>();
						endNote.flick = step.contains("direction")
						                    ? step["direction"].get<std::string>() == "up"
						                          ? FlickType::Default
						                      : step["direction"].get<std::string>() == "left"
						                          ? FlickType::Left
						                          : FlickType::Right
						                    : FlickType::None;
						endNote.ID = nextID++;
						endNote.parentID = hold.start.ID;

						if (step["judgeType"].get<std::string>() == "trace")
						{
							endNote.friction = true;
							hold.endType = HoldNoteType::Normal;
						}
						else if (step["judgeType"].get<std::string>() == "none")
						{
							hold.endType = HoldNoteType::Hidden;
						}
						else
						{
							hold.endType = HoldNoteType::Normal;
						}
						score.notes[endNote.ID] = endNote;
						hold.end = endNote.ID;
					}
					else
					{
						HoldStep s;
						Note mid(NoteType::HoldMid);
						mid.tick = step["beat"].get<double>() * TICKS_PER_BEAT;
						mid.lane = step["lane"].get<float>() + 6 - step["size"].get<float>();
						mid.width = step["size"].get<float>() * 2;
						mid.layer = step["timeScaleGroup"].get<int>();
						mid.critical = isCritical;
						mid.ID = nextID++;
						mid.parentID = hold.start.ID;
						score.notes[mid.ID] = mid;
						s.ID = mid.ID;
						if (step["ease"].get<std::string>() == "in")
						{
							s.ease = EaseType::EaseIn;
						}
						else if (step["ease"].get<std::string>() == "out")
						{
							s.ease = EaseType::EaseOut;
						}
						else if (step["ease"].get<std::string>() == "inout")
						{
							s.ease = EaseType::EaseInOut;
						}
						else if (step["ease"].get<std::string>() == "outin")
						{
							s.ease = EaseType::EaseOutIn;
						}
						else
						{
							s.ease = EaseType::Linear;
						}

						if (type == "tick")
						{
							if (step.contains("critical"))
							{
								s.type = HoldStepType::Normal;
							}
							else
							{
								s.type = HoldStepType::Hidden;
							}
						}
						else if (type == "attach")
						{
							s.type = HoldStepType::Skip;
						}
						hold.steps.push_back(s);
					}
				}

				score.holdNotes[hold.start.ID] = hold;
			}
		}

		if (score.layers.size() == 0)
		{
			score.layers.push_back(Layer{ "#0" });
		}
		if (score.tempoChanges.size() == 0)
		{
			score.tempoChanges.push_back(Tempo{ 0, 120 });
		}

		return score;
	}
}
