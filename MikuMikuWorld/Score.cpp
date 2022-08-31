#include "Score.h"
#include "File.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "StringOperations.h"
#include "Constants.h"
#include <unordered_set>

namespace MikuMikuWorld
{
	Score::Score()
	{
		metadata.title = "";
		metadata.author = "";
		metadata.artist = "";
		metadata.musicOffset = 0;

		tempoChanges.push_back(Tempo());
		timeSignatures.insert(std::pair<int, TimeSignature>(0, TimeSignature{ 0, 4, 4 }));

		fever.startTick = fever.endTick = -1;
	}

	Note readNote(NoteType type, BinaryReader* reader)
	{
		Note note(type);
		note.tick = reader->readInt32();
		note.lane = reader->readInt32();
		note.width = reader->readInt32();

		if (!note.hasEase())
			note.flick = (FlickType)reader->readInt32();

		note.critical = (bool)reader->readInt32();
		return note;
	}

	void writeNote(const Note& note, BinaryWriter* writer)
	{
		writer->writeInt32(note.tick);
		writer->writeInt32(note.lane);
		writer->writeInt32(note.width);

		if (!note.hasEase())
			writer->writeInt32((int)note.flick);
		writer->writeInt32(note.critical);
	}

	Score deserializeScore(const std::string& filename)
	{
		Score score;
		BinaryReader reader(filename);
		if (!reader.isStreamValid())
			return score;

		std::string signature = reader.readString();
		int version = reader.readInt32();

		score.tempoChanges.clear();
		score.timeSignatures.clear();

		score.metadata.title = reader.readString();
		score.metadata.author = reader.readString();
		score.metadata.artist = reader.readString();
		score.metadata.musicFile = reader.readString();
		score.metadata.musicOffset = reader.readSingle();

		int timeSignatureCount = reader.readInt32();
		for (int i = 0; i < timeSignatureCount; ++i)
		{
			int measure = reader.readInt32();
			int numerator = reader.readInt32();
			int denominator = reader.readInt32();
			score.timeSignatures[measure] = TimeSignature{ measure, numerator, denominator };
		}

		int tempoCount = reader.readInt32();
		for (int i = 0; i < tempoCount; ++i)
		{
			int tick = reader.readInt32();
			float bpm = reader.readSingle();
			score.tempoChanges.push_back(Tempo{ tick, bpm });
		}

		int noteCount = reader.readInt32();
		score.notes.reserve(noteCount);
		for (int i = 0; i < noteCount; ++i)
		{
			Note note = readNote(NoteType::Tap, &reader);
			note.ID = nextID++;
			score.notes[note.ID] = note;
		}

		int holdCount = reader.readInt32();
		score.holdNotes.reserve(holdCount);
		for (int i = 0; i < holdCount; ++i)
		{
			HoldNote hold;

			Note start = readNote(NoteType::Hold, &reader);
			start.ID = nextID++;
			hold.start.ease = (EaseType)reader.readInt32();
			hold.start.ID = start.ID;
			score.notes[start.ID] = start;

			int stepCount = reader.readInt32();
			hold.steps.reserve(stepCount);
			for (int i = 0; i < stepCount; ++i)
			{
				Note mid = readNote(NoteType::HoldMid, &reader);
				mid.ID = nextID++;
				mid.parentID = start.ID;
				score.notes[mid.ID] = mid;

				HoldStep step;
				step.type = (HoldStepType)reader.readInt32();
				step.ease = (EaseType)reader.readInt32();
				step.ID = mid.ID;
				hold.steps.push_back(step);
			}

			Note end = readNote(NoteType::HoldEnd, &reader);
			end.ID = nextID++;
			end.parentID = start.ID;
			score.notes[end.ID] = end;
			
			hold.end = end.ID;
			score.holdNotes[start.ID] = hold;
		}

		reader.close();
		return score;
	}

	void serializeScore(const Score& score, const std::string& filename)
	{
		BinaryWriter writer(filename);
		if (!writer.isStreamValid())
			return;

		// signature
		writer.writeString("MMWS");

		// verison
		writer.writeInt32(1);

		// metadata
		writer.writeString(score.metadata.title);
		writer.writeString(score.metadata.author);
		writer.writeString(score.metadata.artist);
		writer.writeString(File::fixPath(score.metadata.musicFile));
		writer.writeSingle(score.metadata.musicOffset);

		int timeSignatureCount = score.timeSignatures.size();
		writer.writeInt32(timeSignatureCount);

		for (const auto&[measure, ts] : score.timeSignatures)
		{
			writer.writeInt32(ts.measure);
			writer.writeInt32(ts.numerator);
			writer.writeInt32(ts.denominator);
		}

		int tempoCount = score.tempoChanges.size();
		writer.writeInt32(tempoCount);

		for (int i = 0; i < tempoCount; ++i)
		{
			writer.writeInt32(score.tempoChanges[i].tick);
			writer.writeSingle(score.tempoChanges[i].bpm);
		}

		size_t noteCountAddress = writer.getStreamPosition();
		writer.writeNull(sizeof(uint32_t));

		int noteCount = 0;
		for (const auto&[id, note] : score.notes)
		{
			if (note.getType() != NoteType::Tap)
				continue;

			writeNote(note, &writer);
			++noteCount;
		}

		size_t holdsAddress = writer.getStreamPosition();
		writer.seek(noteCountAddress);
		writer.writeInt32(noteCount);
		writer.seek(holdsAddress);
			
		int holdCount = score.holdNotes.size();
		writer.writeInt32(holdCount);
		for (const auto&[id, hold] : score.holdNotes)
		{	
			// note data
			const Note& start = score.notes.at(hold.start.ID);
			writeNote(start, &writer);
			writer.writeInt32((int)hold.start.ease);

			// steps
			int stepCount = hold.steps.size();
			writer.writeInt32(stepCount);
			for (const auto& step : hold.steps)
			{
				const Note& mid = score.notes.at(step.ID);
				writeNote(mid, &writer);
				writer.writeInt32((int)step.type);
				writer.writeInt32((int)step.ease);
			}

			// end
			const Note& end = score.notes.at(hold.end);
			writeNote(end, &writer);
		}

		writer.flush();
		writer.close();
	}
}
