#pragma once
#include "Result.h"
#include <string>
#include <json.hpp>

namespace MikuMikuWorld
{
	class NotesPreset
	{
	private:
		int ID;
		std::string filename;

	public:
		NotesPreset(int id, std::string name);
		NotesPreset();

		std::string name;
		std::string description;

		nlohmann::json data;

		inline std::string getName() const { return name; };
		inline std::string getFilename() const { return filename; }
		inline int getID() const { return ID; };

		Result read(const std::string& filepath);
		void write(std::string filepath, bool overwrite);
	};
}