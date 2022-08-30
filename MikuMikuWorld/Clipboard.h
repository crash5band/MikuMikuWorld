#pragma once
#include "Score.h"
#include <unordered_set>
#include <unordered_map>

namespace MikuMikuWorld
{
	struct ClipboardNotes
	{
		std::unordered_map<int, Note> notes;
		std::unordered_map<int, HoldNote> holds;
	};

	class Clipboard
	{
	private:
		static ClipboardNotes data;
		static ClipboardNotes flippedData;

	public:
		static void copy(const Score& score, const std::unordered_set<int>& selection);
		static const ClipboardNotes& getData(bool flipped);

		static int count();
		static bool hasData();
	};
}

