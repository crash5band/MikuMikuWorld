#pragma once
#include "Note.h"
#include "Tempo.h"
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace MikuMikuWorld
{
	extern int nextSkillID;
	extern int nextHiSpeedID;

	struct SkillTrigger
	{
		int ID;
		int tick;
	};

	struct Fever
	{
		int startTick;
		int endTick;
	};

	struct Layer
	{
		std::string name;
	};

	struct Waypoint
	{
		std::string name;
		int tick;
	};

	struct HiSpeedChange
	{
		int ID;
		int tick;
		float speed;
		int layer = 0;
	};

	struct ScoreMetadata
	{
		std::string title;
		std::string artist;
		std::string author;
		std::string musicFile;
		std::string jacketFile;
		float musicOffset;

		int laneExtension = 0;
	};

	struct Score
	{
		ScoreMetadata metadata;
		std::unordered_map<int, Note> notes;
		std::unordered_map<int, HoldNote> holdNotes;
		std::vector<Tempo> tempoChanges;
		std::map<int, TimeSignature> timeSignatures;
		std::unordered_map<int, HiSpeedChange> hiSpeedChanges;
		std::vector<SkillTrigger> skills;
		Fever fever;

		std::vector<Layer> layers{ { Layer{ "default" } } };
		std::vector<Waypoint> waypoints;

		Score();
	};

	Score deserializeScore(const std::string& filename);
	void serializeScore(const Score& score, const std::string& filename);
}
