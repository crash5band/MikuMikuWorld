#pragma once
#include "ScoreSerializer.h"
#include <string>
#include "Sonolus.h"
#include "JsonIO.h"
#include "Constants.h"

namespace MikuMikuWorld
{
    class SonolusSerializer : public ScoreSerializer
    {
    public:
        void serialize(const Score& score, std::string filename) override;
        Score deserialize(std::string filename) override;

		Sonolus::LevelData serialize(const Score& score);
		Score deserialize(const Sonolus::LevelData& levelData);

		SonolusSerializer(bool compressData = true) : compressData(compressData) { }

		static float ticksToBeats(int ticks, int beatTicks = TICKS_PER_BEAT);
		static float widthToSize(int width);
		static float toSonolusLane(int lane, int width);
		static int toSonolusDirection(FlickType flick);
		static int toSonolusEase(EaseType ease);

		static int beatsToTicks(float beats, int beatTicks = TICKS_PER_BEAT);
		static int sizeToWidth(float size);
		static int toNativeLane(float lane, float size);
		static FlickType toNativeFlick(int direction);
		static EaseType toNativeEase(int ease);

		bool compressData;
    };

	struct SonolusSerializingError : std::runtime_error
	{
		SonolusSerializingError(const char* message) : std::runtime_error(message) {}
		SonolusSerializingError(const std::string& message) : std::runtime_error(message) {}
	};
}

namespace Sonolus
{
	template<typename Item_t>
	static void to_json(nlohmann::json& useItemJson, const UseItem<Item_t>& useItem);
	template<ResourceType Res_t>
	static void to_json(nlohmann::json& itemJson, const SRL<Res_t>& item);
	template<typename T>
	static void to_json(nlohmann::json& itemJson, const ItemSection<T>& item);
	template<typename T>
	static void to_json(nlohmann::json& json, const ItemDetail<T>& levelDetail);
	void to_json(nlohmann::json& itemJson, const Tag& item);
	void to_json(nlohmann::json& itemJson, const SkinItem& item);
	void to_json(nlohmann::json& itemJson, const BackgroundItem& item);
	void to_json(nlohmann::json& itemJson, const EffectItem& item);
	void to_json(nlohmann::json& itemJson, const ParticleItem& item);
	void to_json(nlohmann::json& itemJson, const EngineItem& item);
	void to_json(nlohmann::json& itemJson, const LevelItem& item);
	void to_json(nlohmann::json& json, const LevelDataEntity& levelData);
	void to_json(nlohmann::json& json, const LevelData& levelData);

	template<typename Item_t>
	static void from_json(const nlohmann::json& useItemJson, UseItem<Item_t>& useItem);
	template<ResourceType Res_t>
	static void from_json(const nlohmann::json& itemJson, SRL<Res_t>& item);
	template<typename T>
	static void from_json(const nlohmann::json& itemJson, ItemSection<T>& item);
	void from_json(const nlohmann::json& itemJson, Tag& item);
	void from_json(const nlohmann::json& itemJson, SkinItem& item);
	void from_json(const nlohmann::json& itemJson, BackgroundItem& item);
	void from_json(const nlohmann::json& itemJson, EffectItem& item);
	void from_json(const nlohmann::json& itemJson, ParticleItem& item);
	void from_json(const nlohmann::json& itemJson, EngineItem& item);
	void from_json(const nlohmann::json& itemJson, LevelItem& item);
	void from_json(const nlohmann::json& json, LevelDataEntity& levelData);
	void from_json(const nlohmann::json& json, LevelData& levelData);

	template<typename Item_t>
	void to_json(nlohmann::json& useItemJson, const UseItem<Item_t>& useItem)
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
	void to_json(nlohmann::json& itemJson, const SRL<Res_t>& item)
	{
		itemJson = nlohmann::json::object();
		if (!item.hash.empty()) itemJson["hash"] = item.hash;
		if (!item.url.empty()) itemJson["url"] = item.url;
	}

	template<typename T>
	void to_json(nlohmann::json& itemJson, const ItemSection<T>& item)
	{
		itemJson["title"] = item.title;
		itemJson["itemType"] = item.getItemType();
		itemJson["items"] = item.items;
		if (!item.icon.empty()) itemJson["icon"] = item.icon;
		if (!item.description.empty()) itemJson["description"] = item.description;
		if (!item.help.empty()) itemJson["help"] = item.help;
	}

	template<typename T>
	void to_json(nlohmann::json& json, const ItemDetail<T>& levelDetail)
	{
		json["item"] = levelDetail.item;
		if (!levelDetail.description.empty()) json["description"] = levelDetail.description;
		// Not Implemented
		json["actions"] = nlohmann::json::array();
		json["hasCommunity"] = levelDetail.hasCommunity;
		json["leaderboards"] = nlohmann::json::array();
		json["sections"] = nlohmann::json::array();
	}

	template<typename Item_t>
	void from_json(const nlohmann::json& useItemJson, UseItem<Item_t>& useItem)
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
	void from_json(const nlohmann::json& itemJson, SRL<Res_t>& item)
	{
		jsonIO::optional_get_to(itemJson, "hash", item.hash);
		jsonIO::optional_get_to(itemJson, "url", item.url);
	}

	template<typename T>
	void from_json(const nlohmann::json& itemJson, ItemSection<T>& item)
	{
		if (itemJson.at("itemType").get<std::string>() != item.getItemType())
			throw mmw::SonolusSerializingError("itemType mismatch!");
		itemJson.at("title").get_to(item.title);
		itemJson.at("items").get_to(item.items);
		jsonIO::optional_get_to(itemJson, "icon", item.icon);
		jsonIO::optional_get_to(itemJson, "description", item.description);
		jsonIO::optional_get_to(itemJson, "help", item.help);
	}
}