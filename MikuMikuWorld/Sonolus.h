#pragma once
#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <stdexcept>
#include "Score.h"
#include "Archive.h"
#include "JsonIO.h"

namespace Sonolus
{
	enum class ResourceType
	{
		LevelCover,
		LevelBgm,
		LevelPreview,
		LevelData,
		SkinThumbnail,
		SkinData,
		SkinTexture,
		BackgroundThumbnail,
		BackgroundData,
		BackgroundImage,
		BackgroundConfiguration,
		EffectThumbnail,
		EffectData,
		EffectAudio,
		ParticleThumbnail,
		ParticleData,
		ParticleTexture,
		EngineThumbnail,
		EngineData,
		EngineRom,
		EngineConfiguration,
		ServerBanner,
		EnginePlayData,
		EngineTutorialData,
		EnginePreviewData,
		EngineWatchData,
		ReplayData,
		ReplayConfiguration,
		PostThumbnail,
		PlaylistThumbnail,
		Unknown
	};

	template<ResourceType res_t>
	struct SRL
	{
		std::string hash;
		std::string url;
	};

	template<typename T>
	class UseItem
	{
		std::optional<T> item;
	public:
		inline bool useDefault() const { return !item.has_value(); };
		inline const T& getItem() const { return item.value(); }
		inline T& getItem() { return item.value(); }
		inline void setItem(T&& item) { this->item.emplace(std::move(item)); }
		UseItem() {}
		UseItem(T&& item) : item(item) {}
		UseItem(const T& item) : item(item) {}
	};

	struct Tag
	{
		std::string title;
		std::string icon;
	};

	struct SkinItem
	{
		std::string name;
		int version;
		std::string title;
		std::string subtitle;
		std::string author;
		SRL<ResourceType::SkinThumbnail> thumbnail;
		SRL<ResourceType::SkinData> data;
		SRL<ResourceType::SkinTexture> texture;
		std::vector<Tag> tags;
		std::string source;

		static inline std::string_view getTypeName() { return "skin"; }
	};

	struct BackgroundItem
	{
		std::string name;
		int version;
		std::string title;
		std::string subtitle;
		std::string author;
		SRL<ResourceType::BackgroundThumbnail> thumbnail;
		SRL<ResourceType::BackgroundData> data;
		SRL<ResourceType::BackgroundImage> image;
		SRL<ResourceType::BackgroundConfiguration> configuration;
		std::vector<Tag> tags;
		std::string source;

		static inline std::string_view getTypeName() { return "background"; }
	};

	struct EffectItem {
		std::string name;
		int version;
		std::string title;
		std::string subtitle;
		std::string author;
		SRL<ResourceType::EffectThumbnail> thumbnail;
		SRL<ResourceType::EffectData> data;
		SRL<ResourceType::EffectAudio> audio;
		std::vector<Tag> tags;
		std::string source;

		static inline std::string_view getTypeName() { return "effect"; }
	};

	struct ParticleItem
	{
		std::string name;
		int version;
		std::string title;
		std::string subtitle;
		std::string author;
		SRL<ResourceType::ParticleThumbnail> thumbnail;
		SRL<ResourceType::ParticleData> data;
		SRL<ResourceType::ParticleTexture> texture;
		std::vector<Tag> tags;
		std::string source;

		static inline std::string_view getTypeName() { return "particle"; }
	};

	struct EngineItem
	{
		std::string name;
		int version;
		std::string title;
		std::string subtitle;
		std::string author;
		SkinItem skin;
		BackgroundItem background;
		EffectItem effect;
		ParticleItem particle;
		SRL<ResourceType::EngineThumbnail> thumbnail;
		SRL<ResourceType::EngineData> playData;
		SRL<ResourceType::EngineWatchData> watchData;
		SRL<ResourceType::EnginePreviewData> previewData;
		SRL<ResourceType::EngineTutorialData> tutorialData;
		SRL<ResourceType::EngineConfiguration> configuration;
		SRL<ResourceType::EngineRom> rom;
		std::vector<Tag> tags;
		std::string source;

		static inline std::string_view getTypeName() { return "engine"; }
	};

	struct LevelItem
	{
		std::string name;
		int version;
		int rating;
		std::string title;
		std::string artists;
		std::string author;
		EngineItem engine;
		UseItem<SkinItem> useSkin;
		UseItem<BackgroundItem> useBackground;
		UseItem<EffectItem> useEffect;
		UseItem<ParticleItem> useParticle;
		SRL<ResourceType::LevelCover> cover;
		SRL<ResourceType::LevelBgm> bgm;
		SRL<ResourceType::LevelData> data;
		SRL<ResourceType::LevelPreview> preview;
		std::vector<Tag> tags;
		std::string source;

		static inline std::string_view getTypeName() { return "level"; }
	};

	struct ServerForm { };
	struct ServerItemLeaderboard {};

	template<typename T>
	struct ItemList
	{
		int pageCount;
		std::vector<T> items;
	};

	template<typename T>
	struct ItemSection
	{
		std::string title;
		std::string icon;
		std::string description;
		std::string help;
		std::vector<T> items = {};

		static inline std::string_view getItemType() { return T::getTypeName(); }
	};

	template<typename T>
	struct ItemDetail
	{
		T item;
		std::string description;
		// Not Implemented
		bool hasCommunity = false;
		ServerForm* actions = nullptr;
		ServerItemLeaderboard* leaderboards = nullptr;
		ItemSection<void*>* sections = nullptr;
	};

	template<typename T>
	struct ItemInfo
	{
		std::vector<ItemSection<T>> sections;
	};

	struct SonolusSerializingError : std::runtime_error
	{
		SonolusSerializingError(const char* message) : std::runtime_error(message) { }
		SonolusSerializingError(const std::string& message) : std::runtime_error(message) { }
	};

	extern int skinVersion;
	extern int backgroundVersion;
	extern int effectVersion;
	extern int particleVersion;
	extern int engineVersion;
	extern int levelVersion;

	struct LevelDataEntity
	{
		enum class DataValueType { Integer, Real, Ref };
		using IntegerType = int;
		using RealType = float;
		using RefType = std::string;

		using ValueType = std::variant<IntegerType, RealType, RefType>;
		using MapDataType = std::map<std::string, ValueType>;
		
		std::string name;
		std::string archetype;
		MapDataType data;
		LevelDataEntity() = default;
		LevelDataEntity(const std::string& archetype) : archetype(archetype) { }
		template<typename StrArg, typename MapArg>
		LevelDataEntity(StrArg&& archetype, MapArg&& data) : archetype(std::forward<StrArg>(archetype)), data(std::forward<MapArg>(data)) { }
		LevelDataEntity(const std::string& archetype, std::initializer_list<MapDataType::value_type> init) : archetype(archetype), data(init) { }
		LevelDataEntity(const std::string& name, const std::string& archetype, const MapDataType& data) : name(name), archetype(archetype), data(data) { }

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
		float bgmOffset;
		std::vector<LevelDataEntity> entities;
	};

	bool createSonolusCollection(const std::string& filename);
	LevelItem generateLevelItem(const mmw::Score& score);

	void serializeLevelDetails(IO::Archive& scpArchive, const ItemDetail<LevelItem>& levelDetail);
	void serializeLevelInfo(IO::Archive& scpArchive, const ItemInfo<LevelItem>& levelInfo);
	void serializeLevelList(IO::Archive& scpArchive, const ItemList<LevelItem>& levelList);

	ItemDetail<LevelItem> deserializeLevelDetails(IO::Archive& scpArchive, const std::string levelName);

	ItemInfo<LevelItem> deserializeLevelInfo(const std::string& filename);
	ItemInfo<LevelItem> deserializeLevelInfo(IO::Archive& scpArchive);

	ItemList<LevelItem> deserializeLevelList(const std::string& filename);
	ItemList<LevelItem> deserializeLevelList(IO::Archive& scpArchive);

	void to_json(nlohmann::json& json, const LevelData& levelData);
	void from_json(const nlohmann::json& itemJson, LevelData& levelData);

	std::pair<std::string, std::string> packageData(IO::Archive& scpArchive, const std::vector<uint8_t>& buffer);
	std::pair<std::string, std::string> packageFile(IO::Archive& scpArchive, const std::string& filepath);

	template<ResourceType res_t>
	inline SRL<res_t> packageFile(IO::Archive& scpArchive, const std::string& filepath)
	{
		auto [hash, url] = packageFile(scpArchive, filepath);
		return { hash, url };
	}

	template<ResourceType res_t>
	inline SRL<res_t> packageData(IO::Archive& scpArchive, const std::vector<uint8_t>& buffer)
	{
		auto [hash, url] = packageData(scpArchive, buffer);
		return { hash, url };
	}

	std::string hash(const std::vector<uint8_t>& data);
}
