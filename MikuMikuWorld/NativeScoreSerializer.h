#pragma once

#include "ScoreSerializer.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"

namespace MikuMikuWorld
{
	class NativeScoreSerializer : public ScoreSerializer
	{
	private:
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

		Note readNote(NoteType type, IO::BinaryReader* reader);
		void writeNote(const Note& note, IO::BinaryWriter* writer);
		ScoreMetadata readMetadata(IO::BinaryReader* reader, int version);
		void writeMetadata(const ScoreMetadata& metadata, IO::BinaryWriter* writer);
		void readScoreEvents(Score& score, int version, IO::BinaryReader* reader);
		void writeScoreEvents(const Score& score, IO::BinaryWriter* writer);

	public:
		static const int SCORE_VERSION;
		void serialize(Score score, std::string filename) override;
		Score deserialize(std::string filename) override;
	};
}