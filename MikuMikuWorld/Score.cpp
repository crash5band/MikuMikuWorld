#include "Score.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "Constants.h"
#include "File.h"
#include "IO.h"
#include <unordered_set>

using namespace IO;

namespace MikuMikuWorld
{
	enum NoteFlags
	{
		NOTE_CRITICAL = 1 << 0,
		NOTE_FRICTION = 1 << 1
	};

	enum HoldFlags
	{
		HOLD_START_HIDDEN = 1 << 0,
		HOLD_END_HIDDEN = 1 << 1,
		HOLD_GUIDE = 1 << 2
	};

	Score::Score()
	{
		metadata.title = "";
		metadata.author = "";
		metadata.artist = "";
		metadata.musicOffset = 0;

		tempoChanges.push_back(Tempo());
		timeSignatures[0] = { 0, 4, 4 };
		auto id = nextHiSpeedID++;
		hiSpeedChanges[id] = HiSpeedChange{ id, 0, 1.0f, 0 };

		fever.startTick = fever.endTick = -1;
	}

	Note readNote(NoteType type, BinaryReader* reader, int cyanvasVersion)
	{
		// printf("%d\n", cyanvasVersion);

		Note note(type);

		if (cyanvasVersion <= 5)
		{
			note.tick = reader->readInt32();
			note.lane = (float)reader->readInt32();
			note.width = (float)reader->readInt32();
		}
		else
		{
			note.tick = reader->readInt32();
			note.lane = reader->readSingle();
			note.width = reader->readSingle();
		}

		if (cyanvasVersion >= 4)
			note.layer = reader->readInt32();

		if (!note.hasEase())
			note.flick = (FlickType)reader->readInt32();

		unsigned int flags = reader->readInt32();
		note.critical = (bool)(flags & NOTE_CRITICAL);
		note.friction = (bool)(flags & NOTE_FRICTION);
		return note;
	}

	void writeNote(const Note& note, BinaryWriter* writer)
	{
		writer->writeInt32(note.tick);
		writer->writeSingle(note.lane);
		writer->writeSingle(note.width);
		writer->writeInt32(note.layer);

		if (!note.hasEase())
			writer->writeInt32((int)note.flick);

		unsigned int flags{};
		if (note.critical)
			flags |= NOTE_CRITICAL;
		if (note.friction)
			flags |= NOTE_FRICTION;
		writer->writeInt32(flags);
	}

	ScoreMetadata readMetadata(BinaryReader* reader, int version, int cyanvasVersion)
	{
		ScoreMetadata metadata;
		metadata.title = reader->readString();
		metadata.author = reader->readString();
		metadata.artist = reader->readString();
		metadata.musicFile = reader->readString();
		metadata.musicOffset = reader->readSingle();

		if (version > 1)
			metadata.jacketFile = reader->readString();

		if (cyanvasVersion >= 1)
			metadata.laneExtension = reader->readInt32();

		return metadata;
	}

	void writeMetadata(const ScoreMetadata& metadata, BinaryWriter* writer)
	{
		writer->writeString(metadata.title);
		writer->writeString(metadata.author);
		writer->writeString(metadata.artist);
		writer->writeString(metadata.musicFile);
		writer->writeSingle(metadata.musicOffset);
		writer->writeString(metadata.jacketFile);
		writer->writeInt32(metadata.laneExtension);
	}

	void readScoreEvents(Score& score, int version, int cyanvasVersion, BinaryReader* reader)
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
				int layer = 0;
				if (cyanvasVersion >= 4)
					layer = reader->readInt32();
				int id = nextHiSpeedID++;
				score.hiSpeedChanges[id] = HiSpeedChange{ id, tick, speed, layer };
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
		for (const auto& [_, hiSpeed] : score.hiSpeedChanges)
		{
			writer->writeInt32(hiSpeed.tick);
			writer->writeSingle(hiSpeed.speed);
			writer->writeInt32(hiSpeed.layer);
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
		if (signature != "MMWS" && signature != "CCMMWS")
			throw std::runtime_error("Not a MMWS file.");

		bool isCyanvas = signature == "CCMMWS";

		int version = reader.readInt16();
		int cyanvasVersion = reader.readInt16();
		if (isCyanvas && cyanvasVersion == 0)
		{
			cyanvasVersion = 1;
		}

		uint32_t metadataAddress{};
		uint32_t eventsAddress{};
		uint32_t tapsAddress{};
		uint32_t holdsAddress{};
		uint32_t damagesAddress{};
		uint32_t layersAddress{};
		uint32_t waypointsAddress{};
		if (version > 2)
		{
			metadataAddress = reader.readInt32();
			eventsAddress = reader.readInt32();
			tapsAddress = reader.readInt32();
			holdsAddress = reader.readInt32();
			if (isCyanvas)
				damagesAddress = reader.readInt32();
			if (cyanvasVersion >= 4)
				layersAddress = reader.readInt32();
			if (cyanvasVersion >= 5)
				waypointsAddress = reader.readInt32();

			reader.seek(metadataAddress);
		}

		score.metadata = readMetadata(&reader, version, cyanvasVersion);

		if (version > 2)
			reader.seek(eventsAddress);

		readScoreEvents(score, version, cyanvasVersion, &reader);

		if (version > 2)
			reader.seek(tapsAddress);

		int noteCount = reader.readInt32();
		score.notes.reserve(noteCount);
		for (int i = 0; i < noteCount; ++i)
		{
			Note note = readNote(NoteType::Tap, &reader, cyanvasVersion);
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

			unsigned int flags{};
			if (version > 3)
				flags = reader.readInt32();

			if (flags & HOLD_START_HIDDEN)
				hold.startType = HoldNoteType::Hidden;

			if (flags & HOLD_END_HIDDEN)
				hold.endType = HoldNoteType::Hidden;

			if (flags & HOLD_GUIDE)
				hold.startType = hold.endType = HoldNoteType::Guide;

			Note start = readNote(NoteType::Hold, &reader, cyanvasVersion);
			start.ID = nextID++;
			hold.start.ease = (EaseType)reader.readInt32();
			hold.start.ID = start.ID;
			if (cyanvasVersion >= 2)
			{
				hold.fadeType = (FadeType)reader.readInt32();
			}
			if (cyanvasVersion >= 3)
			{
				hold.guideColor = (GuideColor)reader.readInt32();
			}
			else
			{
				hold.guideColor = start.critical ? GuideColor::Yellow : GuideColor::Green;
			}
			score.notes[start.ID] = start;

			int stepCount = reader.readInt32();
			hold.steps.reserve(stepCount);
			for (int i = 0; i < stepCount; ++i)
			{
				Note mid = readNote(NoteType::HoldMid, &reader, cyanvasVersion);
				mid.ID = nextID++;
				mid.parentID = start.ID;
				score.notes[mid.ID] = mid;

				HoldStep step{};
				step.type = (HoldStepType)reader.readInt32();
				step.ease = (EaseType)reader.readInt32();
				step.ID = mid.ID;
				hold.steps.push_back(step);
			}

			Note end = readNote(NoteType::HoldEnd, &reader, cyanvasVersion);
			end.ID = nextID++;
			end.parentID = start.ID;
			score.notes[end.ID] = end;

			hold.end = end.ID;
			score.holdNotes[start.ID] = hold;
		}

		if (cyanvasVersion >= 1)
		{
			reader.seek(damagesAddress);

			int damageCount = reader.readInt32();
			score.notes.reserve(damageCount);
			for (int i = 0; i < damageCount; ++i)
			{
				Note note = readNote(NoteType::Damage, &reader, cyanvasVersion);
				note.ID = nextID++;
				score.notes[note.ID] = note;
			}
		}

		if (cyanvasVersion >= 4)
		{
			score.layers.clear();
			reader.seek(layersAddress);

			int layerCount = reader.readInt32();
			score.layers.reserve(layerCount);
			for (int i = 0; i < layerCount; ++i)
			{
				std::string name = reader.readString();
				score.layers.push_back({ name });
			}
		}

		if (cyanvasVersion >= 5)
		{
			score.waypoints.clear();
			reader.seek(waypointsAddress);

			int waypointCount = reader.readInt32();
			score.waypoints.reserve(waypointCount);
			for (int i = 0; i < waypointCount; ++i)
			{
				std::string name = reader.readString();
				int tick = reader.readInt32();
				score.waypoints.push_back({ name, tick });
			}
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
		writer.writeString("CCMMWS");

		// verison
		writer.writeInt16(4);
		// cyanvas version
		writer.writeInt16(6);

		// offsets address in order: metadata -> events -> taps -> holds
		// Cyanvas extension: -> damages -> layers -> waypoints
		uint32_t offsetsAddress = writer.getStreamPosition();
		writer.writeNull(sizeof(uint32_t) * 7);

		uint32_t metadataAddress = writer.getStreamPosition();
		writeMetadata(score.metadata, &writer);

		uint32_t eventsAddress = writer.getStreamPosition();
		writeScoreEvents(score, &writer);

		uint32_t tapsAddress = writer.getStreamPosition();
		writer.writeNull(sizeof(uint32_t));

		int noteCount = 0;
		for (const auto& [id, note] : score.notes)
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
		for (const auto& [id, hold] : score.holdNotes)
		{
			unsigned int flags{};
			if (hold.startType == HoldNoteType::Guide)
				flags |= HOLD_GUIDE;
			if (hold.startType == HoldNoteType::Hidden)
				flags |= HOLD_START_HIDDEN;
			if (hold.endType == HoldNoteType::Hidden)
				flags |= HOLD_END_HIDDEN;
			writer.writeInt32(flags);

			// note data
			const Note& start = score.notes.at(hold.start.ID);
			writeNote(start, &writer);
			writer.writeInt32((int)hold.start.ease);
			writer.writeInt32((int)hold.fadeType);
			writer.writeInt32((int)hold.guideColor);

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

		uint32_t damagesAddress = writer.getStreamPosition();
		writer.writeNull(sizeof(uint32_t));

		// Cyanvas extension: write damages
		int damageNoteCount = 0;
		for (const auto& [id, note] : score.notes)
		{
			if (note.getType() != NoteType::Damage)
				continue;

			writeNote(note, &writer);
			++damageNoteCount;
		}

		// Cyanvas extension: write layers
		uint32_t layersAddress = writer.getStreamPosition();

		// write damages count
		writer.seek(damagesAddress);
		writer.writeInt32(damageNoteCount);

		writer.seek(layersAddress);

		writer.writeInt32(score.layers.size());

		for (const auto& layer : score.layers)
		{
			writer.writeString(layer.name);
		}

		uint32_t waypointsAddress = writer.getStreamPosition();

		writer.writeInt32(score.waypoints.size());

		for (const auto& waypoint : score.waypoints)
		{
			writer.writeString(waypoint.name);
			writer.writeInt32(waypoint.tick);
		}
		// write offset addresses
		writer.seek(offsetsAddress);
		writer.writeInt32(metadataAddress);
		writer.writeInt32(eventsAddress);
		writer.writeInt32(tapsAddress);
		writer.writeInt32(holdsAddress);
		writer.writeInt32(damagesAddress);
		writer.writeInt32(layersAddress);
		writer.writeInt32(waypointsAddress);

		writer.flush();
		writer.close();
	}
}
