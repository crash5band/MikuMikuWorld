#pragma once
#include "Score.h"
#include <map>
#include <stack>
#include <string>
#include <unordered_map>

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

		int undoCount() const;
		int redoCount() const;
		std::string peekUndo() const;
		std::string peekRedo() const;

		void pushHistory(const History& history);
		void pushHistory(const std::string& description, const Score& prev, const Score& curr);
		void clear();
		bool hasUndo() const;
		bool hasRedo() const;
	};
}