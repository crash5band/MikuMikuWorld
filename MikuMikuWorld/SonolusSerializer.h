#pragma once
#include "ScoreSerializer.h"
#include "ScoreEditorWindows.h"
#include <string>
#include "Sonolus.h"
#include "JsonIO.h"
#include "json.hpp"

using json = nlohmann::json;

namespace MikuMikuWorld
{
	struct LevelMetadata
	{
		enum Flag {
			None = 0,
			EngineNotRegconized = 1,
			LevelDataMissing = 1 << 1
		};
		std::string name;
		int rating;
		std::string title;
		std::string artists;
		std::string author;
		Flag flags;
	};

    class SonolusSerializer : public ScoreSerializer
    {
    public:
        void serialize(const Score& score, std::string filename) override;
        Score deserialize(std::string filename) override;

		SonolusSerializer() { }

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
    };

	class SonolusSerializationDialog : public ScoreSerializationDialog
	{
		intptr_t selectedItem = -1;
		std::string insertLevelName;
		std::vector<LevelMetadata> levelInfos;
		bool updateFileSelector(SerializationDialogResult& result);
	public:
		void openSerializingDialog(const ScoreContext& context, const std::string& filename) override;
		void openDeserializingDialog(const std::string& filename) override;
		SerializationDialogResult update() override;
	};
}
