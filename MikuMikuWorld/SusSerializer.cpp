#include "SusSerializer.h"
#include "SusExporter.h"
#include "SusParser.h"
#include "ScoreConverter.h"
#include "SUS.h"

namespace MikuMikuWorld
{	
	void SusSerializer::serialize(Score score, std::string filename)
	{
		SUS sus = ScoreConverter::scoreToSus(score);

		SusExporter exporter{};
		exporter.dump(sus, filename, exportComment);
	}

	Score SusSerializer::deserialize(std::string filename)
	{
		SUS sus = SusParser().parse(filename);
		return ScoreConverter::susToScore(sus);
	}

	std::pair<int, int> SusSerializer::barLengthToFraction(float length, float fractionDenom)
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

	std::string SusSerializer::noteKey(const SUSNote& note)
	{
		return IO::formatString("%d-%d", note.tick, note.lane);
	}

	std::string SusSerializer::noteKey(const Note& note)
	{
		return IO::formatString("%d-%d", note.tick, note.lane);
	}

	Score SusSerializer::susToScore(const SUS& sus)
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

		for (const auto& slide : sus.slides)
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

		for (const auto& note : sus.taps)
		{
			if (note.type == 4)
			{
				skills.push_back(SkillTrigger{ nextSkillID++, note.tick });
			}
			else if (note.lane == 15 && note.width == 1)
			{
				if (note.type == 1)
					fever.startTick = note.tick;
				else if (note.type == 2)
					fever.endTick = note.tick;
			}
			else if (note.type == 7 || note.type == 8)
			{
				// Invisible slide tap point
				continue;
			}

			if (note.lane - 2 < MIN_LANE || note.lane - 2 > MAX_LANE)
				continue;

			const std::string key = noteKey(note);

			// Conflict with skip slide steps and hidden holds
			if (slideKeys.find(key) != slideKeys.end())
				continue;

			Note n(NoteType::Tap, note.tick, note.lane - 2, note.width);
			n.critical = criticals.find(key) != criticals.end();
			n.friction = frictions.find(key) != frictions.end();
			n.flick = flicks.find(key) != flicks.end() ? flicks[key] : FlickType::None;
			n.ID = nextID++;

			notes[n.ID] = n;
		}

		// Not the best way to handle this but it will do the job
		auto slideFillFunc = [&](const std::vector<std::vector<SUSNote>>& slides, bool isGuide)
			{
				for (const auto& slide : slides)
				{
					const std::string key = noteKey(slide[0]);

					auto start = std::find_if(slide.begin(), slide.end(),
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
							n.ID = startID;

							if (isGuide)
							{
								hold.startType = HoldNoteType::Guide;
							}
							else
							{
								n.friction = frictions.find(noteKey(note)) != frictions.end();
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
							}
							else
							{
								n.flick = flicks.find(key) != flicks.end() ? flicks[key] : FlickType::None;
								n.friction = frictions.find(noteKey(note)) != frictions.end();
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
							if (stepIgnore.find(key) != stepIgnore.end())
								type = HoldStepType::Skip;

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

		std::sort(hiSpeedChanges.begin(), hiSpeedChanges.end(),
			[](const auto& a, const auto& b) { return a.tick < b.tick; });

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

	SUS SusSerializer::scoreToSus(const Score& score)
	{
		constexpr std::array<int, static_cast<int>(FlickType::FlickTypeCount)> flickToType{ 0, 1, 3, 4 };

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
				taps.push_back(SUSNote{ note.tick, note.lane + 2, note.width, type });

				if (note.isFlick())
					directionals.push_back(SUSNote{ note.tick, note.lane + 2, note.width, flickToType[static_cast<int>(note.flick)] });
			}
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
				const Note& n1 = score.notes.at(a.start.ID);
				const Note& n2 = score.notes.at(b.start.ID);
				return n1.tick == n2.tick ? n1.ID < n2.ID : n1.tick < n2.tick;
			});

		for (const auto& hold : exportHolds)
		{
			std::vector<SUSNote> slide;
			slide.reserve(hold.steps.size() + 2);

			const Note& start = score.notes.at(hold.start.ID);

			slide.push_back(SUSNote{ start.tick, start.lane + 2, start.width, 1 });

			bool hasEase = hold.start.ease != EaseType::Linear;
			bool invisibleTapPoint =
				hold.startType == HoldNoteType::Hidden ||
				hold.startType != HoldNoteType::Normal && hasEase ||
				hold.startType != HoldNoteType::Normal && start.critical;

			if (hasEase)
				directionals.push_back(SUSNote{ start.tick, start.lane + 2, start.width, hold.start.ease == EaseType::EaseIn ? 2 : 6 });

			bool guideAlreadyOnCritical = hold.isGuide() && criticalKeys.find(noteKey(start)) != criticalKeys.end();

			// We'll use type 1 to indicate it's a normal note
			int type = invisibleTapPoint ? 7 : start.friction ? 5 : 1;
			if (start.critical && criticalKeys.find(noteKey(start)) == criticalKeys.end())
			{
				type++;
				criticalKeys.insert(noteKey(start));
			}

			if (type > 1 && !((type == 7 || type == 8) && guideAlreadyOnCritical))
				taps.push_back({ start.tick, start.lane + 2, start.width, type });

			for (const auto& step : hold.steps)
			{
				const Note& midNote = score.notes.at(step.ID);

				slide.push_back(SUSNote{ midNote.tick, midNote.lane + 2, midNote.width, step.type == HoldStepType::Hidden ? 5 : 3 });
				if (step.type == HoldStepType::Skip)
				{
					taps.push_back(SUSNote{ midNote.tick, midNote.lane + 2, midNote.width, 3 });
				}
				else if (step.ease != EaseType::Linear)
				{
					int stepTapType = hold.isGuide() ? start.critical ? 8 : 7 : 1;
					taps.push_back(SUSNote{ midNote.tick, midNote.lane + 2, midNote.width, stepTapType });
					directionals.push_back(SUSNote{ midNote.tick, midNote.lane + 2, midNote.width, step.ease == EaseType::EaseIn ? 2 : 6 });
				}
			}

			const Note& end = score.notes.at(hold.end);

			slide.push_back(SUSNote{ end.tick, end.lane + 2, end.width, 2 });

			// Hidden and guide slides do not have flicks
			if (end.isFlick() && hold.endType == HoldNoteType::Normal)
			{
				directionals.push_back(SUSNote{ end.tick, end.lane + 2, end.width, flickToType[static_cast<int>(end.flick)] });

				// Critical friction notes use type 6 not 2
				if (end.critical && !start.critical && !end.friction)
					taps.push_back(SUSNote{ end.tick, end.lane + 2, end.width, 2 });
			}

			int endType = hold.endType == HoldNoteType::Hidden ? 7 : end.friction ? 5 : 1;
			if (end.critical)
			{
				endType++;
				criticalKeys.insert(noteKey(end));
			}

			if (endType != 1 && endType != 2)
				taps.push_back(SUSNote{ end.tick, end.lane + 2, end.width, endType });

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

		// milliseconds -> seconds
		metadata.waveOffset = score.metadata.musicOffset / 1000.0f;

		return SUS{ metadata, taps, directionals, slides, guides, bpms, barlengths, hiSpeeds };
	}
}