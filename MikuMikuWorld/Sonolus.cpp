#include "Sonolus.h"

namespace Sonolus
{
	void to_json(nlohmann::json& json, const LevelDataEntity& levelData)
	{
		if (!levelData.name.empty())
			json["name"] = levelData.name;
		json["archetype"] = levelData.archetype;
		nlohmann::json& dataJson = json["data"] = nlohmann::json::array();
		for (const auto& [name, value] : levelData.data)
		{
			nlohmann::json item;
			item["name"] = name;
			switch (levelData.getValueDataType(value))
			{
			case LevelDataEntity::DataValueType::Ref:
				item["ref"] = LevelDataEntity::getValue<LevelDataEntity::RefType>(value);
				break;
			case LevelDataEntity::DataValueType::Real:
				item["value"] = LevelDataEntity::getValue<LevelDataEntity::RealType>(value);
				break;
			case LevelDataEntity::DataValueType::Integer:
				item["value"] = LevelDataEntity::getValue<LevelDataEntity::IntegerType>(value);
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

	void from_json(const nlohmann::json& json, LevelDataEntity& levelData)
	{
		jsonIO::optional_get_to(json, "name", levelData.name);
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
					throw std::runtime_error("Bad archetype data! Value is not a number!");
			}
		}
	}

	void from_json(const nlohmann::json& json, LevelData& levelData)
	{
		json.at("bgmOffset").get_to(levelData.bgmOffset);
		json.at("entities").get_to(levelData.entities);
	}
}