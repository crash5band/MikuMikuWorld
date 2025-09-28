#pragma once
#include <string>
#include <vector>
#include <variant>
#include "JsonIO.h"

namespace Sonolus
{
	struct LevelDataEntity
	{
		enum class DataValueType { Integer, Real, Ref };
		using IntegerType = int;
		using RealType = double;
		using RefType = std::string;

		using ValueType = std::variant<IntegerType, RealType, RefType>;
		using MapDataType = std::map<std::string, ValueType>;

		std::string name;
		std::string archetype;
		MapDataType data;
		LevelDataEntity() = default;
		LevelDataEntity(const std::string& archetype) : archetype(archetype) {}
		template<typename StrArg, typename MapArg>
		LevelDataEntity(StrArg&& archetype, MapArg&& data) : archetype(std::forward<StrArg>(archetype)), data(std::forward<MapArg>(data)) {}
		LevelDataEntity(const std::string& archetype, std::initializer_list<MapDataType::value_type> init) : archetype(archetype), data(init) {}
		LevelDataEntity(const std::string& name, const std::string& archetype, const MapDataType& data) : name(name), archetype(archetype), data(data) {}

		template<typename T>
		inline T& getDataValue(const std::string& name) { return std::get<T>(data.at(name)); }

		template<typename T>
		inline const T& getDataValue(const std::string& name) const { return std::get<T>(data.at(name)); }

		template<typename T, typename ValueT>
		inline static auto getDataValue(ValueT&& value) { return std::get<T>(value); }

		inline static DataValueType getDataValueType(const ValueType& value)
		{
			if (std::holds_alternative<RefType>(value))
				return DataValueType::Ref;
			if (std::holds_alternative<RealType>(value))
				return DataValueType::Real;
			return DataValueType::Integer;
		}

		template <typename StrType, typename T>
		inline bool tryGetDataValue(StrType&& name, T& value) const
		{
			auto it = data.find(std::forward<StrType>(name));
			if (it == data.end()) return false;

			switch (getDataValueType(it->second))
			{
			case DataValueType::Ref:
				if constexpr (std::is_assignable_v<T, RefType>)
					value = getDataValue<RefType>(it->second);
				else
					return false;
				break;
			case DataValueType::Integer:
				if constexpr (std::is_arithmetic_v<T>)
					value = static_cast<T>(getDataValue<IntegerType>(it->second));
				else
					return false;
				break;
			case DataValueType::Real:
				if constexpr (std::is_floating_point_v<T>)
					value = static_cast<T>(getDataValue<RealType>(it->second));
				else
					return false;
				break;
			default:
				return false;
			}
			return true;
		}
	};

	struct LevelData
	{
		double bgmOffset;
		std::vector<LevelDataEntity> entities;
	};

	void to_json(nlohmann::json& json, const LevelDataEntity& levelData);
	void to_json(nlohmann::json& json, const LevelData& levelData);

	void from_json(const nlohmann::json& json, LevelDataEntity& levelData);
	void from_json(const nlohmann::json& json, LevelData& levelData);
}
