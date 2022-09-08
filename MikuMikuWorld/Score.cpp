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

	ScoreMetadata readMetadata(BinaryReader* reader, int version)
	{
		ScoreMetadata metadata;
		metadata.title = reader->readString();
		metadata.author = reader->readString();
		metadata.artist = reader->readString();
		metadata.musicFile = reader->readString();
		metadata.musicOffset = reader->readSingle();

		if (version > 1)
			metadata.jacketFile = reader->readString();

		return metadata;
	}

	void writeMetadata(const ScoreMetadata& metadata, BinaryWriter* writer)
	{
		writer->writeString(metadata.title);
		writer->writeString(metadata.author);
		writer->writeString(metadata.artist);
		writer->writeString(File::fixPath(metadata.musicFile));
		writer->writeSingle(metadata.musicOffset);
		writer->writeString(File::fixPath(metadata.jacketFile));
	}

	Score deserializeScore(const std::string& filename)
	{
		Score score;
		BinaryReader reader(filename);
		if (!reader.isStreamValid())
			return score;

		std::string signature = reader.readString();
		if (signature != "MMWS")
			throw std::runtime_error("Invalid mmws file signature.");

		int version = reader.readInt32();

		score.metadata = readMetadata(&reader, version);

		int timeSignatureCount = reader.readInt32();
		if (timeSignatureCount)
			score.timeSignatures.clear();

		for (int i = 0; i < timeSignatureCount; ++i)
		{
			int measure = reader.readInt32();
			int numerator = reader.readInt32();
			int denominator = reader.readInt32();
			score.timeSignatures[measure] = TimeSignature{ measure, numerator, denominator };
		}

		int tempoCount = reader.readInt32();
		if (tempoCount)
			score.tempoChanges.clear();

		for (int i = 0; i < tempoCount; ++i)
		{
			int tick = reader.readInt32();
			float bpm = reader.readSingle();
			score.tempoChanges.push_back(Tempo{ tick, bpm });
		}

		if (version > 1)
		{
			int skillCount = reader.readInt32();
			for (int i = 0; i < skillCount; ++i)
			{
				int tick = reader.readInt32();
				score.skills.push_back(SkillTrigger{ nextSkillID++, tick });
			}

			score.fever.startTick = reader.readInt32();
			score.fever.endTick = reader.readInt32();
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
		writer.writeInt32(2);

		writeMetadata(score.metadata, &writer);

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

		int skillCount = score.skills.size();
		writer.writeInt32(skillCount);

		for (int i = 0; i < skillCount; ++i)
		{
			writer.writeInt32(score.skills[i].tick);
		}

		writer.writeInt32(score.fever.startTick);
		writer.writeInt32(score.fever.endTick);

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
