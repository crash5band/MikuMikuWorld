#pragma once
#include "ScoreSerializer.h"
#include "Sonolus.h"
#include "Constants.h"
#include <string>

namespace MikuMikuWorld
{
    class SonolusSerializer : public ScoreSerializer
    {
		using IntegerType = Sonolus::LevelDataEntity::IntegerType;
		using RealType = Sonolus::LevelDataEntity::RealType;

    public:
        void serialize(const Score& score, std::string filename) override;
        Score deserialize(std::string filename) override;

		Sonolus::LevelData serialize(const Score& score);
		Score deserialize(const Sonolus::LevelData& levelData);

		SonolusSerializer(bool compressData = true) : compressData(compressData) { }

		static RealType ticksToBeats(int ticks, int beatTicks = TICKS_PER_BEAT);
		static RealType widthToSize(int width);
		static RealType toSonolusLane(int lane, int width);
		static int toSonolusDirection(FlickType flick);
		static int toSonolusEase(EaseType ease);

		static int beatsToTicks(RealType beats, int beatTicks = TICKS_PER_BEAT);
		static int sizeToWidth(RealType size);
		static int toNativeLane(RealType lane, RealType size);
		static FlickType toNativeFlick(int direction);
		static EaseType toNativeEase(int ease);

		bool compressData;
    };
}

