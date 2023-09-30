#pragma once
#include "NoteTypes.h"
#include <vector>
#include <string>

namespace MikuMikuWorld
{
	struct Score;

	extern int nextID;

	class Note final
	{
	private:
		NoteType type;

	public:
		int ID;
		int parentID;
		int tick;
		int lane;
		int width;
		bool critical;
		bool friction;
		FlickType flick;

		Note(NoteType _type);
		Note();

		inline constexpr NoteType getType() const { return type; }

		bool isFlick() const;
		bool hasEase() const;

		void setLane(int l);
		void setWidth(int w);
	};

	struct HoldStep
	{
		int ID;
		HoldStepType type;
		EaseType ease;
	};

	class HoldNote final
	{
	public:
		HoldStep start;
		std::vector<HoldStep> steps;
		int end;
	};

	void resetNextID();

	void cycleFlick(Note& note);
	void cycleStepEase(HoldStep& note);
	void cycleStepType(HoldStep& note);
	void sortHoldSteps(const Score& score, HoldNote& note);
	int findHoldStep(const HoldNote& note, int stepID);

	int getFlickArrowSpriteIndex(const Note& note);
	int getNoteSpriteIndex(const Note& note);
	int getFrictionSpriteIndex(const Note& note);
	std::string getNoteSE(const Note& note, const Score& score);
}
