#pragma once
#include "NoteTypes.h"
#include <vector>
#include <string>
#include <string_view>

namespace MikuMikuWorld
{
	constexpr int MIN_NOTE_WIDTH = 1;
	constexpr int MAX_NOTE_WIDTH = 12;
	constexpr int MIN_LANE = 0;
	constexpr int MAX_LANE = 11;
	constexpr int NUM_LANES = 12;

	constexpr const char* SE_PERFECT			= "perfect";
	constexpr const char* SE_FLICK				= "flick";
	constexpr const char* SE_TICK				= "tick";
	constexpr const char* SE_FRICTION			= "friction";
	constexpr const char* SE_CONNECT			= "connect";
	constexpr const char* SE_CRITICAL_TAP		= "critical_tap";
	constexpr const char* SE_CRITICAL_FLICK		= "critical_flick";
	constexpr const char* SE_CRITICAL_TICK		= "critical_tick";
	constexpr const char* SE_CRITICAL_FRICTION	= "critical_friction";
	constexpr const char* SE_CRITICAL_CONNECT	= "critical_connect";

	constexpr const char* SE_NAMES[] =
	{
		SE_PERFECT,
		SE_FLICK,
		SE_TICK,
		SE_FRICTION,
		SE_CONNECT,
		SE_CRITICAL_TAP,
		SE_CRITICAL_FLICK,
		SE_CRITICAL_TICK,
		SE_CRITICAL_FRICTION,
		SE_CRITICAL_CONNECT
	};

	constexpr float flickArrowWidths[] =
	{
		0.95f, 1.25f, 1.8f, 2.3f, 2.6f, 3.2f
	};

	constexpr float flickArrowHeights[] =
	{
		1, 1.05f, 1.2f, 1.4f, 1.5f, 1.6f
	};

	enum class ZIndex : int32_t
	{
		HoldLine,
		Guide,
		Note,
		FlickArrow
	};

	struct NoteTextures
	{
		int notes;
		int holdPath;
		int touchLine;
	};

	extern NoteTextures noteTextures;
	extern int nextID;

	class Note
	{
	private:
		NoteType type;

	public:
		int ID;
		int parentID;
		int tick;
		int lane;
		int width;
		bool critical{ false };
		bool friction{ false };
		FlickType flick{ FlickType::None };

		explicit Note(NoteType _type);
		explicit Note(NoteType _type, int tick, int lane, int width);
		Note();

		constexpr NoteType getType() const { return type; }

		bool isFlick() const;
		bool hasEase() const;
	};

	struct HoldStep
	{
		int ID;
		HoldStepType type;
		EaseType ease;
	};

	class HoldNote
	{
	public:
		HoldStep start;
		std::vector<HoldStep> steps;
		int end;

		HoldNoteType startType{};
		HoldNoteType endType{};

		constexpr bool isGuide() const
		{ 
			return startType == HoldNoteType::Guide || endType == HoldNoteType::Guide;
		}
	};


	struct Score;

	void resetNextID();

	void cycleFlick(Note& note);
	void cycleStepEase(HoldStep& note);
	void cycleStepType(HoldStep& note);
	void sortHold(Score& score, HoldNote& note);
	void sortHoldSteps(const Score& score, HoldNote& note);
	int findHoldStep(const HoldNote& note, int stepID);

	int getFlickArrowSpriteIndex(const Note& note);
	int getNoteSpriteIndex(const Note& note);
	int getFrictionSpriteIndex(const Note& note);
	std::string_view getNoteSE(const Note& note, const Score& score);
}
