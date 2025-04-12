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
		
		std::unordered_map<int, NotesPreset> presets;
		NotesPreset deletedPreset{};

		inline const std::wstring_view getPresetsPath() const { return presetsPath.c_str(); }
		
		void loadPresets();
		Result importPreset(const std::string& path);
		bool savePreset(NotesPreset preset);

		void createPreset(const ScoreContext& context, const std::string& name, const std::string& desc);	
		void removePreset(int id);
		bool undoDeletePreset();

		// Replaces illegal filesystem characters with '_'
		std::string fixFilename(const std::string& name);

		void applyPreset(int presetId, ScoreContext& context);
	};
}
