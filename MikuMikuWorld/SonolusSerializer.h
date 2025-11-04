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

		SonolusSerializer(std::unique_ptr<SonolusEngine>&& engine, bool compressData = true, bool prettyDump = false) : engine(std::move(engine)), compressData(compressData), prettyDump(prettyDump)
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

	class SekaiBestEngine : public SonolusEngine
	{
		struct HoldJoint
		{
			size_t entityIdx;
			EaseType ease;
		};
		struct LinkedHoldStep
		{
			enum StepType : uint8_t { Joint, Attach, EndHold };
			enum Modifier : uint8_t { None = 0, Guide = 1, Critical = 2 };

			RefType nextTail;
			StepType step;
			Modifier mod;
			EaseType ease;
		};
		struct HoldSegment
		{
			bool critical = false, active = false;
			int connEase;
			RefType startRef, endRef, headRef, tailRef;
		};
	public:
		Sonolus::LevelData serialize(const Score& score) override;
		Score deserialize(const Sonolus::LevelData& levelData) override;
	private:
		static Sonolus::LevelDataEntity toSpeedChangeEntity(const HiSpeedChange& hispeed);
		static Sonolus::LevelDataEntity toTapNoteEntity(const Note& note);
		static Sonolus::LevelDataEntity toStartHoldNoteEntity(const Note& note, const HoldNote& holdNote);
		static Sonolus::LevelDataEntity toEndHoldNoteEntity(const Note& note, const HoldNote& holdNote, const FuncRef& getCurrJoint);
		static Sonolus::LevelDataEntity toTickNoteEntity(const Note& note, const HoldStep& step, const FuncRef& getCurrJoint);
		static Sonolus::LevelDataEntity toHiddenTickNoteEntity(TickType tick, const RefType& ref);
		static void insertHiddenTickNote(const Note& headNote, const Note& tailNote, bool isHead, std::vector<Sonolus::LevelDataEntity>& entities, const FuncRef& getCurrJoint);
		static int toDirectionNumeric(FlickType flick);
		static int toEaseNumeric(EaseType ease);

		static bool fromSpeedChangeEntity(const Sonolus::LevelDataEntity& speedChangeEntity, HiSpeedChange& hispeed);
		static bool fromNoteEntity(const Sonolus::LevelDataEntity& noteEntity, Note& note);
		static bool fromTapNoteEntity(const Sonolus::LevelDataEntity& tapNoteEntity, Note& note);
		static bool fromConnectorEntity(const Sonolus::LevelDataEntity& connectorEntity, HoldSegment& segment);
		static bool fromAttachTickEntity(const Sonolus::LevelDataEntity& attachEntity, RefType& connectorRef);
		static bool fromStartHoldNoteEntity(const Sonolus::LevelDataEntity& noteEntity, Note& startNote);
		static bool fromEndHoldNoteEntity(const Sonolus::LevelDataEntity& noteEntity, Note& endNote);
		static bool fromTickNoteEntity(const Sonolus::LevelDataEntity& tickEntity, Note& tickNote);
		static FlickType fromDirectionNumeric(int direction);
		static EaseType fromEaseNumeric(int ease);
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

	class SonolusEngineController : public ScoreSerializeController
	{
		bool updateEngineSelector(SerializeResult& result);
	public:

		SonolusEngineController(Score score, const std::string& filename);
		SonolusEngineController(const std::string& filename);
		SerializeResult update() override;
	private:
		bool isSerializing, open = true, compress = true;
		std::unique_ptr<SonolusSerializer> serializer;

		intptr_t selectedItem = -1;
	};
}