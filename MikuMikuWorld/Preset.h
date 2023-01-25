#pragma once
#include "Note.h"
#include "Result.h"
#include <string>
#include <unordered_map>
#include <json.hpp>

namespace MikuMikuWorld
{
	class NotesPreset
	{
	private:
		int ID;
		std::string filename;

		Note readNote(const nlohmann::json& data, NoteType type);
		FlickType getNoteFlick(const std::string& data);
		EaseType getNoteEase(const std::string& data);
		HoldStepType getStepType(const std::string& data);

		void writeNote(nlohmann::json& data, const Note& note) const;

	public:
		NotesPreset(int id, std::string name);
		NotesPreset();

		std::string name;
		std::string description;

		std::unordered_map<int, Note> notes;
		std::unordered_map<int, HoldNote> holds;

		inline std::string getName() const { return name; };
		inline std::string getFilename() const { return filename; }
		inline int getID() const { return ID; };

		Result read(const nlohmann::json& data, const std::string& filepath);
		nlohmann::json write();
	};
}