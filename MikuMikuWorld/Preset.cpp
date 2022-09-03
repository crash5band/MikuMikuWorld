#include "Preset.h"
#include "StringOperations.h"
#include "Application.h"
#include <fstream>

using namespace nlohmann;

namespace MikuMikuWorld
{
	NotesPreset::NotesPreset(int _id, std::string _name) :
		ID{ _id }, name{ _name }
	{

	}

	NotesPreset::NotesPreset() : ID{ -1 }, name{""}
	{

	}

	FlickType NotesPreset::getNoteFlick(const std::string& data)
	{
		std::string flick = data;
		std::transform(flick.begin(), flick.end(), flick.begin(), ::tolower);

		for (int i = 0; i < 4; ++i)
			if (flickTypes[i] == data)
				return (FlickType)i;

		return FlickType::None;
	}

	EaseType NotesPreset::getNoteEase(const std::string& data)
	{
		std::string ease = data;
		std::transform(ease.begin(), ease.end(), ease.begin(), ::tolower);

		for (int i = 0; i < 3; ++i)
			if (easeTypes[i] == data)
				return (EaseType)i;

		return EaseType::None;
	}

	HoldStepType NotesPreset::getStepType(const std::string& data)
	{
		std::string type = data;
		std::transform(type.begin(), type.end(), type.begin(), ::tolower);
		for (int i = 0; i < 3; ++i)
			if (stepTypes[i] == data)
				return (HoldStepType)i;

		return HoldStepType::Invisible;
	}

	Note NotesPreset::readNote(const json& data, NoteType type)
	{
		Note note(type);
		note.tick = data["tick"];
		note.lane = data["lane"];
		note.width = data["width"];

		if (note.getType() != NoteType::HoldMid)
			note.critical = data["critical"];
		
		if (!note.hasEase())
			note.flick = getNoteFlick(data["flick"]);

		return note;
	}

	void NotesPreset::writeNote(json& data, const Note& note) const
	{
		data["tick"] = note.tick;
		data["lane"] = note.lane;
		data["width"] = note.width;

		if (note.getType() != NoteType::HoldMid)
			data["critical"] = note.critical;

		if (!note.hasEase())
			data["flick"] = flickTypes[(int)note.flick];
	}

	void NotesPreset::read(const json& data, const std::string& filepath)
	{
		filename = File::getFilenameWithoutExtension(filepath);
		if (data.find("name") != data.end())
			name = data["name"];

		if (data.find("description") != data.end())
			description = data["description"];

		int id = 0;
		if (data.find("notes") != data.end())
		{
			notes.reserve(data["notes"].size());
			for (const auto& noteData : data["notes"])
			{
				Note note = readNote(noteData, NoteType::Tap);
				note.ID = id++;
				notes[note.ID] = note;
			}
		}

		if (data.find("holds") != data.end())
		{
			holds.reserve(data["holds"].size());
			for (const auto& holdData : data["holds"])
			{
				HoldNote hold;
				Note start = readNote(holdData["start"], NoteType::Hold);
				start.ID = id++;
				notes[start.ID] = start;

				hold.start = HoldStep{ start.ID, HoldStepType::Visible, getNoteEase(holdData["start"]["ease"]) };
				if (holdData.find("steps") != holdData.end())
				{
					hold.steps.reserve(holdData["steps"].size());
					for (const auto& stepData : holdData["steps"])
					{
						Note mid = readNote(stepData, NoteType::HoldMid);
						mid.ID = id++;
						mid.parentID = start.ID;
						mid.critical = start.critical;
						notes[mid.ID] = mid;

						hold.steps.push_back(HoldStep{ mid.ID, getStepType(stepData["type"]), getNoteEase(stepData["ease"]) });
					}
				}

				Note end = readNote(holdData["end"], NoteType::HoldEnd);
				end.ID = id++;
				end.parentID = start.ID;
				notes[end.ID] = end;

				hold.end = end.ID;
				holds[start.ID] = hold;
			}
		}
	}

	json NotesPreset::write()
	{
		json notesArray;
		for (const auto& [id, note] : notes)
		{
			if (note.getType() == NoteType::Tap)
			{
				json noteJson;
				writeNote(noteJson, note);

				notesArray.push_back(noteJson);
			}
		}

		json holdsArray;
		for (const auto& [id, hold] : holds)
		{
			json holdJson;
			json startJson;
			writeNote(startJson, notes.at(hold.start.ID));
			startJson["ease"] = easeTypes[(int)hold.start.ease];

			holdJson["start"] = startJson;

			json stepsArray;
			for (const auto& step : hold.steps)
			{
				json stepJson;
				writeNote(stepJson, notes.at(step.ID));
				stepJson["ease"] = easeTypes[(int)step.ease];
				stepJson["type"] = stepTypes[(int)step.type];

				stepsArray.push_back(stepJson);
			}
			holdJson["steps"] = stepsArray;

			json endJson;
			writeNote(endJson, notes.at(hold.end));
			holdJson["end"] = endJson;

			holdsArray.push_back(holdJson);
		}

		json presetJson;
		presetJson["name"] = name;
		presetJson["description"] = description;
		presetJson["notes"] = notesArray;
		presetJson["holds"] = holdsArray;

		return presetJson;
	}
}