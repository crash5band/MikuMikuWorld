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

	bool HistoryManager::hasUndo()
	{
		return undoHistory.size();
	}

	bool HistoryManager::hasRedo()
	{
		return redoHistory.size();
	}

	int HistoryManager::undoCount()
	{
		return undoHistory.size();
	}

	int HistoryManager::redoCount()
	{
		return redoHistory.size();
	}

	std::string HistoryManager::peekUndo()
	{
		return undoHistory.size() ? undoHistory.top().description : "";
	}

	std::string HistoryManager::peekRedo()
	{
		return redoHistory.size() ? redoHistory.top().description : "";
	}
}