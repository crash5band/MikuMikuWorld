#pragma once
#include <filesystem>
#include "Sonolus.h"
#include "JsonIO.h"
#include "Archive.h"

namespace MikuMikuWorld
{
	class CollectionSerializer
	{
	public:
		void serialize(const Score& score, const std::string& levelName, const std::string& newLevelName);
		Score deserialize(const std::string& levelName);

		virtual void serializeJson(const nlohmann::json& j, const std::string& entryPath) = 0;
		virtual nlohmann::json deserializeJson(const std::string& entryPath) = 0;
		virtual std::pair<std::string, std::string> packageData(const std::vector<uint8_t>& buffer) = 0;
		virtual std::pair<std::string, std::string> packageFile(const std::string& filePath) = 0;
		virtual std::vector<uint8_t> unpackData(const std::string& entryPath) = 0;
		virtual std::string unpackFile(const std::string& entryPath, const std::string_view& outputExtension) = 0;
		virtual void removeFile(const std::string& entryPath) = 0;
		virtual bool exists(const std::string& entryPath) = 0;
		virtual ~CollectionSerializer() { }

		void createCollection();
		Sonolus::EngineItem generateEngineItem();

		static std::string urlToEntry(const std::string& url);
		static std::string entryToUrl(const std::string& entry);
		static void updateLevelInfo(nlohmann::json& infoJson, const Sonolus::LevelItem& item, const Sonolus::LevelItem& oldItem);
		static void updateLevelList(nlohmann::json& listJson, const Sonolus::LevelItem& item, const Sonolus::LevelItem& oldItem);
	public:
		std::string engineName = "pjsekai";
		bool compressLevelData = true;
		bool removeOldFiles = false;
	};

	class SonolusCollectionSerializer : public CollectionSerializer
	{
	public:
		SonolusCollectionSerializer(const std::string& rootPath);
		
		void serializeJson(const nlohmann::json& j, const std::string& entryPath) override;
		nlohmann::json deserializeJson(const std::string& entryPath) override;
		std::pair<std::string, std::string> packageData(const std::vector<uint8_t>& buffer) override;
		std::pair<std::string, std::string> packageFile(const std::string& filePath) override;
		std::vector<uint8_t> unpackData(const std::string& entryPath) override;
		std::string unpackFile(const std::string& entryPath, const std::string_view& outputExtension) override;
		void removeFile(const std::string& entryPath) override;
		bool exists(const std::string& entryPath) override;
	private:
		std::filesystem::path rootPath;
	};

	class SonolusArchiveCollectionSerializer : public CollectionSerializer
	{
	public:
		SonolusArchiveCollectionSerializer(const std::string& archiveFilename);
		
		void serializeJson(const nlohmann::json& j, const std::string& entryPath) override;
		nlohmann::json deserializeJson(const std::string& entryPath) override;
		std::pair<std::string, std::string> packageData(const std::vector<uint8_t>& buffer) override;
		std::pair<std::string, std::string> packageFile(const std::string& filePath) override;
		std::vector<uint8_t> unpackData(const std::string& entryPath) override;
		std::string unpackFile(const std::string& entryPath, const std::string_view& outputExtension) override;
		void removeFile(const std::string& entryPath) override;
		bool exists(const std::string& entryPath) override;
	private:
		IO::Archive scpArchive;
	};

	struct SonolusLevelMetadata
	{
		enum Flag {
			None = 0,
			EngineNotRegconized = 1,
			LevelDataMissing = 1 << 1
		};
		std::string name;
		std::string title;
		Flag flags;
	};

	class SonolusSerializeDialog : public ScoreSerializeDialog
	{
		bool updateFileSelector(SerializeDialogResult& result);
	public:
		SonolusSerializeDialog(Score score, const std::string& filename);
		SonolusSerializeDialog(const std::string& filename);
		SerializeDialogResult update() override;
	private:
		bool isSerializing, open = true;
		std::unique_ptr<CollectionSerializer> serializer;
		
		intptr_t selectedItem = -1;
		std::string insertLevelName;
		std::vector<SonolusLevelMetadata> levelInfos;
	};
}
