#pragma once
#include "Score.h"
#include "ScoreStats.h"
#include "HistoryManager.h"
#include "Audio/AudioManager.h"
#include "Audio/Waveform.h"
#include "JsonIO.h"
#include "Jacket.h"
#include "Constants.h"
#include "TimelineMode.h"
#include "PreviewEngine.h"
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

	class EditorScoreData
	{
	public:
		std::string title{};
		std::string designer{};
		std::string artist{};
		std::string filename{};
		std::string musicFilename{};
		float musicOffset{};
		Jacket jacket{};

		EditorScoreData() {}
		EditorScoreData(const ScoreMetadata& metadata, const std::string& filename) :
			title{ metadata.title }, designer{ metadata.author }, artist{ metadata.artist },
			musicFilename{ metadata.musicFile }, musicOffset{ metadata.musicOffset }
		{
			this->filename = filename;
			jacket.load(metadata.jacketFile);
		}

		ScoreMetadata toScoreMetadata() const
		{
			return { title, artist, designer, musicFilename, jacket.getFilename(), musicOffset };
		}
	};

	struct PasteData
	{
		std::map<int, Note> notes;
		std::map<int, HoldNote> holds;
		bool pasting{ false };
		int minTick{};
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
		Audio::AudioManager audio;
		PasteData pasteData{};
		std::unordered_set<int> selectedNotes;
		Engine::DrawData scorePreviewDrawData;
		Audio::WaveformMipChain waveformL, waveformR;

		int currentTick{};
		bool upToDate{ true };

		std::unordered_set<int> getHoldsFromSelection() const
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

		double getTimeAtCurrentTick() const
		{
			return accumulateDuration(currentTick, TICKS_PER_BEAT, score.tempoChanges);
		}

		int minTickFromSelection() const;
		bool selectionHasEase() const;
		bool selectionHasStep() const;
		bool selectionHasAnyStep() const;
		bool selectionHasFlickable() const;
		bool selectionCanConnect() const;
		bool selectionCanChangeHoldType() const;
		inline bool isNoteSelected(const Note& note) { return selectedNotes.find(note.ID) != selectedNotes.end(); }
		inline void selectAll() { selectedNotes.clear(); for (auto& it : score.notes) selectedNotes.insert(it.first); }
		inline void clearSelection() { selectedNotes.clear(); }

		void setStep(HoldStepType step);
		void setFlick(FlickType flick);
		void setEase(EaseType ease);
		void setHoldType(HoldNoteType hold);
		void toggleCriticals();
		void toggleFriction();

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
		void splitHoldInSelection();

		void undo();
		void redo();
		void pushHistory(std::string description, const Score& prev, const Score& current);
	};
}
