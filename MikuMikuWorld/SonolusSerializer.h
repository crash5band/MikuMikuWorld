#pragma once
#include "ScoreSerializer.h"
#include "Sonolus.h"
#include "Constants.h"
#include <string>
#include <memory>

namespace MikuMikuWorld
{
	class SonolusEngine; // Forward declaration
    class SonolusSerializer : public ScoreSerializer
    {
    public:
        void serialize(const Score& score, std::string filename) override;
        Score deserialize(std::string filename) override;

		SonolusSerializer(std::unique_ptr<SonolusEngine>&& engine, bool compressData = true, bool prettyDump = false)
			: engine(std::move(engine)), compressData(compressData), prettyDump(prettyDump)
		{

		}

	private:
		std::unique_ptr<SonolusEngine> engine;
		bool compressData;
		bool prettyDump;
    };

	class SonolusEngine
	{
	protected:
		using IntegerType = Sonolus::LevelDataEntity::IntegerType;
		using RealType = Sonolus::LevelDataEntity::RealType;
		using RefType = Sonolus::LevelDataEntity::RefType;
		using FuncRef = std::function<RefType(void)>;
		using TickType = decltype(Note::tick);

		static double toBgmOffset(float musicOffset);
		static Sonolus::LevelDataEntity toBpmChangeEntity(const Tempo& tempo);
		static RealType ticksToBeats(TickType ticks, TickType beatTicks = TICKS_PER_BEAT);
		static RealType widthToSize(int width);
		static RealType toSonolusLane(int lane, int width);

		static float fromBgmOffset(double bgmOffset);
		static bool fromBpmChangeEntity(const Sonolus::LevelDataEntity& bpmChangeEntity, Tempo& tempo);
		static TickType beatsToTicks(RealType beats, TickType beatTicks = TICKS_PER_BEAT);
		static int sizeToWidth(RealType size);
		static int toNativeLane(RealType lane, RealType size);

		static bool isValidNoteState(const Note& note);
		static bool isValidHoldNotes(const std::vector<Note>& holdNotes);

		static std::string getTapNoteArchetype(const Note& note);
	public:
		virtual Sonolus::LevelData serialize(const Score& score) = 0;
		virtual Score deserialize(const Sonolus::LevelData& levelData) = 0;

		// virtual ~SonolusEngine() = default;
	};

	class PySekaiEngine : public SonolusEngine
	{
	public:
		Sonolus::LevelData serialize(const Score& score) override;
		Score deserialize(const Sonolus::LevelData& levelData) override;

	private:
		static Sonolus::LevelDataEntity toSpeedChangeEntity(const HiSpeedChange& hispeed, const RefType& groupName);
		static Sonolus::LevelDataEntity toNoteEntity(
			const Note& note, const std::string& archetype, const RefType& groupName,
			HoldNoteType holdType = HoldNoteType::Normal, HoldStepType stepType = HoldStepType::Normal, EaseType easing = EaseType::Linear, bool isGuide = false, double alpha = 1.0
		);
		static Sonolus::LevelDataEntity toConnector(const HoldNote& hold, const RefType& head, const RefType& tail, const RefType& segmentHead, const RefType& segmentTail);
		static std::string getHoldNoteArchetype(const Note& note, const HoldNote& holdNote);
		static void insertTransientTickNote(const Sonolus::LevelDataEntity& head, const Sonolus::LevelDataEntity& tail, bool isHead, std::vector<Sonolus::LevelDataEntity>& entities);
		static int toDirectionNumeric(FlickType flick);
		static int toEaseNumeric(EaseType ease);
		static int toKindNumeric(bool critical = false, HoldNoteType holdType = HoldNoteType::Normal);
	};
}