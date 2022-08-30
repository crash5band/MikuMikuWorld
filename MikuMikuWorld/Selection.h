#pragma once
#include <unordered_set>
#include "Score.h"

namespace MikuMikuWorld
{
	class Selection
	{
	private:
		std::unordered_set<int> selectedNotes;
		bool _hasEase;
		bool _hasStep;
		bool _hasFlick;

		bool _hasSelectionEase(const Score& score);
		bool _hasSelectionStep(const Score& score);
		bool _hasSelectionFlick(const Score& score);

	public:
		Selection();

		void clear();
		void append(int noteID);
		void remove(int noteID);
		void update(const Score& score);

		int count() const;

		bool hasSelection() const;
		bool hasEase() const;
		bool hasStep() const;
		bool hasFlickable() const;

		const std::unordered_set<int>& getSelection();
		std::unordered_set<int> getHolds(const Score& score);
	};
}


