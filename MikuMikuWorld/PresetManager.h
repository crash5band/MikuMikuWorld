#pragma once
#include "Preset.h"
#include "ScoreContext.h"
#include <unordered_set>
#include <atomic>

namespace MikuMikuWorld
{
	enum class UpdateMode
	{
		Create,
		Delete
	};

	class PresetManager
	{
	private:
		std::atomic<int> nextPresetID;
		std::unordered_set<int> createPresets;
		std::unordered_set<std::string> deletePresets;

	public:
		std::unordered_map<int, NotesPreset> presets;

		/// <summary>
		/// Loads note presets from the specified directory
		/// </summary>
		/// <param name="path">The directory to load presets from</param>
		void loadPresets(const std::string& path);

		/// <summary>
		/// Saves note presets into the specified directory
		/// </summary>
		/// <param name="path">The directory to save presets to</param>
		void savePresets(const std::string& path);

		/// <summary>
		/// Creates a note preset from the selected notes
		/// </summary>
		/// <param name="score">The score containing the notes</param>
		/// <param name="selectedNotes">The IDs of selected notes</param>
		/// <param name="name">The name of the preset to create</param>
		/// <param name="desc">The description of the preset to create</param>
		void createPreset(const Score& score, const std::unordered_set<int>& selectedNotes,
			const std::string& name, const std::string& desc);

		/// <summary>
		/// Removes a preset by ID. if the ID does not exist nothing is removed.
		/// </summary>
		/// <param name="id">The ID of the preset to remove.</param>
		void removePreset(int id);

		/// <summary>
		/// Fixes filenames to work on filesystems
		/// </summary>
		/// <param name="name">The name of the file to fix</param>
		/// <returns></returns>
		std::string fixFilename(const std::string& name);

		void applyPreset(int presetId, ScoreContext& context);
	};
}