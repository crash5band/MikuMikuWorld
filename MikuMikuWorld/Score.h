#pragma once
#include "Note.h"
#include "Tempo.h"
#include <string>
#include <map>
#include <unordered_map>
#include <vector>

namespace MikuMikuWorld
{
	struct ScoreMetadata
	{
		std::string filename;
		std::string title;
		std::string artist;
		std::string author;
		std::string musicFile;
		int difficulty;
		float offset;
	};

	struct Score
	{
		ScoreMetadata metadata;
		std::unordered_map<int, Note> notes;
		std::unordered_map<int, HoldNote> holdNotes;
		std::vector<Tempo> tempoChanges;
		std::map<int, TimeSignature> timeSignatures;

		Score();
	};

	Score loadScore(const std::string& filename);
	void saveScore(const Score& score, const std::string& filename);

	std::vector<int> countNotes(const Score& score);
}