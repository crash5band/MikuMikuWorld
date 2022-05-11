#pragma once
#include "Preset.h"
#include "Score.h"
#include "json.hpp"
#include <unordered_set>

namespace MikuMikuWorld
{
	constexpr const char* invalidFilenameChars = "\\/\"|<>?*:";

	enum class UpdateMode
	{
		Create,
		Delete
	};

	class PresetManager
	{
	private:
		int nextPresetID;
		std::unordered_map<int, NotesPreset> presets;
		std::unordered_set<int> createPresets;
		std::unordered_set<std::string> deletePresets;

		void normalizeTicks(NotesPreset& preset);
		void readPreset(const std::string& filename);
		void writePreset(NotesPreset& preset, const std::string& path, bool overwrite);

	public:
		void loadPresets(const std::string& path);
		void savePresets(const std::string& path);
		void createPreset(const Score& score, std::unordered_set<int> selectedNotes,
			const std::string& name, const std::string& desc);

		void removePreset(int id);
		std::string fixFilename(const std::string& name);

		const std::unordered_map<int, NotesPreset>& getPresets() const;
	};
}