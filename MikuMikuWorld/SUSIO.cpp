#include "SUSIO.h"
#include "Constants.h"
#include <unordered_set>
#include <algorithm>
#include <numeric>

namespace MikuMikuWorld
{
	std::string SUSIO::noteKey(const SUSNote& note)
	{
		return std::to_string(note.tick) + "-" + std::to_string(note.lane);
	}

	std::pair<int, int> SUSIO::simplify4(int numerator, int denominator)
	{
		int g = std::gcd(numerator, denominator);
		int n = numerator / g;
		int d = denominator / g;
		if (d % 4 == 0)
			return std::pair<int, int>(n, d);
		else
			return std::pair<int, int>(n * 4, d * 4);
	}

	Score SUSIO::importSUS(const std::string& filename)
	{
		SUS sus = loadSUS(filename);

		ScoreMetadata metadata{ sus.metadata.data["title"], sus.metadata.data["artist"], sus.metadata.data["designer"] };
		metadata.musicOffset = sus.metadata.waveOffset * 1000;

		std::unordered_map<std::string, FlickType> flicks;
		std::unordered_set<std::string> criticals;
		std::unordered_set<std::string> stepIgnore;
		std::unordered_set<std::string> easeIns;
		std::unordered_set<std::string> easeOuts;
		std::unordered_set<std::string> slideKeys;

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
				flicks.insert_or_assign(key, FlickType::Up);
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
			default:
				break;
			}
		}

		std::unordered_set<std::string> tapKeys;
		tapKeys.reserve(sus.taps.size());

		std::unordered_map<int, Note> notes;
		notes.reserve(sus.taps.size());

		std::unordered_map<int, HoldNote> holds;
		holds.reserve(sus.slides.size());

		for (const auto& note : sus.taps)
		{
			if (note.lane - 2 < MIN_LANE || note.lane - 2 > MAX_LANE)
				continue;

			const std::string key = noteKey(note);
			if (slideKeys.find(key) != slideKeys.end() || (note.type != 1 && note.type != 2))
				continue;

			if (tapKeys.find(key) != tapKeys.end())
				continue;

			tapKeys.insert(key);
			Note n(NoteType::Tap);
			n.tick = note.tick;
			n.lane = note.lane - 2;
			n.width = note.width;
			n.critical = note.type == 2;
			n.flick = flicks.find(key) != flicks.end() ? flicks[key] : FlickType::None;
			n.ID = nextID++;

			notes[n.ID] = n;
		}

		for (const auto& slide : sus.slides)
		{
			const std::string key = noteKey(slide[0]);

			auto start = std::find_if(slide.begin(), slide.end(),
				[](const SUSNote& a) { return a.type == 1 || a.type == 2; });

			if (start == slide.end())
				continue;

			bool critical = criticals.find(key) != criticals.end();

			HoldNote hold;
			int startID = nextID++;
			for (const auto& note : slide)
			{
				const std::string key = noteKey(note);

				EaseType ease = EaseType::None;
				if (easeIns.find(key) != easeIns.end())
					ease = EaseType::EaseIn;
				else if (easeOuts.find(key) != easeOuts.end())
					ease = EaseType::EaseOut;

				switch (note.type)
				{
					// start
				case 1:
				{
					Note n(NoteType::Hold);
					n.tick = note.tick;
					n.lane = note.lane - 2;
					n.width = note.width;
					n.critical = critical;
					n.ID = startID;

					notes[n.ID] = n;
					hold.start = HoldStep{ n.ID, HoldStepType::Visible, ease };
				}
				break;
				// end
				case 2:
				{
					Note n(NoteType::HoldEnd);
					n.tick = note.tick;
					n.lane = note.lane - 2;
					n.width = note.width;
					n.critical = (critical ? true : (criticals.find(key) != criticals.end()));
					n.ID = nextID++;
					n.parentID = startID;
					n.flick = flicks.find(key) != flicks.end() ? flicks[key] : FlickType::None;

					notes[n.ID] = n;
					hold.end = n.ID;
				}
				break;
				// mid
				case 3:
				case 5:
				{

					Note n(NoteType::HoldMid);
					n.tick = note.tick;
					n.lane = note.lane - 2;
					n.width = note.width;
					n.critical = critical;
					n.ID = nextID++;
					n.parentID = startID;

					HoldStepType type = note.type == 3 ? HoldStepType::Visible : HoldStepType::Invisible;
					if (stepIgnore.find(key) != stepIgnore.end())
						type = HoldStepType::Ignored;

					notes[n.ID] = n;
					hold.steps.push_back(HoldStep{ n.ID, type, ease });
				}
				break;
				default:
					break;
				}
			}

			holds[startID] = hold;
			if (notes.at(hold.end).critical && !notes.at(hold.end).isFlick())
			{
				notes.at(hold.start.ID).critical = true;
				for (const auto& step : hold.steps)
					notes.at(step.ID).critical = true;
			}
		}

		std::vector<Tempo> tempos;
		tempos.reserve(sus.bpms.size());
		for (const auto& tempo : sus.bpms)
			tempos.push_back(Tempo(tempo.tick, tempo.bpm));

		std::map<int, TimeSignature> timeSignatures;
		for (const auto& sign : sus.barlengths)
		{
			auto fraction = simplify4(sign.length, 4);
			timeSignatures.insert(std::pair<int, TimeSignature>(sign.bar, TimeSignature{ sign.bar, fraction.first, fraction.second }));
		}

		Score score;
		score.metadata = metadata;
		score.notes = notes;
		score.holdNotes = holds;
		score.tempoChanges = tempos;
		score.timeSignatures = timeSignatures;

		return score;
	}

	void SUSIO::exportSUS(const Score& score, const std::string& filename)
	{
		std::unordered_map<FlickType, int> flickToType;
		flickToType[FlickType::Up] = 1;
		flickToType[FlickType::Left] = 3;
		flickToType[FlickType::Right] = 4;

		std::vector<SUSNote> taps, directionals;
		std::vector<std::vector<SUSNote>> slides;
		std::vector<BPM> bpms;
		std::vector<BarLength> barlengths;

		for (const auto& it : score.notes)
		{
			const Note& note = it.second;
			if (note.getType() == NoteType::Tap)
			{
				taps.push_back(SUSNote{ note.tick, note.lane + 2, note.width, note.critical ? 2 : 1 });
				if (note.isFlick())
					directionals.push_back(SUSNote{ note.tick, note.lane + 2, note.width, flickToType[note.flick] });
			}
		}

		for (const auto& it : score.holdNotes)
		{
			std::vector<SUSNote> slide;
			slide.reserve(it.second.steps.size() + 2);

			const HoldNote& hold = it.second;
			const Note& start = score.notes.at(hold.start.ID);

			slide.push_back(SUSNote{ start.tick, start.lane + 2, start.width, 1 });
			EaseType ease = hold.start.ease;
			if (ease != EaseType::None)
			{
				taps.push_back(SUSNote{ start.tick, start.lane + 2, start.width, 1 });
				directionals.push_back(SUSNote{ start.tick, start.lane + 2, start.width, ease == EaseType::EaseIn ? 2 : 6 });
			}

			if (start.critical)
				taps.push_back(SUSNote{ start.tick, start.lane + 2, start.width, 2 });

			for (const auto& step : hold.steps)
			{
				const Note& midNote = score.notes.at(step.ID);

				slide.push_back(SUSNote{ midNote.tick, midNote.lane + 2, midNote.width, step.type == HoldStepType::Invisible ? 5 : 3 });
				if (step.type == HoldStepType::Ignored)
					taps.push_back(SUSNote{ midNote.tick, midNote.lane + 2, midNote.width, 3 });
				else if (step.ease != EaseType::None)
				{
					taps.push_back(SUSNote{ midNote.tick, midNote.lane + 2, midNote.width, 1 });
					directionals.push_back(SUSNote{ midNote.tick, midNote.lane + 2, midNote.width, step.ease == EaseType::EaseIn ? 2 : 6 });
				}
			}

			const Note& end = score.notes.at(hold.end);

			slide.push_back(SUSNote{ end.tick, end.lane + 2, end.width, 2 });
			if (end.isFlick())
				directionals.push_back(SUSNote{ end.tick, end.lane + 2, end.width, flickToType[end.flick] });

			if (end.critical)
				taps.push_back(SUSNote{ end.tick, end.lane + 2, end.width, 2 });

			slides.push_back(slide);
		}

		for (const auto& tempo : score.tempoChanges)
			bpms.push_back(BPM{ tempo.tick, tempo.bpm });

		if (!bpms.size())
			bpms.push_back(BPM{ 0, 120 });

		std::sort(taps.begin(), taps.end(),
			[](SUSNote& a, SUSNote& b) { return a.tick < b.tick; });

		std::sort(slides.begin(), slides.end(),
			[](auto& a, auto& b) { return a[0].tick < b[0].tick; });

		std::sort(directionals.begin(), directionals.end(),
			[](SUSNote& a, SUSNote& b) {return a.tick < b.tick; });

		std::sort(bpms.begin(), bpms.end(),
			[](BPM& a, BPM& b) { return a.tick < b.tick; });

		for (const auto& it : score.timeSignatures)
		{
			const TimeSignature& ts = it.second;
			barlengths.push_back(BarLength{ ts.measure, ((float)ts.numerator / (float)ts.denominator) * 4 });
		}

		std::sort(barlengths.begin(), barlengths.end(),
			[](BarLength& a, BarLength& b) { return a.bar < b.bar; });

		SUSMetadata metadata;
		metadata.data["title"] = score.metadata.title;
		metadata.data["artist"] = score.metadata.artist;
		metadata.data["designer"] = score.metadata.author;
		metadata.waveOffset = score.metadata.musicOffset / 1000.0f;
		metadata.requests.push_back("ticks_per_beat 480");

		SUS sus{ metadata, taps, directionals, slides, bpms, barlengths };
		saveSUS(sus, filename);
	}
}