#pragma once
#include <stack>
#include <map>
#include <unordered_map>
#include <string>
#include "Score.h"

namespace MikuMikuWorld
{
	struct History
	{
		std::string description;
		Score prev;
		Score curr;
	};

	class HistoryManager
	{
	private:
		std::stack<History> undoHistory;
		std::stack<History> redoHistory;

	public:
		Score undo();
		Score redo();

		int undoCount();
		int redoCount();
		std::string peekUndo();
		std::string peekRedo();

		void pushHistory(const History& history);
		void pushHistory(const std::string& description, const Score& prev, const Score& curr);
		void clear();
		bool hasUndo();
		bool hasRedo();
	};
}