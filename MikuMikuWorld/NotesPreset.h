#pragma once
#include "ScoreContext.h"
#include "IO.h"
#include <atomic>
#include <filesystem>
#include <future>

namespace MikuMikuWorld
{
	class NotesPreset
	{
	private:
		int ID{};
		std::string filename{};

	public:
		NotesPreset(int id, std::string name);
		NotesPreset();

		std::string name{};
		std::string description{};

		nlohmann::json data{};

		inline std::string getName() const { return name; }
		inline std::string getFilename() const { return filename; }
		inline int getID() const { return ID; }

		Result read(const std::string& filepath);
		void write(std::filesystem::path filePath, bool overwrite);
	};

	class PresetManager
	{
	private:
		std::filesystem::path presetsPath;
		std::atomic<int> nextPresetID{1};
		std::future<bool> createPresetFuture{};
		std::future<bool> deletePresetFuture{};

		IO::MessageBoxResult showErrorMessage(const std::string& message);

	public:
		PresetManager(const std::string& path);
		
		std::vector<NotesPreset> presets;
		NotesPreset deletedPreset{};
		size_t deletedPresetIndex{};

		inline const std::filesystem::path getPresetsPath() const { return presetsPath; }
		
		void loadPresets();
		Result importPreset(const std::string& path);
		bool savePreset(NotesPreset preset);

		void createPreset(const ScoreContext& context, const std::string& name, const std::string& desc);	
		void removePreset(int index);
		bool undoDeletePreset();

		// Replaces illegal filesystem characters with '_'
		std::string fixFilename(const std::string& name);

		void applyPreset(int index, ScoreContext& context);
		void applyPreset(const NotesPreset& preset, ScoreContext& context);
	};
}
