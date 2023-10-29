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

	struct HiSpeedChange
	{
		int tick;
		float speed;
	};

	struct ScoreMetadata
	{
		std::string title;
		std::string artist;
		std::string author;
		std::string musicFile;
		std::string jacketFile;
		float musicOffset;
	};

	struct Score
	{
		ScoreMetadata metadata;
		std::unordered_map<int, Note> notes;
		std::unordered_map<int, HoldNote> holdNotes;
		std::vector<Tempo> tempoChanges;
		std::map<int, TimeSignature> timeSignatures;
		std::vector<HiSpeedChange> hiSpeedChanges;
		std::vector<SkillTrigger> skills;
		Fever fever;

		Score();
	};

	Score deserializeScore(const std::string& filename);
	void serializeScore(const Score& score, const std::string& filename);
}