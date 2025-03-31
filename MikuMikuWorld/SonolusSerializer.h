#pragma once
#include "ScoreSerializer.h"
#include <string>
#include "JsonIO.h"
#include "json.hpp"

using json = nlohmann::json;

namespace MikuMikuWorld
{
	enum class DataValueType : uint8_t
	{
		Value, Ref
	};

	enum class NumberType
	{
		Integer,
		Float
	};

	union Number
	{
		int intValue;
		float floatValue;
	};

	class DataValue
	{
	public:
		inline std::string getName() const { return mName; }
		inline float getFloatValue() const { return mValue.floatValue; }
		inline int getIntValue() const { return mValue.intValue; }
		inline std::string getRef() const { return mRef; }
		inline DataValueType getType() const { return mType; }
		inline NumberType getNumType() const { return mNumType; }

		DataValue(std::string name, float v) : mType{ DataValueType::Value }, mNumType{ NumberType::Float }, mName{ name }
		{
			mValue.floatValue = v;
		}

		explicit DataValue(std::string name, int v) : mType{ DataValueType::Value }, mNumType{ NumberType::Integer }, mName{ name }
		{
			mValue.intValue = v;
		}

		DataValue(std::string name, std::string r) : mType{ DataValueType::Ref }, mName{ name }, mRef { r }
		{
		}

	private:
		std::string mName;
		DataValueType mType;
		NumberType mNumType;
		Number mValue;
		std::string mRef;
	};

	struct ArchetypeData
	{
		std::string name;
		std::string archetype;
		std::vector<DataValue> data;
	};

	struct SlideConnection
	{
		std::string head;
		std::string tail;
		int ease;
		bool active;
		bool critical;
	};

	void to_json(json& j, const ArchetypeData& data);

    class SonolusSerializer : public ScoreSerializer
    {
    public:
        void serialize(Score score, std::string filename) override;
        Score deserialize(std::string filename) override;

		SonolusSerializer(bool prettyDump, bool gzip) : prettyDump{ prettyDump }, useGzip{ gzip }
		{

		}

	private:
		bool prettyDump;
		bool useGzip;

		float ticksToBeats(int ticks);
		float toSonolusLane(int lane, int width);
		int flickToDirection(FlickType flick);
		int toSonolusEase(EaseType ease);

		int beatsToTicks(float beats);
		int toNativeLane(float lane, float width);
		FlickType directionToFlick(int direction);
		EaseType toNativeEase(int ease);

		json getEntitiesByArchetype(const json& j, const std::string& archetype);
		json getEntitiesByArchetypeEndingWith(const json& j, const std::string& archetype);
		std::string getSlideConnectorId(const HoldStep& step);
		std::string noteToArchetype(const Score& score, const Note& note);
		std::map<int, std::vector<Note>> buildTickNoteMap(const Score& score);

		std::string getSlideConnectorArchetype(const Score& score, const HoldNote& hold);
		std::string getParentSlideConnector(const std::unordered_map<std::string, std::string>& connections, const std::string& slideKey);
		std::string getLastSlideConnector(const std::unordered_map<std::string, SlideConnection>& connections, const std::string& slideKey);

		Note createNote(const json& entity, NoteType type);
		NoteType getNoteTypeFromArchetype(const std::string& archetype);

		template<typename T>
		T getEntityDataOrDefault(const json& data, const std::string& name, const T& def)
		{
			auto it = std::find_if(data.begin(), data.end(),
				[&name](const auto& entry) { return jsonIO::tryGetValue<std::string>(entry, "name", "") == name; });

			if (it != data.end())
			{
				if constexpr (std::is_same_v<T, std::string>)
					return jsonIO::tryGetValue<std::string>(it.value(), "ref", def);
				else
					return jsonIO::tryGetValue<float>(it.value(), "value", def);
			}

			return def;
		}
    };
}


