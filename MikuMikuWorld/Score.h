#pragma once
#include "Note.h"
#include "Tempo.h"
#include "ScoreEvents.h"
#include <string>
#include <map>
#include <unordered_map>
#include <vector>

namespace MikuMikuWorld
{
	struct ScoreMetadata
	{
		std::string title;
		std::string artist;
		std::string author;
		std::string musicFile;
		float musicOffset;
	};

	struct Score
	{
		ScoreMetadata metadata;
		std::unordered_map<int, Note> notes;
		std::unordered_map<int, HoldNote> holdNotes;
		std::vector<Tempo> tempoChanges;
		std::map<int, TimeSignature> timeSignatures;
		std::vector<SkillTrigger> skills;
		Fever fever;

		Score();
	};

	Score deserializeScore(const std::string& filename);
	void serializeScore(const Score& score, const std::string& filename);
}