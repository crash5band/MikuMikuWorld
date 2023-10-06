#pragma once
#include <string>

namespace MikuMikuWorld
{
	class Note;
	struct Score;
	struct SUS;
	struct SUSNote;

	class ScoreConverter
	{
	private:
		static std::pair<int, int> barLengthToFraction(float length, float fractionDenom);
		static std::string noteKey(const SUSNote& note);
		static std::string noteKey(const Note& note);

	public:
		static Score susToScore(const SUS& sus);
		static SUS scoreToSus(const Score& score);
	};
}