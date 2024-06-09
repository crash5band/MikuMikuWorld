#pragma once
#include "ScoreContext.h"
#include <unordered_set>
#include <atomic>
#include <filesystem>
#include <future>

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

		inline std::string getName() const { return name; }
		inline std::string getFilename() const { return filename; }
		inline int getID() const { return ID; }

		Result read(const std::string& filepath);
		void write(std::string filepath, bool overwrite);
	};

	enum class UpdateMode
	{
		Create, Delete
	};

	class PresetManager
	{
	private:
		std::filesystem::path presetsPath;
		std::atomic<int> nextPresetID{};
		std::future<void> createPresetFuture{};
		std::future<bool> deletePresetFuture{};

	public:
		PresetManager(const std::string& path);
		
		std::unordered_map<int, NotesPreset> presets;

		void loadPresets(const std::string& path);

		Result loadPreset(const std::string& path);
		void savePreset(NotesPreset preset);

		void createPreset(const ScoreContext& context, const std::string& name, const std::string& desc);	
		void removePreset(int id);

		// Replaces illegal filesystem characters with '_'
		std::string fixFilename(const std::string& name);

		void applyPreset(int presetId, ScoreContext& context);
	};
}
