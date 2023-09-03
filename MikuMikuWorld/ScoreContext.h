#pragma once
#include "Score.h"
#include "ScoreStats.h"
#include "HistoryManager.h"
#include "Audio/AudioManager.h"
#include "JsonIO.h"
#include "Jacket.h"
#include "TimelineMode.h"
#include <unordered_set>

namespace MikuMikuWorld
{
	struct EditArgs
	{
		int noteWidth{ 3 };
		FlickType flickType{ FlickType::Default };
		HoldStepType stepType{ HoldStepType::Normal };
		EaseType easeType{ EaseType::Linear };

		float bpm{ 160.0f };
		int timeSignatureNumerator{ 4 };
		int timeSignatureDenominator{ 4 };
		float hiSpeed{ 1.0f };
	};

	struct EditorScoreData
	{
		std::string title{};
		std::string designer{};
		std::string artist{};
		std::string filename{};
		std::string musicFilename{};
		float musicOffset{};
		Jacket jacket{};
	};

	struct PasteData
	{
		std::unordered_map<int, Note> notes;
		std::unordered_map<int, HoldNote> holds;
		bool pasting{ false };
		int offsetTicks{};
		int offsetLane{};
		int midLane{};
		int minLaneOffset{};
		int maxLaneOffset{};
	};

	class ScoreContext
	{
	public:
		Score score;
		EditorScoreData workingData;
		ScoreStats scoreStats;
		HistoryManager history;
		AudioManager audio;
		PasteData pasteData{};
		std::unordered_set<int> selectedNotes;

		int currentTick{};
		bool upToDate{ true };

		std::unordered_set<int> getHoldsFromSelection()
		{
			std::unordered_set<int> holds;
			for (int id : selectedNotes)
			{
				const Note& note = score.notes.at(id);
				if (note.getType() == NoteType::Hold)
					holds.insert(note.ID);
				else if (note.getType() == NoteType::HoldMid || note.getType() == NoteType::HoldEnd)
					holds.insert(note.parentID);
			}

			return holds;
		}

		bool selectionHasEase() const;
		bool selectionHasStep() const;
		bool selectionHasFlickable() const;
		bool selectionCanConnect() const;
		inline bool isNoteSelected(const Note& note) { return selectedNotes.find(note.ID) != selectedNotes.end(); }
		inline void selectAll() { selectedNotes.clear(); for (auto& it : score.notes) selectedNotes.insert(it.first); }
		inline void clearSelection() { selectedNotes.clear(); }

		void setStep(HoldStepType step);
		void setFlick(FlickType flick);
		void setEase(EaseType ease);
		void toggleCriticals();

		void deleteSelection();
		void flipSelection();
		void cutSelection();
		void copySelection();
		void paste(bool flip);
		void doPasteData(const nlohmann::json& data, bool flip);
		void cancelPaste();
		void confirmPaste();
		void shrinkSelection(Direction direction);

		void connectHoldsInSelection();
		void ripHoldInSelection();

		void undo();
		void redo();
		void pushHistory(std::string description, const Score& prev, const Score& current);
	};
}
