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
		timeSignatures[0] = { 0, 4, 4 };

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

	void readScoreEvents(Score& score, int version, BinaryReader* reader)
	{
		// time signature
		int timeSignatureCount = reader->readInt32();
		if (timeSignatureCount)
			score.timeSignatures.clear();

		for (int i = 0; i < timeSignatureCount; ++i)
		{
			int measure = reader->readInt32();
			int numerator = reader->readInt32();
			int denominator = reader->readInt32();
			score.timeSignatures[measure] = { measure, numerator, denominator };
		}

		// bpm
		int tempoCount = reader->readInt32();
		if (tempoCount)
			score.tempoChanges.clear();

		for (int i = 0; i < tempoCount; ++i)
		{
			int tick = reader->readInt32();
			float bpm = reader->readSingle();
			score.tempoChanges.push_back({ tick, bpm });
		}

		// hi-speed
		if (version > 2)
		{
			int hiSpeedCount = reader->readInt32();
			for (int i = 0; i < hiSpeedCount; ++i)
			{
				int tick = reader->readInt32();
				float speed = reader->readSingle();

				score.hiSpeedChanges.push_back({ tick, speed });
			}
		}

		// skills and fever
		if (version > 1)
		{
			int skillCount = reader->readInt32();
			for (int i = 0; i < skillCount; ++i)
			{
				int tick = reader->readInt32();
				score.skills.push_back({ nextSkillID++, tick });
			}

			score.fever.startTick = reader->readInt32();
			score.fever.endTick = reader->readInt32();
		}
	}

	void writeScoreEvents(const Score& score, BinaryWriter* writer)
	{
		writer->writeInt32(score.timeSignatures.size());
		for (const auto& [_, timeSignature] : score.timeSignatures)
		{
			writer->writeInt32(timeSignature.measure);
			writer->writeInt32(timeSignature.numerator);
			writer->writeInt32(timeSignature.denominator);
		}

		writer->writeInt32(score.tempoChanges.size());
		for (const auto& tempo : score.tempoChanges)
		{
			writer->writeInt32(tempo.tick);
			writer->writeSingle(tempo.bpm);
		}

		writer->writeInt32(score.hiSpeedChanges.size());
		for (const auto& hiSpeed : score.hiSpeedChanges)
		{
			writer->writeInt32(hiSpeed.tick);
			writer->writeSingle(hiSpeed.speed);
		}

		writer->writeInt32(score.skills.size());
		for (const auto& skill : score.skills)
		{
			writer->writeInt32(skill.tick);
		}

		writer->writeInt32(score.fever.startTick);
		writer->writeInt32(score.fever.endTick);
	}

	Score deserializeScore(const std::string& filename)
	{
		Score score;
		BinaryReader reader(filename);
		if (!reader.isStreamValid())
			return score;

		std::string signature = reader.readString();
		if (signature != "MMWS")
			throw std::runtime_error("Not a MMWS file.");

		int version = reader.readInt32();

		uint32_t metadataAddress = 0;
		uint32_t eventsAddress = 0;
		uint32_t tapsAddress = 0;
		uint32_t holdsAddress = 0;
		if (version > 2)
		{
			metadataAddress = reader.readInt32();
			eventsAddress = reader.readInt32();
			tapsAddress = reader.readInt32();
			holdsAddress = reader.readInt32();

			reader.seek(metadataAddress);
		}

		score.metadata = readMetadata(&reader, version);

		if (version > 2)
			reader.seek(eventsAddress);

		readScoreEvents(score, version, &reader);

		if (version > 2)
			reader.seek(tapsAddress);

		int noteCount = reader.readInt32();
		score.notes.reserve(noteCount);
		for (int i = 0; i < noteCount; ++i)
		{
			Note note = readNote(NoteType::Tap, &reader);
			note.ID = nextID++;
			score.notes[note.ID] = note;
		}

		if (version > 2)
			reader.seek(holdsAddress);

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
		writer.writeInt32(3);

		// offsets address in order: metadata -> events -> taps -> holds
		uint32_t offsetsAddress = writer.getStreamPosition();
		writer.writeNull(sizeof(uint32_t) * 4);

		uint32_t metadataAddress = writer.getStreamPosition();
		writeMetadata(score.metadata, &writer);

		uint32_t eventsAddress = writer.getStreamPosition();
		writeScoreEvents(score, &writer);

		uint32_t tapsAddress = writer.getStreamPosition();
		writer.writeNull(sizeof(uint32_t));

		int noteCount = 0;
		for (const auto&[id, note] : score.notes)
		{
			if (note.getType() != NoteType::Tap)
				continue;

			writeNote(note, &writer);
			++noteCount;
		}
		
		uint32_t holdsAddress = writer.getStreamPosition();
		
		// write taps count
		writer.seek(tapsAddress);
		writer.writeInt32(noteCount);
		writer.seek(holdsAddress);
		
		writer.writeInt32(score.holdNotes.size());
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

		// write offset addresses
		writer.seek(offsetsAddress);
		writer.writeInt32(metadataAddress);
		writer.writeInt32(eventsAddress);
		writer.writeInt32(tapsAddress);
		writer.writeInt32(holdsAddress);

		writer.flush();
		writer.close();
	}
}
