#pragma once
#include <string>
#include "SUS.h"
#include "Score.h"

namespace MikuMikuWorld
{
	class SUSIO
	{
	private:
		std::pair<int, int> simplify4(int numerator, int denominator);
		std::string noteKey(const SUSNote& note);

	public:
		Score importSUS(const std::string& filename);
		void exportSUS(const Score& score, const std::string& filename);
	};
}