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
		FlickType flick;

		Note(NoteType _type);
		Note();

		NoteType getType() const;
		bool isFlick() const;
		bool hasEase() const;

		void setLane(int l);
		void setWidth(int w);

		inline bool operator<(const Note& other)
		{
			return tick < other.tick;
		}
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
	void setStepEase(HoldStep& step, EaseType ease);
	void cycleStepType(HoldStep& note);
	void sortHoldSteps(const Score& score, HoldNote& note);
	int getFlickArrowSpriteIndex(const Note& note);
	int getNoteSpriteIndex(const Note& note);
	int findHoldStep(const HoldNote& note, int stepID);
	std::string getNoteSE(const Note& note, const Score& score);
}