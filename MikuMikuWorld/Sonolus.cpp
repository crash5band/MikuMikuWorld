#include "Sonolus.h"
#include "Archive.h"
#include "Application.h"
#include "SHA-1.hpp"

namespace Sonolus
{
	int skinVersion = 4;
	int backgroundVersion = 2;
	int effectVersion = 5;
	int particleVersion = 3;
	int engineVersion = 13;
	int levelVersion = 1;
	
	template<typename Item_t>
	static void to_json(nlohmann::json& useItemJson, const UseItem<Item_t>& useItem)
	{
		if (useItem.useDefault())
		{
			useItemJson["useDefault"] = true;
		}
		else
		{
			useItemJson["useDefault"] = false;
			useItemJson["item"] = useItem.getItem();
		}
	}

	template<ResourceType Res_t>
	static void to_json(nlohmann::json& itemJson, const SRL<Res_t>& item)
	{
		itemJson = nlohmann::json::object();
		if (!item.hash.empty()) itemJson["hash"] = item.hash;
		if (!item.url.empty()) itemJson["url"] = item.url;
	}

	static void to_json(nlohmann::json& itemJson, const Tag& item)
	{
		itemJson["title"] = item.title;
		if (!item.icon.empty()) itemJson["icon"] = item.icon;
	}

	static void to_json(nlohmann::json& itemJson, const SkinItem& item)
	{
		itemJson["name"] = item.name;
		itemJson["version"] = item.version;
		itemJson["title"] = item.title;
		itemJson["subtitle"] = item.subtitle;
		itemJson["author"] = item.author;
		itemJson["thumbnail"] = item.thumbnail;
		itemJson["data"] = item.data;
		itemJson["texture"] = item.texture;
		itemJson["tags"] = item.tags;
		if (!item.source.empty()) itemJson["source"] = item.source;
	}

	static void to_json(nlohmann::json& itemJson, const BackgroundItem& item)
	{
		itemJson["name"] = item.name;
		itemJson["version"] = item.version;
		itemJson["title"] = item.title;
		itemJson["subtitle"] = item.subtitle;
		itemJson["author"] = item.author;
		itemJson["thumbnail"] = item.thumbnail;
		itemJson["data"] = item.data;
		itemJson["image"] = item.image;
		itemJson["configuration"] = item.configuration;
		itemJson["tags"] = item.tags;
		if (!item.source.empty()) itemJson["source"] = item.source;
	}

	static void to_json(nlohmann::json& itemJson, const EffectItem& item)
	{
		itemJson["name"] = item.name;
		itemJson["version"] = item.version;
		itemJson["title"] = item.title;
		itemJson["subtitle"] = item.subtitle;
		itemJson["author"] = item.author;
		itemJson["thumbnail"] = item.thumbnail;
		itemJson["data"] = item.data;
		itemJson["audio"] = item.audio;
		itemJson["tags"] = item.tags;
		if (!item.source.empty()) itemJson["source"] = item.source;
	}

	static void to_json(nlohmann::json& itemJson, const ParticleItem& item)
	{
		itemJson["name"] = item.name;
		itemJson["version"] = item.version;
		itemJson["title"] = item.title;
		itemJson["subtitle"] = item.subtitle;
		itemJson["author"] = item.author;
		itemJson["thumbnail"] = item.thumbnail;
		itemJson["data"] = item.data;
		itemJson["texture"] = item.texture;
		itemJson["tags"] = item.tags;
		if (!item.source.empty()) itemJson["source"] = item.source;
	}

	static void to_json(nlohmann::json& itemJson, const EngineItem& item)
	{
		itemJson["name"] = item.name;
		itemJson["version"] = item.version;
		itemJson["title"] = item.title;
		itemJson["subtitle"] = item.subtitle;
		itemJson["author"] = item.author;
		itemJson["skin"] = item.skin;
		itemJson["background"] = item.background;
		itemJson["effect"] = item.effect;
		itemJson["particle"] = item.particle;
		itemJson["thumbnail"] = item.thumbnail;
		itemJson["playData"] = item.playData;
		itemJson["watchData"] = item.watchData;
		itemJson["previewData"] = item.previewData;
		itemJson["tutorialData"] = item.tutorialData;
		itemJson["configuration"] = item.configuration;
		itemJson["tags"] = item.tags;
		if (!item.source.empty()) itemJson["source"] = item.source;
		if (!item.rom.url.empty()) itemJson["rom"] = item.source;
	}

	static void to_json(nlohmann::json& itemJson, const LevelItem& item)
	{
		itemJson["name"] = item.name;
		itemJson["version"] = item.version;
		itemJson["rating"] = item.rating;
		itemJson["title"] = item.title;
		itemJson["artists"] = item.artists;
		itemJson["author"] = item.author;
		itemJson["engine"] = item.engine;
		itemJson["useSkin"] = item.useSkin;
		itemJson["useBackground"] = item.useBackground;
		itemJson["useEffect"] = item.useEffect;
		itemJson["useParticle"] = item.useParticle;
		itemJson["cover"] = item.cover;
		itemJson["bgm"] = item.bgm;
		itemJson["data"] = item.data;
		itemJson["tags"] = item.tags;
		if (!item.source.empty()) itemJson["source"] = item.source;
		if (!item.preview.url.empty()) itemJson["preview"] = item.preview;
	}

	template<typename T>
	static void to_json(nlohmann::json& itemJson, const ItemSection<T>& item)
	{
		itemJson["title"] = item.title;
		itemJson["itemType"] = item.getItemType();
		itemJson["items"] = item.items;
		if (!item.icon.empty()) itemJson["icon"] = item.icon;
		if (!item.description.empty()) itemJson["description"] = item.description;
		if (!item.help.empty()) itemJson["help"] = item.help;
	}

	static void to_json(nlohmann::json& json, const LevelDataEntity& levelData)
	{
		if (!levelData.name.empty())
			json["name"] = levelData.name;
		json["archetype"] = levelData.archetype;
		nlohmann::json& dataJson = json["data"] = nlohmann::json::array();
		for (const auto& [name, value] : levelData.data)
		{
			nlohmann::json item;
			item["name"] = name;
			switch (levelData.getDataValueType(value))
			{
			case LevelDataEntity::DataValueType::Ref:
				item["ref"] = LevelDataEntity::getDataValue<LevelDataEntity::RefType>(value);
				break;
			case LevelDataEntity::DataValueType::Real:
				item["value"] = LevelDataEntity::getDataValue<LevelDataEntity::RealType>(value);
				break;
			case LevelDataEntity::DataValueType::Integer:
				item["value"] = LevelDataEntity::getDataValue<LevelDataEntity::IntegerType>(value);
				break;
			}
			dataJson.push_back(item);
		}
	}

	void to_json(nlohmann::json& json, const LevelData& levelData)
	{
		json["bgmOffset"] = levelData.bgmOffset;
		json["entities"] = levelData.entities;
	}

	void serializeLevelDetails(IO::Archive& scpArchive, const ItemDetail<LevelItem>& levelDetail)
	{
		IO::ArchiveFile itemFile = scpArchive.openFile("sonolus/levels/" + levelDetail.item.name);
		if (!itemFile.isOpen())
			throw Sonolus::SonolusSerializingError(IO::formatString("Failed to open the level file.\nReason: %s", scpArchive.getLastError().getStr()));
		nlohmann::json json;
		json["item"] = levelDetail.item;
		if (!levelDetail.description.empty()) json["description"] = levelDetail.description;
		json["actions"] = nlohmann::json::array();
		json["hasCommunity"] = levelDetail.hasCommunity;
		json["leaderboards"] = nlohmann::json::array();
		json["sections"] = nlohmann::json::array();
		std::string data = json.dump();
		itemFile.writeAllText(data);
		if (itemFile.getLastError().get() != 0)
			throw Sonolus::SonolusSerializingError(scpArchive.getLastError().getStr());
	}

	void serializeLevelInfo(IO::Archive& scpArchive, const ItemInfo<LevelItem>& levelInfo)
	{
		IO::ArchiveFile itemFile = scpArchive.openFile("sonolus/levels/info");
		if (!itemFile.isOpen())
			throw Sonolus::SonolusSerializingError(IO::formatString("Failed to open the level info file.\nReason: %s", scpArchive.getLastError().getStr()));
		nlohmann::json json;
		json["sections"] = levelInfo.sections;
		std::string data = json.dump();
		itemFile.writeAllText(data);
		if (itemFile.getLastError().get() != 0)
			throw Sonolus::SonolusSerializingError(scpArchive.getLastError().getStr());
	}

	void serializeLevelList(IO::Archive& scpArchive, const ItemList<LevelItem>& levelList)
	{
		IO::ArchiveFile itemFile = scpArchive.openFile("sonolus/levels/list");
		if (!itemFile.isOpen())
			throw Sonolus::SonolusSerializingError(IO::formatString("Failed to open the level list file.\nReason: %s", scpArchive.getLastError().getStr()));
		nlohmann::json json;
		json["pageCount"] = levelList.pageCount;
		json["items"] = levelList.items;
		std::string data = json.dump();
		itemFile.writeAllText(data);
		if (itemFile.getLastError().get() != 0)
			throw Sonolus::SonolusSerializingError(scpArchive.getLastError().getStr());
	}

	template<typename T>
	static auto optional_get_to(const nlohmann::json& j, const std::string& k, T& v)
	-> typename decltype(j.get_to(v))
	{
		if (j.contains(k))
			return j[k].get_to(v);
		return v;
	}
	
	template<typename Item_t>
	static void from_json(const nlohmann::json& useItemJson, UseItem<Item_t>& useItem)
	{
		bool useDefault = useItemJson.at("useDefault").get<bool>();

		if (!useDefault)
		{
			Item_t item{};
			from_json(useItemJson.at("item"), item);
			useItem.setItem(std::move(item));
		}
	}

	template<ResourceType Res_t>
	static void from_json(const nlohmann::json& itemJson, SRL<Res_t>& item)
	{
		optional_get_to(itemJson, "hash", item.hash);
		optional_get_to(itemJson, "url", item.url);
	}

	static void from_json(const nlohmann::json& itemJson, Tag& item)
	{
		itemJson.at("title").get_to(item.title);
		optional_get_to(itemJson, "icon", item.icon);
	}

	static void from_json(const nlohmann::json& itemJson, SkinItem& item)
	{
		itemJson.at("name").get_to(item.name);
		itemJson.at("version").get_to(item.version);
		itemJson.at("title").get_to(item.title);
		itemJson.at("subtitle").get_to(item.subtitle);
		itemJson.at("author").get_to(item.author);
		itemJson.at("thumbnail").get_to(item.thumbnail);
		itemJson.at("data").get_to(item.data);
		itemJson.at("texture").get_to(item.texture);
		itemJson.at("tags").get_to(item.tags);
		optional_get_to(itemJson, "source", item.source);
	}

	static void from_json(const nlohmann::json& itemJson, BackgroundItem& item)
	{
		itemJson.at("name").get_to(item.name);
		itemJson.at("version").get_to(item.version);
		itemJson.at("title").get_to(item.title);
		itemJson.at("subtitle").get_to(item.subtitle);
		itemJson.at("author").get_to(item.author);
		itemJson.at("thumbnail").get_to(item.thumbnail);
		itemJson.at("data").get_to(item.data);
		itemJson.at("image").get_to(item.image);
		itemJson.at("configuration").get_to(item.configuration);
		itemJson.at("tags").get_to(item.tags);
		optional_get_to(itemJson, "source", item.source);
	}

	static void from_json(const nlohmann::json& itemJson, EffectItem& item)
	{
		itemJson.at("name").get_to(item.name);
		itemJson.at("version").get_to(item.version);
		itemJson.at("title").get_to(item.title);
		itemJson.at("subtitle").get_to(item.subtitle);
		itemJson.at("author").get_to(item.author);
		itemJson.at("thumbnail").get_to(item.thumbnail);
		itemJson.at("data").get_to(item.data);
		itemJson.at("audio").get_to(item.audio);
		itemJson.at("tags").get_to(item.tags);
		optional_get_to(itemJson, "source", item.source);
	}

	static void from_json(const nlohmann::json& itemJson, ParticleItem& item)
	{
		itemJson.at("name").get_to(item.name);
		itemJson.at("version").get_to(item.version);
		itemJson.at("title").get_to(item.title);
		itemJson.at("subtitle").get_to(item.subtitle);
		itemJson.at("author").get_to(item.author);
		itemJson.at("thumbnail").get_to(item.thumbnail);
		itemJson.at("data").get_to(item.data);
		itemJson.at("texture").get_to(item.texture);
		itemJson.at("tags").get_to(item.tags);
		optional_get_to(itemJson, "source", item.source);
	}

	static void from_json(const nlohmann::json& itemJson, EngineItem& item)
	{
		itemJson.at("name").get_to(item.name);
		itemJson.at("version").get_to(item.version);
		itemJson.at("title").get_to(item.title);
		itemJson.at("subtitle").get_to(item.subtitle);
		itemJson.at("author").get_to(item.author);
		itemJson.at("skin").get_to(item.skin);
		itemJson.at("background").get_to(item.background);
		itemJson.at("effect").get_to(item.effect);
		itemJson.at("particle").get_to(item.particle);
		itemJson.at("thumbnail").get_to(item.thumbnail);
		itemJson.at("playData").get_to(item.playData);
		itemJson.at("watchData").get_to(item.watchData);
		itemJson.at("previewData").get_to(item.previewData);
		itemJson.at("tutorialData").get_to(item.tutorialData);
		itemJson.at("configuration").get_to(item.configuration);
		itemJson.at("tags").get_to(item.tags);
		optional_get_to(itemJson, "source", item.source);
		optional_get_to(itemJson, "rom", item.rom);
	}

	static void from_json(const nlohmann::json& itemJson, LevelItem& item)
	{
        itemJson.at("name").get_to(item.name);
		itemJson.at("version").get_to(item.version);
		itemJson.at("rating").get_to(item.rating);
		itemJson.at("title").get_to(item.title);
		itemJson.at("artists").get_to(item.artists);
		itemJson.at("author").get_to(item.author);
		itemJson.at("engine").get_to(item.engine);
		itemJson.at("useSkin").get_to(item.useSkin);
		itemJson.at("useBackground").get_to(item.useBackground);
		itemJson.at("useEffect").get_to(item.useEffect);
		itemJson.at("useParticle").get_to(item.useParticle);
		itemJson.at("cover").get_to(item.cover);
		itemJson.at("bgm").get_to(item.bgm);
		itemJson.at("data").get_to(item.data);
		itemJson.at("tags").get_to(item.tags);
		optional_get_to(itemJson, "source", item.source);
		optional_get_to(itemJson, "preview", item.preview);
    }

	template<typename T>
	static void from_json(const nlohmann::json& itemJson, ItemSection<T>& item)
	{
		if (itemJson.at("itemType").get<std::string>() != item.getItemType())
			throw SonolusSerializingError("itemType mismatch!");
		itemJson.at("title").get_to(item.title);
		itemJson.at("items").get_to(item.items);
		optional_get_to(itemJson, "icon", item.icon);
		optional_get_to(itemJson, "description", item.description);
		optional_get_to(itemJson, "help", item.help);
	}

	static void from_json(const nlohmann::json& json, LevelDataEntity& levelData)
	{
		optional_get_to(json, "name", levelData.name);
		json.at("archetype").get_to(levelData.archetype);
		auto& data = json.at("data");
		for (auto& item : data)
		{
			auto it = item.find("ref");
			if (it != item.end())
			{
				levelData.data.emplace(item.at("name").get<std::string>(), item.at("ref").get<std::string>());
			}
			else
			{
				auto& valueJson = item.at("value");
				if (valueJson.is_number_float())
					levelData.data.emplace(item.at("name").get<std::string>(), valueJson.get<float>());
				else if (valueJson.is_number())
					levelData.data.emplace(item.at("name").get<std::string>(), valueJson.get<int>());
				else
					throw SonolusSerializingError("Bad archetype data!");
			}
		}
	}

	void from_json(const nlohmann::json& json, LevelData& levelData)
	{
		json.at("bgmOffset").get_to(levelData.bgmOffset);
		json.at("entities").get_to(levelData.entities);
	}

	ItemDetail<LevelItem> deserializeLevelDetails(IO::Archive& scpArchive, const std::string levelName)
	{
		IO::ArchiveFile itemFile = scpArchive.openFile("sonolus/levels/" + levelName);
		if (!itemFile.isOpen())
			throw Sonolus::SonolusSerializingError(IO::formatString("Failed to open the level file.\nReason: %s", scpArchive.getLastError().getStr()));
		nlohmann::json json = nlohmann::json::parse(itemFile.readAllText());
		std::string desc;
		optional_get_to(json, "description", desc);
		return {
			json["item"].get<LevelItem>(),
			desc
		};
	}

	ItemInfo<LevelItem> deserializeLevelInfo(const std::string& filename)
	{
		IO::Archive scpArchive(filename, IO::ArchiveOpenMode::Read);
		if (!scpArchive.isOpen())
			throw SonolusSerializingError(scpArchive.getLastError().getStr());
		return deserializeLevelInfo(scpArchive);
	}

	ItemInfo<LevelItem> deserializeLevelInfo(IO::Archive& scpArchive)
	{
		int64_t infoIdx = scpArchive.getEntryIndex("sonolus/levels/info", false);
		if (infoIdx < 0)
		{
			// Check if the archive is a sonolus archive
			int64_t idx = scpArchive.getEntryIndex("sonolus/info", false);
			if (idx < 0) throw SonolusSerializingError("Not a sonolus collection file!");
			// Valid collection so we just assume it's empty
			return {};
		}
		IO::ArchiveFile infoFile = scpArchive.openFile(infoIdx);
		if (!infoFile.isOpen())
			throw SonolusSerializingError(scpArchive.getLastError().getStr());
		std::string data = infoFile.readAllText();
		if (!data.size() && infoFile.getLastError().get() != 0)
			throw SonolusSerializingError(infoFile.getLastError().getStr());
		nlohmann::json infoJson = nlohmann::json::parse(data);
		return { infoJson["sections"].template get<std::vector<ItemSection<LevelItem>>>() };
	}

	ItemList<LevelItem> deserializeLevelList(const std::string& filename)
	{
		IO::Archive scpArchive(filename, IO::ArchiveOpenMode::Read);
		if (!scpArchive.isOpen())
			throw SonolusSerializingError(scpArchive.getLastError().getStr());
		return deserializeLevelList(scpArchive);
	}

	ItemList<LevelItem> deserializeLevelList(IO::Archive& scpArchive)
	{
		int64_t infoIdx = scpArchive.getEntryIndex("sonolus/levels/list", false);
		if (infoIdx < 0)
		{
			// Check if the archive is a sonolus archive
			int64_t idx = scpArchive.getEntryIndex("sonolus/info", false);
			if (idx < 0) throw SonolusSerializingError("Not a sonolus collection file!");
			// Valid collection so we just assume it's empty
			return {};
		}
		IO::ArchiveFile listFile = scpArchive.openFile(infoIdx);
		if (!listFile.isOpen())
			throw SonolusSerializingError(scpArchive.getLastError().getStr());
		std::string data = listFile.readAllText();
		if (!data.size() && listFile.getLastError().get() != 0)
			throw SonolusSerializingError(listFile.getLastError().getStr());
		nlohmann::json listJson = nlohmann::json::parse(data);
		return {
			jsonIO::tryGetValue(listJson, "pageCount", 1),
			listJson["items"].template get<std::vector<LevelItem>>()
		};
	}

	static EngineItem deserializeEngineItem(const std::string& name)
	{
		IO::Archive scpArchive(mmw::Application::getAppDir() + "res\\scp\\" + "Engine.scp", IO::ArchiveOpenMode::Read);
		if (!scpArchive.isOpen())
			throw SonolusSerializingError(scpArchive.getLastError().getStr());
		int64_t engineIdx = scpArchive.getEntryIndex("sonolus/engines/" + name, false);
		if (engineIdx < 0)
			throw SonolusSerializingError(IO::formatString("Can't find the info for engine '%s'", name));
		IO::ArchiveFile engineFile = scpArchive.openFile(engineIdx);
		if (!engineFile.isOpen())
			throw SonolusSerializingError(scpArchive.getLastError().getStr());
		std::string data = engineFile.readAllText();
		if (engineFile.getLastError().get() != 0)
			throw SonolusSerializingError(engineFile.getLastError().getStr());
		nlohmann::json engineDetailsJson = nlohmann::json::parse(data);
		return engineDetailsJson["item"].get<EngineItem>();
	}

	LevelItem Sonolus::generateLevelItem(const mmw::Score& score)
	{
		LevelItem item;
		item.version = levelVersion;
		item.rating = score.metadata.rating;
		item.title = score.metadata.title;
		item.artists = score.metadata.artist;
		item.author = score.metadata.author;
		item.engine = deserializeEngineItem("pjsekai");
		return item;
	}

	std::pair<std::string, std::string> packageData(IO::Archive& scpArchive, const std::vector<uint8_t>& buffer)
	{
		std::string fileHash = hash(buffer);
		std::string arcFilePath = "/sonolus/repository/" + fileHash;
		IO::ArchiveFile arcFile = scpArchive.openFile(arcFilePath);
		if (!arcFile.isOpen())
			return {};
		arcFile.writeAllBytes(buffer);
		if (arcFile.getLastError().get() != 0)
			return {};
		return { fileHash, arcFilePath };
	}

	std::pair<std::string, std::string> packageFile(IO::Archive& scpArchive, const std::string& filepath)
	{
		if (!IO::File::exists(filepath))
			return {};
		IO::File file(filepath, IO::FileMode::ReadBinary);
		return packageData(scpArchive, file.readAllBytes());
	}

	std::string hash(const std::vector<uint8_t>& data)
	{
		IO::SHA1 sha1;
		sha1.update(data);
		return sha1.final();
	}

	bool Sonolus::createSonolusCollection(const std::string& filename)
	{
		IO::Archive archive(filename, IO::ArchiveOpenMode::Create | IO::ArchiveOpenMode::Truncate);
		if (!archive.isOpen())
			return false;
		IO::ArchiveFile serverPackageFile = archive.openFile("sonolus/package");
		if (!serverPackageFile.isOpen())
			return false;
		nlohmann::json serverPackageJson;
		serverPackageJson["shouldUpdate"] = true;
		serverPackageFile.writeAllText(serverPackageJson.dump());
		if (serverPackageFile.getLastError().get() != 0)
			return false;
		serverPackageFile.close();
		IO::ArchiveFile serverInfoFile = archive.openFile("sonolus/info");
		if (!serverInfoFile.isOpen())
			return false;
		nlohmann::json serverInfoJson;
		serverInfoJson["title"] = APP_NAME " Collection";
		/*serverInfoJson["buttons"] = nlohmann::json::array();
		auto& btnsJson = serverInfoJson["buttons"];
		btnsJson.push_back({ { "type", "post" } });
		btnsJson.push_back({ { "type", "playlist" } });
		btnsJson.push_back({ { "type", "level" } });
		btnsJson.push_back({ { "type", "replay" } });
		btnsJson.push_back({ { "type", "skin" } });
		btnsJson.push_back({ { "type", "background" } });
		btnsJson.push_back({ { "type", "effect" } });
		btnsJson.push_back({ { "type", "particle" } });
		btnsJson.push_back({ { "type", "engine" } });
		btnsJson.push_back({ { "type", "configuration" } });*/
		serverInfoJson["buttons"] = {
			{ { "type", "post" } },
			{ { "type", "playlist" } },
			{ { "type", "level" } },
			{ { "type", "replay" } },
			{ { "type", "skin" } },
			{ { "type", "background" } },
			{ { "type", "effect" } },
			{ { "type", "particle" } },
			{ { "type", "engine" } },
			{ { "type", "configuration" } },
		};
		serverInfoJson["configuration"] = { { "options", nlohmann::json::array() } };
		serverInfoFile.writeAllText(serverInfoJson.dump());
		if (serverInfoFile.getLastError().get() != 0)
			return false;
		serverInfoFile.close();
		ItemInfo<LevelItem> levelInfo;
		levelInfo.sections.push_back({ "#NEWEST" });
		ItemList<LevelItem> levelList{ 1 };
		try
		{
			serializeLevelInfo(archive, levelInfo);
			serializeLevelList(archive, levelList);
		}
		catch (const Sonolus::SonolusSerializingError& snlError)
		{
			return false;
		}
		return true;
	}
}