#include "HistoryManager.h"

namespace MikuMikuWorld
{
	Score HistoryManager::undo()
	{
		History history = undoHistory.top();
		redoHistory.push(history);
		undoHistory.pop();

		return history.prev;
	}

	Score HistoryManager::redo()
	{
		History history = redoHistory.top();
		undoHistory.push(history);
		redoHistory.pop();

		return history.curr;
	}

	void HistoryManager::pushHistory(const std::string& description, const Score& prev, const Score& curr)
	{
		History history{ description, prev, curr };
		pushHistory(history);
	}

	void HistoryManager::pushHistory(const History& history)
	{
		undoHistory.push(history);

		while (!redoHistory.empty())
			redoHistory.pop();
	}

	void HistoryManager::clear()
	{
		while (!undoHistory.empty())
			undoHistory.pop();

		while (!redoHistory.empty())
			redoHistory.pop();
	}

	bool HistoryManager::hasUndo() const { return undoHistory.size(); }

	bool HistoryManager::hasRedo() const { return redoHistory.size(); }

	int HistoryManager::undoCount() const { return undoHistory.size(); }

	int HistoryManager::redoCount() const { return redoHistory.size(); }

	std::string HistoryManager::peekUndo() const { return undoHistory.size() ? undoHistory.top().description : ""; }

	std::string HistoryManager::peekRedo() const { return redoHistory.size() ? redoHistory.top().description : ""; }
}