#include "JsonIO.h"

using namespace nlohmann;

namespace jsonIO
{
	mmw::Note jsonToNote(const json& data, mmw::NoteType type)
	{
		mmw::Note note(type);

		note.tick = tryGetValue<int>(data, "tick", 0);
		note.lane = tryGetValue<int>(data, "lane", 0);
		note.width = tryGetValue<int>(data, "width", 3);

		if (note.getType() != mmw::NoteType::HoldMid)
		{
			note.critical = tryGetValue<bool>(data, "critical", false);
			note.friction = tryGetValue<bool>(data, "friction", false);
		}

		if (!note.hasEase())
		{
			std::string flickString = tryGetValue<std::string>(data, "flick", "none");
			std::transform(flickString.begin(), flickString.end(), flickString.begin(), ::tolower);

			if (flickString == "up" || flickString == "default")
				note.flick = mmw::FlickType::Default;
			else if (flickString == "left")
				note.flick = mmw::FlickType::Left;
			else if (flickString == "right")
				note.flick = mmw::FlickType::Right;
		}

		return note;
	}

	static json noteToJson(const mmw::Note& note)
	{
		json data;
		data["tick"] = note.tick;
		data["lane"] = note.lane;
		data["width"] = note.width;

		if (note.getType() != mmw::NoteType::HoldMid)
		{
			data["critical"] = note.critical;
			data["friction"] = note.friction;
		}

		if (!note.hasEase())
		{
			data["flick"] = mmw::flickTypes[(int)note.flick];
		}

		return data;
	}

	json noteSelectionToJson(const mmw::Score& score, const std::unordered_set<int>& selection,
	                         int baseTick)
	{
		json data, notes, holds, damages, hiSpeedChanges;
		std::unordered_set<int> selectedNotes;
		std::unordered_set<int> selectedHiSpeedChanges;
		std::unordered_set<int> selectedHolds;
		std::unordered_set<int> selectedDamages;

		for (int id : selection)
		{
			if (score.notes.find(id) == score.notes.end())
				continue;

			const mmw::Note& note = score.notes.at(id);
			switch (note.getType())
			{
			case mmw::NoteType::Tap:
				selectedNotes.insert(note.ID);
				break;
			case mmw::NoteType::Hold:
				selectedHolds.insert(note.ID);
				break;
			case mmw::NoteType::HoldMid:
			case mmw::NoteType::HoldEnd:
				selectedHolds.insert(note.parentID);
				break;

			case mmw::NoteType::Damage:
				selectedDamages.insert(note.ID);
				break;
			default:
				break;
			}
		}

		for (int id : selectedNotes)
		{
			const mmw::Note& note = score.notes.at(id);
			json data = noteToJson(note);
			data["tick"] = note.tick - baseTick;

			notes.push_back(data);
		}
		for (int id : selectedDamages)
		{
			const mmw::Note& note = score.notes.at(id);
			json data = noteToJson(note);
			data["tick"] = note.tick - baseTick;

			damages.push_back(data);
		}
		for (int id : selectedHiSpeedChanges)
		{
			const mmw::HiSpeedChange& note = score.hiSpeedChanges.at(id);
			data["tick"] = note.tick - baseTick;
			data["speed"] = note.tick;

			hiSpeedChanges.push_back(data);
		}

		for (int id : selectedHolds)
		{
			const mmw::HoldNote& hold = score.holdNotes.at(id);
			const mmw::Note& start = score.notes.at(hold.start.ID);
			const mmw::Note& end = score.notes.at(hold.end);

			json holdData, stepsArray;

			json holdStart = noteToJson(start);
			holdStart["tick"] = start.tick - baseTick;
			holdStart["ease"] = mmw::easeTypes[(int)hold.start.ease];
			holdStart["type"] = mmw::holdTypes[(int)hold.startType];

			for (auto& step : hold.steps)
			{
				const mmw::Note& mid = score.notes.at(step.ID);
				json stepData = noteToJson(mid);
				stepData["tick"] = mid.tick - baseTick;
				stepData["type"] = mmw::stepTypes[(int)step.type];
				stepData["ease"] = mmw::easeTypes[(int)step.ease];

				stepsArray.push_back(stepData);
			}

			json holdEnd = noteToJson(end);
			holdEnd["tick"] = end.tick - baseTick;
			holdEnd["type"] = mmw::holdTypes[(int)hold.endType];

			holdData["start"] = holdStart;
			holdData["steps"] = stepsArray;
			holdData["end"] = holdEnd;
			holdData["fade"] = mmw::fadeTypes[(int)hold.fadeType];
			holdData["guide"] = mmw::guideColors[(int)hold.guideColor];
			holds.push_back(holdData);
		}

		data["notes"] = notes;
		data["holds"] = holds;
		data["damages"] = damages;
		data["hi_speed_changes"] = hiSpeedChanges;
		return data;
	}
}
