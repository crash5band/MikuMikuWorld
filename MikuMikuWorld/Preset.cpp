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

		if (data.find("tick") != data.end())
			note.tick = data["tick"];

		if (data.find("lane") != data.end())
			note.lane = data["lane"];
		
		if (data.find("width") != data.end())
			note.width = data["width"];
		else
			note.width = 3;

		if (note.getType() != NoteType::HoldMid && data.find("critical") != data.end())
			note.critical = data["critical"];
		
		if (!note.hasEase())
			note.flick = getNoteFlick(data.find("flick") != data.end() ? data["flick"] : "");

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

	Result NotesPreset::read(const json& data, const std::string& filepath)
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
				if (holdData.find("start") == holdData.end() || holdData.find("end") == holdData.end())
				{
					return Result(
						ResultStatus::Warning,
						formatString("A hold note in the preset %s is missing its start or end. Skipping hold...", filepath.c_str())
					);
				}

				HoldNote hold;
				Note start = readNote(holdData["start"], NoteType::Hold);
				start.ID = id++;
				notes[start.ID] = start;

				std::string startEase = holdData["start"].find("ease") != holdData["start"].end() ? holdData["start"]["ease"] : "";

				hold.start = HoldStep{ start.ID, HoldStepType::Visible, getNoteEase(startEase) };
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

						std::string stepType = stepData.find("type") != stepData.end() ? stepData["type"] : "";
						std::string stepEase = stepData.find("ease") != stepData.end() ? stepData["ease"] : "";

						hold.steps.push_back(HoldStep{ mid.ID, getStepType(stepType), getNoteEase(stepEase) });
					}

					std::stable_sort(hold.steps.begin(), hold.steps.end(),
						[this](const HoldStep& s1, const HoldStep& s2)
						{
							const Note& n1 = notes.at(s1.ID);
							const Note& n2 = notes.at(s2.ID);
							return n1.tick == n2.tick ? n1.lane < n2.lane : n1.tick < n2.tick;
						});
				}

				Note end = readNote(holdData["end"], NoteType::HoldEnd);
				end.ID = id++;
				end.parentID = start.ID;
				notes[end.ID] = end;

				hold.end = end.ID;
				holds[start.ID] = hold;
			}
		}

		return Result::Ok();
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