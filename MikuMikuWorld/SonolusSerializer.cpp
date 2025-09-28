#include "SonolusSerializer.h"
#include "Constants.h"
#include "IO.h"
#include "File.h"

using json = nlohmann::json;

namespace MikuMikuWorld
{
	constexpr int halfBeat = TICKS_PER_BEAT / 2;

	class IdManager
	{
		inline int64_t getID(size_t idx)
		{
			auto it = indexToID.find(idx);
			if (it == indexToID.end())
				it = indexToID.emplace_hint(indexToID.end(), idx, nextID++);
			return it->second;
		}
		inline bool hasIdx(size_t idx) const
		{
			auto it = indexToID.find(idx);
			return it != indexToID.end();
		}
		inline static std::string toRef(int64_t index)
		{
			// Basically base 10 to base 36
			if (index < 0)
				return toRef(-index).insert(0, "-");
			std::string ref;
			lldiv_t dv;
			do
			{
				dv = std::div(index, 36ll);
				ref += dv.rem < 10 ? ('0' + dv.rem) : ('a' + (dv.rem - 10));
				index = dv.quot;
			} while (dv.quot);
			return { ref.rbegin(), ref.rend() };
		}
	public:
		inline void clear() { indexToID.clear(); }
		inline std::string getStartRef() { return toRef(getID(START_INDEX)); }
		inline std::string getEndRef() { return toRef(getID(END_INDEX)); }
		inline std::string getConnectorRef(size_t index) { return toRef(getID(index)); }
		inline std::string getExistingConnectorRef(size_t index) const { return hasIdx(index) ? toRef(indexToID.at(index)) : ""; }
		inline std::string getNextRef() { return toRef(nextID++); }
		
	private:
		int64_t nextID;
		std::map<size_t, int64_t> indexToID;
		static constexpr size_t START_INDEX = size_t(-1);
		static constexpr size_t END_INDEX = size_t(-2);
	};

	struct HoldJoint
	{
		size_t entityIdx;
		EaseType ease;
	};

	struct LinkedHoldStep
	{
		enum StepType : uint8_t { Joint, Attach, EndHold };
		enum Modifier : uint8_t { None = 0, Guide = 1, Critical = 2 };

		std::string nextTail;
		StepType step;
		Modifier mod;
		EaseType ease;

	};
	
	inline static std::string getTapNoteArchetype(const Note& note)
	{
		std::string archetype = (note.critical ? "Critical" : "Normal");
		if (note.friction) archetype += "Trace";
		if (note.isFlick()) archetype += "Flick";
		if (!note.friction && !note.isFlick()) archetype += "Tap";
		archetype += "Note";
		return archetype;
	}
	
	inline static std::string getHoldNoteArchetype(const Note& note, const HoldNote& holdNote)
	{
		if (note.friction && note.isFlick())
			// Special case
			return getTapNoteArchetype(note);
		std::string archetype = (note.critical ? "Critical" : "Normal");
		archetype += "Slide";
		if (note.ID == holdNote.end)
		{
			if (holdNote.endType != HoldNoteType::Normal)
				return "IgnoredSlideTickNote";
			archetype += "End";
			if (note.friction)
				archetype += "Trace";
			if (note.isFlick())
				archetype += "Flick";
		}
		else
		{
			if (holdNote.startType != HoldNoteType::Normal)
				return "IgnoredSlideTickNote";
			if (note.friction)
				archetype += "Trace";
			else
				archetype += "Start";
		}
		archetype += "Note";
		return archetype;
	}

	inline static Sonolus::LevelDataEntity getTickNoteEntity(const Note& note, const HoldStep& step, IdManager& mgr, size_t currJoint)
	{
		std::string archetype = (note.critical ? "Critical" : "Normal");
		Sonolus::LevelDataEntity::MapDataType data = { { "#BEAT", SonolusSerializer::ticksToBeats(note.tick) } };
		switch (step.type)
		{
		case HoldStepType::Skip:
			archetype += "Attached";
			data.emplace("attach", mgr.getConnectorRef(currJoint));
			break;
		case HoldStepType::Normal:
			data.emplace("lane", SonolusSerializer::toSonolusLane(note.lane, note.width));
			data.emplace("size", SonolusSerializer::widthToSize(note.width));
			break;
		default:
		case HoldStepType::Hidden:
			archetype = "Ignored";
			data.emplace("lane", SonolusSerializer::toSonolusLane(note.lane, note.width));
			data.emplace("size", SonolusSerializer::widthToSize(note.width));
			break;
		}
		archetype += "SlideTickNote";
		return { archetype, data };
	}

	inline static void insertTickNoteBetween(const Note& headNote, const Note& tailNote, std::vector<Sonolus::LevelDataEntity>& entities, IdManager& mgr, size_t currJoint)
	{
		int startTick = (headNote.tick / halfBeat + 1) * halfBeat;
		int endTick = (tailNote.tick / halfBeat) * halfBeat;
		std::string refStr;
		if (startTick < endTick) refStr = mgr.getConnectorRef(currJoint);
		for (int tick = startTick; tick < endTick; tick += halfBeat)
			entities.push_back({ "HiddenSlideTickNote", {{ "#BEAT", SonolusSerializer::ticksToBeats(tick) }, { "attach", refStr }} });
	}

	inline static bool stringMatching(const std::string& source, const std::string_view& toMatch, size_t& offset)
	{
		if (toMatch.size() > source.size() - offset) return false;
		std::string_view source_view = std::string_view(source).substr(offset, toMatch.size());
		bool result = source_view == toMatch;
		if (result == true)
			offset += toMatch.size();
		return result;
	}

	inline static bool stringMatchAll(const std::string& source, const std::string_view& toMatch, size_t offset)
	{
		return std::string_view(source).substr(offset, toMatch.size()) == toMatch;
	}

	inline static bool isValidNotePosition(const Note& note)
	{
		return note.tick >= 0 && note.width >= MIN_NOTE_WIDTH && note.width <= MAX_NOTE_WIDTH && note.lane >= MIN_LANE && note.lane <= MAX_LANE;
	}

	inline static bool isValidHoldNotes(const std::vector<Note>& holdNotes)
	{
		if (holdNotes.size() < 2)
			return false;
		const Note& startNote = holdNotes.front();
		const Note& endNote = holdNotes.back();
		if (startNote.critical != endNote.critical || startNote.isFlick())
			return false;
		if (std::any_of(holdNotes.begin() + 1, holdNotes.end(), [startTick = startNote.tick, endTick = endNote.tick](const Note& note) { return note.tick < startTick || note.tick > endTick; }))
			return false;
		return true;
	}

	inline static bool tapNoteArchetypeToNativeNote(const Sonolus::LevelDataEntity& tapNoteEntity, Note& note)
	{
		size_t offset = 0;
		// Doing the reverse of serializating to ensure we don't parse any invalid archetype
		float beat, lane, size;
		if (!tapNoteEntity.tryGetDataValue("#BEAT", beat)
			|| !tapNoteEntity.tryGetDataValue("lane", lane)
			|| !tapNoteEntity.tryGetDataValue("size", size))
			return false;
		note.tick = SonolusSerializer::beatsToTicks(beat);
		note.width = SonolusSerializer::sizeToWidth(size);
		note.lane = SonolusSerializer::toNativeLane(lane, size);
		if (!isValidNotePosition(note))
			return false;
		if (stringMatching(tapNoteEntity.archetype, "Critical", offset))
			note.critical = true;
		else if (!stringMatching(tapNoteEntity.archetype, "Normal", offset))
			return false;
		if (stringMatchAll(tapNoteEntity.archetype, "TraceFlickNote", offset))
		{
			// Special case of hold end
			if (tapNoteEntity.data.find("slide") != tapNoteEntity.data.end())
				return false;
		}
		bool hasModifier = false;
		if (stringMatching(tapNoteEntity.archetype, "Trace", offset))
		{
			hasModifier = true;
			note.friction = true;
		}
		if (stringMatching(tapNoteEntity.archetype, "Flick", offset))
		{
			hasModifier = true;
			int direction;
			if (!tapNoteEntity.tryGetDataValue("direction", direction))
				return false;
			note.flick = SonolusSerializer::toNativeFlick(direction);
		}
		if (!hasModifier && !stringMatching(tapNoteEntity.archetype, "Tap", offset))
			return false;
		if (!stringMatchAll(tapNoteEntity.archetype, "Note", offset))
			return false;
		return true;
	}

	inline static bool holdStartNoteArchetypeToNativeNote(const Sonolus::LevelDataEntity& noteEntity, Note& startNote)
	{
		size_t offset = 0;
		float beat, lane, size;
		if (!noteEntity.tryGetDataValue("#BEAT", beat)
			|| !noteEntity.tryGetDataValue("lane", lane)
			|| !noteEntity.tryGetDataValue("size", size))
			return false;
		startNote.tick = SonolusSerializer::beatsToTicks(beat);
		startNote.width = SonolusSerializer::sizeToWidth(size);
		startNote.lane = SonolusSerializer::toNativeLane(lane, size);
		if (!isValidNotePosition(startNote))
			return false;
		if (noteEntity.archetype == "IgnoredSlideTickNote")
			return true;
		if (stringMatching(noteEntity.archetype, "Critical", offset))
			startNote.critical = true;
		else if (!stringMatching(noteEntity.archetype, "Normal", offset))
			return false;
		if (!stringMatching(noteEntity.archetype, "Slide", offset))
			return false;
		if (stringMatching(noteEntity.archetype, "Trace", offset))
			startNote.friction = true;
		else if (!stringMatching(noteEntity.archetype, "Start", offset))
			return false;
		if (!stringMatchAll(noteEntity.archetype, "Note", offset))
			return false;
		return true;
	}

	inline static bool holdEndNoteArchetypeToNativeNote(const Sonolus::LevelDataEntity& noteEntity, Note& endNote)
	{
		size_t offset = 0;
		int direction;
		float beat, lane, size;
		if (!noteEntity.tryGetDataValue("#BEAT", beat)
			|| !noteEntity.tryGetDataValue("lane", lane)
			|| !noteEntity.tryGetDataValue("size", size))
			return false;
		endNote.tick = SonolusSerializer::beatsToTicks(beat);
		endNote.width = SonolusSerializer::sizeToWidth(size);
		endNote.lane = SonolusSerializer::toNativeLane(lane, size);
		if (!isValidNotePosition(endNote))
			return false;
		if (noteEntity.archetype == "IgnoredSlideTickNote")
			return true;
		if (stringMatching(noteEntity.archetype, "Critical", offset))
			endNote.critical = true;
		else if (!stringMatching(noteEntity.archetype, "Normal", offset))
			return false;
		if (stringMatchAll(noteEntity.archetype, "TraceFlickNote", offset))
		{
			// Special case
			if (!noteEntity.tryGetDataValue("direction", direction))
				return false;
			endNote.flick = SonolusSerializer::toNativeFlick(direction);
			endNote.friction = true;
			return true;
		}
		if (!stringMatching(noteEntity.archetype, "SlideEnd", offset))
			return false;
		if (stringMatching(noteEntity.archetype, "Trace", offset))
			endNote.friction = true;
		if (stringMatching(noteEntity.archetype, "Flick", offset))
		{
			if (!noteEntity.tryGetDataValue("direction", direction))
				return false;
			endNote.flick = SonolusSerializer::toNativeFlick(direction);
		}
		if (!stringMatchAll(noteEntity.archetype, "Note", offset))
			return false;
		return true;
	}

	inline static bool slideTickNoteArchetypeToNativeNote(const Sonolus::LevelDataEntity& tickEntity, Note& tickNote)
	{
		bool hidden = tickEntity.archetype == "IgnoredSlideTickNote";
		size_t offset = 0;
		float beat, lane, size;
		if (stringMatching(tickEntity.archetype, "Critical", offset))
			tickNote.critical = true;
		else if (!hidden && !stringMatching(tickEntity.archetype, "Normal", offset))
			return false;
		if (hidden || stringMatchAll(tickEntity.archetype, "SlideTickNote", offset))
		{
			if (!tickEntity.tryGetDataValue("#BEAT", beat)
				|| !tickEntity.tryGetDataValue("lane", lane)
				|| !tickEntity.tryGetDataValue("size", size))
				return false;
			tickNote.tick = SonolusSerializer::beatsToTicks(beat);
			tickNote.width = SonolusSerializer::sizeToWidth(size);
			tickNote.lane = SonolusSerializer::toNativeLane(lane, size);
			if (!isValidNotePosition(tickNote))
				return false;
			return true;
		}
		else if (stringMatchAll(tickEntity.archetype, "AttachedSlideTickNote", offset))
		{
			if (!tickEntity.tryGetDataValue("#BEAT", beat))
				return false;
			tickNote.tick = SonolusSerializer::beatsToTicks(beat);
			// We'll estimate lane and width later
			tickNote.lane = 0;
			tickNote.width = 1;
			return true;
		}
		return false;
	}

	void SonolusSerializer::serialize(const Score& score, std::string filename)
	{
		Sonolus::LevelData levelData = serialize(score);
		std::string serializedData = json(levelData).dump();
		std::vector<uint8_t> serializedBytes(serializedData.begin(), serializedData.end());
		if (compressData)
			serializedBytes = IO::deflateGzip(serializedBytes);

		IO::File levelFile(filename, IO::FileMode::WriteBinary);
		levelFile.writeAllBytes(serializedBytes);
		levelFile.flush();
		levelFile.close();
	}

	Sonolus::LevelData SonolusSerializer::serialize(const Score& score)
	{
		Sonolus::LevelData levelData;
		levelData.bgmOffset = score.metadata.musicOffset / -1000.0;
		levelData.entities.emplace_back("Initialization");
		levelData.entities.emplace_back("Stage");

		for (const auto& speed : score.hiSpeedChanges)
		{
			levelData.entities.push_back({
				"#TIMESCALE_CHANGE",
				{
					{ "#BEAT",      ticksToBeats(speed.tick) },
					{ "#TIMESCALE", speed.speed              }
				}
				});
		}

		for (const auto& bpm : score.tempoChanges)
		{
			levelData.entities.push_back({
				"#BPM_CHANGE",
				{
					{ "#BEAT", ticksToBeats(bpm.tick) },
					{ "#BPM", bpm.bpm }
				}
				});
		}

		std::multimap<decltype(Note::tick), size_t> simBuilder;
		for (const auto& [id, note] : score.notes)
		{
			if (note.getType() != NoteType::Tap) continue;
			Sonolus::LevelDataEntity::MapDataType data = {
				{ "#BEAT", ticksToBeats(note.tick)              },
				{ "lane", toSonolusLane(note.lane, note.width)  },
				{ "size", widthToSize(note.width) }
			};

			if (note.isFlick())
				data.emplace("direction", toSonolusDirection(note.flick));

			simBuilder.emplace(note.tick, levelData.entities.size());
			levelData.entities.push_back({
				getTapNoteArchetype(note),
				std::move(data)
				});
		}

		IdManager idMgr;
		std::vector<HoldJoint> joints;
		for (const auto& [id, hold] : score.holdNotes)
		{
			idMgr.clear();
			joints.clear();
			// First we insert the hold intermediates
			const Note& startNote = score.notes.at(hold.start.ID), & endNote = score.notes.at(hold.end);
			const Note* headNote = &startNote;
			joints.push_back({ levelData.entities.size(), hold.start.ease });
			if (hold.startType == HoldNoteType::Normal)
				simBuilder.emplace(startNote.tick, levelData.entities.size());
			levelData.entities.push_back({
				getHoldNoteArchetype(startNote, hold),
				{
					{ "#BEAT", ticksToBeats(startNote.tick)                  },
					{ "lane", toSonolusLane(startNote.lane, startNote.width) },
					{ "size", widthToSize(startNote.width)                   }
				}
				});
			for (auto& tailStep : hold.steps)
			{
				const Note& tailNote = score.notes.at(tailStep.ID);
				if (!hold.isGuide()) insertTickNoteBetween(*headNote, tailNote, levelData.entities, idMgr, joints.size());
				headNote = &tailNote;
				if (tailStep.type != HoldStepType::Skip) joints.push_back({ levelData.entities.size(), tailStep.ease });

				levelData.entities.push_back(getTickNoteEntity(tailNote, tailStep, idMgr, joints.size()));
				if (!hold.isGuide() && (tailNote.tick % halfBeat == 0))
					levelData.entities.push_back({ "HiddenSlideTickNote", {{ "#BEAT", ticksToBeats(tailNote.tick) }, { "attach", idMgr.getConnectorRef(joints.size()) }} });
			}
			if (!hold.isGuide()) insertTickNoteBetween(*headNote, endNote, levelData.entities, idMgr, joints.size());
			Sonolus::LevelDataEntity::MapDataType endData = {
				{ "#BEAT", ticksToBeats(endNote.tick)                },
				{ "lane", toSonolusLane(endNote.lane, endNote.width) },
				{ "size", widthToSize(endNote.width)                 },
			};
			if (endNote.isFlick())
				endData.emplace("direction", toSonolusDirection(endNote.flick));
			if (hold.endType == HoldNoteType::Normal)
				endData.emplace("slide", idMgr.getConnectorRef(joints.size()));
			joints.push_back({ levelData.entities.size(), EaseType::EaseTypeCount });
			if (hold.endType == HoldNoteType::Normal)
				simBuilder.emplace(endNote.tick, levelData.entities.size());
			levelData.entities.emplace_back(getHoldNoteArchetype(endNote, hold), std::move(endData));

			// Then we insert the hold connectors
			std::string connArchetype = (startNote.critical ? "Critical" : "Normal");
			connArchetype += !hold.isGuide() ? "ActiveSlideConnector" : "SlideConnector";
			std::string startRef = idMgr.getStartRef(), endRef = idMgr.getEndRef();
			std::string headRef, tailRef = startRef;
			for (size_t connHeadIdx = 0, connTailIdx = 1; connTailIdx < joints.size(); ++connHeadIdx, ++connTailIdx)
			{
				headRef = std::move(tailRef);
				tailRef = (connTailIdx == joints.size() - 1) ? idMgr.getEndRef() : idMgr.getNextRef();
				std::string connectorRef = idMgr.getExistingConnectorRef(connTailIdx);
				const auto& headJoint = joints[connHeadIdx];
				levelData.entities[headJoint.entityIdx].name = headRef;
				levelData.entities[joints[connTailIdx].entityIdx].name = tailRef;
				int connEase = toSonolusEase(headJoint.ease);
				Sonolus::LevelDataEntity::MapDataType connData = {
					{ "start", startRef },
					{ "end",     endRef },
					{ "head",   headRef },
					{ "tail",   tailRef },
					{ "ease",  connEase }
				};
				levelData.entities.emplace_back(connectorRef, connArchetype, connData);
			}
		}

		for (auto it = simBuilder.begin(), end = simBuilder.end(); it != end; )
		{
			auto [startEnt, endEnt] = simBuilder.equal_range(it->first);

			for (auto aEnt = startEnt, bEnt = std::next(startEnt); bEnt != endEnt; ++aEnt, ++bEnt)
			{
				auto& a = levelData.entities[aEnt->second], & b = levelData.entities[bEnt->second];
				if (a.name.empty())
					a.name = idMgr.getNextRef();
				if (b.name.empty())
					b.name = idMgr.getNextRef();
				levelData.entities.push_back({ "SimLine", {{"a", a.name}, {"b", b.name}} });
			}
			it = endEnt;
		}

		return levelData;
	}

	Score SonolusSerializer::deserialize(std::string filename)
	{
		if (!IO::File::exists(filename.c_str()))
			return {};
		IO::File levelFile(filename, IO::FileMode::ReadBinary);
		std::vector<uint8_t> bytes = levelFile.readAllBytes();
		levelFile.close();
		if (IO::isGzipCompressed(bytes))
			bytes = IO::inflateGzip(bytes);
		json levelDataJson = json::parse(std::string(bytes.begin(), bytes.end()));
		Sonolus::LevelData levelData;
		levelDataJson.get_to(levelData);
		return deserialize(levelData);
	}

	Score SonolusSerializer::deserialize(const Sonolus::LevelData& levelData)
	{
		Score score;
		score.metadata.musicOffset = levelData.bgmOffset * -1000.0;

		auto isTimescaleEntity = [](const Sonolus::LevelDataEntity& ent) { return ent.archetype == "#TIMESCALE_CHANGE"; };
		auto isBpmChangeEntity = [](const Sonolus::LevelDataEntity& ent) { return ent.archetype == "#BPM_CHANGE"; };
		auto isTapNoteEntity = [](const Sonolus::LevelDataEntity& ent) { return IO::endsWith(ent.archetype, "Note") && ent.archetype.find("Slide") == std::string::npos; };
		auto isSlideConnectorEntity = [](const Sonolus::LevelDataEntity& ent) { return IO::endsWith(ent.archetype, "SlideConnector"); };
		auto isAttachedTickNote = [](const Sonolus::LevelDataEntity& ent) { return IO::endsWith(ent.archetype, "AttachedSlideTickNote"); };

		for (const auto& timescaleEntity : levelData.entities)
		{
			if (!isTimescaleEntity(timescaleEntity)) continue;
			float beat = 0, speed = 1;
			if (timescaleEntity.tryGetDataValue("#BEAT", beat) && timescaleEntity.tryGetDataValue("#TIMESCALE", speed))
				score.hiSpeedChanges.push_back({ beatsToTicks(beat), speed });
		}
		std::sort(score.hiSpeedChanges.begin(), score.hiSpeedChanges.end(), [](const HiSpeedChange& sp1, const HiSpeedChange& sp2) { return sp1.tick < sp2.tick; });

		score.tempoChanges.clear();
		for (const auto& bpmChangeEntity : levelData.entities)
		{
			if (!isBpmChangeEntity(bpmChangeEntity)) continue;
			float beat, bpm;
			if (bpmChangeEntity.tryGetDataValue("#BEAT", beat) && bpmChangeEntity.tryGetDataValue("#BPM", bpm))
				score.tempoChanges.push_back({ beatsToTicks(beat), bpm });
		}
		if (score.tempoChanges.empty())
			score.tempoChanges.push_back(Tempo{});
		std::sort(score.tempoChanges.begin(), score.tempoChanges.end(), [](const Tempo& t1, const Tempo& t2) { return t1.tick < t2.tick; });

		for (const auto& tapNoteEntity : levelData.entities)
		{
			if (!isTapNoteEntity(tapNoteEntity)) continue;
			Note note(NoteType::Tap);
			if (!tapNoteArchetypeToNativeNote(tapNoteEntity, note))
				continue;
			note.ID = nextID;
			score.notes[nextID++] = note;
		}

		std::unordered_map<std::string, size_t> entityNameMap;
		for (size_t i = 0; i < levelData.entities.size(); ++i)
		{
			const auto& entity = levelData.entities[i];
			if (!entity.name.empty())
				entityNameMap.emplace(entity.name, i);
		}
		// We'll use this as a linked list to walk forward and collect all the hold steps
		std::unordered_map<std::string, LinkedHoldStep> headToTailMap;
		// This is where we'll start building the hold note
		std::unordered_set<std::string> startRefSet;

		for (const auto& connectorEntity : levelData.entities)
		{
			if (!isSlideConnectorEntity(connectorEntity)) continue;
			size_t offset = 0;
			bool critical = false, active = false;
			// Validate connector archetype
			if (stringMatching(connectorEntity.archetype, "Critical", offset))
				critical = true;
			else if (!stringMatching(connectorEntity.archetype, "Normal", offset))
				continue;
			if (stringMatching(connectorEntity.archetype, "Active", offset))
				active = true;
			if (!stringMatchAll(connectorEntity.archetype, "SlideConnector", offset))
				continue;

			std::string startRef, endRef, headRef, tailRef;
			int connEase;
			if (!connectorEntity.tryGetDataValue("start", startRef) || startRef.empty()
				|| !connectorEntity.tryGetDataValue("end", endRef) || endRef.empty()
				|| !connectorEntity.tryGetDataValue("head", headRef) || headRef.empty()
				|| !connectorEntity.tryGetDataValue("tail", tailRef) || tailRef.empty()
				|| !connectorEntity.tryGetDataValue("ease", connEase))
				continue;

			startRefSet.insert(startRef);
			LinkedHoldStep::Modifier mod = static_cast<LinkedHoldStep::Modifier>((active ? LinkedHoldStep::None : LinkedHoldStep::Guide) | (critical ? LinkedHoldStep::Critical : LinkedHoldStep::None));
			LinkedHoldStep endStep = { "", LinkedHoldStep::EndHold, mod };
			LinkedHoldStep endStepInserted = headToTailMap.try_emplace(endRef, endStep).first->second;

			// Check if the connector is consistent with other connector we'd found
			if (endStep.mod != endStepInserted.mod)
				continue;

			headToTailMap[headRef] = { tailRef, LinkedHoldStep::Joint, LinkedHoldStep::None, toNativeEase(connEase) };
		}

		IdManager idMgr;
		// Since the connector already tell us where to find each joint of the hold note
		// The only thing missing from the holds are attached notes (or HoldStepType::Skip)
		for (size_t i = 0; i < levelData.entities.size(); ++i)
		{
			auto& attachTickEntity = levelData.entities[i];
			if (!isAttachedTickNote(attachTickEntity)) continue;
			size_t offset = 0;
			std::string connRef, headRef;
			if (!stringMatching(attachTickEntity.archetype, "Critical", offset) && !stringMatching(attachTickEntity.archetype, "Normal", offset))
				continue;
			if (!stringMatchAll(attachTickEntity.archetype, "AttachedSlideTickNote", offset))
				continue;
			if (!attachTickEntity.tryGetDataValue("attach", connRef))
				continue;
			auto entIt = entityNameMap.find(connRef);
			if (entIt == entityNameMap.end())
				continue;
			const auto& connectorEntity = levelData.entities[entIt->second];
			if (!connectorEntity.tryGetDataValue("head", headRef))
				continue;
			auto headIt = headToTailMap.find(headRef);
			if (headIt == headToTailMap.end())
				continue;
			std::string attachName;
			if (attachTickEntity.name.empty())
			{
				attachName = "__mmw_attach__" + idMgr.getNextRef();
				entityNameMap[attachName] = i;
			}
			else
				attachName = attachTickEntity.name;
			// Insert the attach note between the head and tail
			headToTailMap[attachName] = LinkedHoldStep{ headIt->second.nextTail , LinkedHoldStep::Attach };
			headIt->second.nextTail = attachName;
		}

		for (auto& startRef : startRefSet)
		{
			HoldNote holdNote;
			std::vector<Note> holdNotes;

			std::string stepRef = startRef;
			HoldStep* step = &holdNote.start;
			NoteType nextNoteType = NoteType::Hold;
			auto toNativeNote = holdStartNoteArchetypeToNativeNote;
			bool validHold = false;

			while (true)
			{
				auto nodeStep = headToTailMap.extract(stepRef);
				if (nodeStep.empty())
					// either the next step was never inserted
					// or we have visited this step before
					break;
				const LinkedHoldStep& linkedStep = nodeStep.mapped();
				auto it = entityNameMap.find(stepRef);
				if (it == entityNameMap.end())
					break;
				auto& holdStepEntity = levelData.entities[it->second];
				if (linkedStep.step == LinkedHoldStep::EndHold)
				{
					Note& endNote = holdNotes.emplace_back(NoteType::HoldEnd);
					endNote.ID = nextID + static_cast<int>(holdNotes.size()) - 1;
					endNote.parentID = holdNotes.front().ID;
					holdNote.end = endNote.ID;
					if (!holdEndNoteArchetypeToNativeNote(holdStepEntity, endNote))
						break;
					if (holdStepEntity.archetype == "IgnoredSlideTickNote")
						holdNote.endType = HoldNoteType::Hidden;
					if ((linkedStep.mod & LinkedHoldStep::Guide))
					{
						holdNote.startType = HoldNoteType::Guide;
						holdNote.endType = HoldNoteType::Guide;
					}
					if ((linkedStep.mod & LinkedHoldStep::Critical))
					{
						for (auto& note : holdNotes)
							note.critical = true;
						endNote.critical = true;
					}

					validHold = true;
					break;
				}
				if (step == nullptr) step = &holdNote.steps.emplace_back();
				Note& stepNote = holdNotes.emplace_back(nextNoteType);
				stepNote.ID = nextID + static_cast<int>(holdNotes.size()) - 1;
				stepNote.parentID = holdNotes.front().ID;
				if (!toNativeNote(holdStepEntity, stepNote))
					break;
				switch (linkedStep.step)
				{
				case LinkedHoldStep::Joint:
					step->ease = linkedStep.ease;
					step->ID = stepNote.ID;
					step->type = HoldStepType::Normal;
					break;
				case LinkedHoldStep::Attach:
					step->ID = stepNote.ID;
					step->type = HoldStepType::Skip;
					break;
				}
				if (holdStepEntity.archetype == "IgnoredSlideTickNote")
					if (stepNote.ID == holdNotes.front().ID)
						holdNote.startType = HoldNoteType::Hidden;
					else
						step->type = HoldStepType::Hidden;

				nextNoteType = NoteType::HoldMid;
				toNativeNote = slideTickNoteArchetypeToNativeNote;
				stepRef = linkedStep.nextTail;
				step = nullptr;
			}
			if (validHold && isValidHoldNotes(holdNotes))
			{
				holdNotes.front().parentID = -1;
				for (auto& note : holdNotes) score.notes.emplace(note.ID, note);
				nextID += holdNotes.size();
				auto&& [it, _] = score.holdNotes.emplace(holdNotes.front().ID, std::move(holdNote));
				auto& insHoldNote = it->second;
				sortHoldSteps(score, insHoldNote);

				// Estimating the position of skip steps
				int lastJointIdx = -1, nextJointIdx = -1;
				for (unsigned i = 0; i < insHoldNote.steps.size(); ++i)
				{
					if (insHoldNote.steps[i].type != HoldStepType::Skip)
					{
						lastJointIdx = i;
						continue;
					}
					Note& stepNote = score.notes[insHoldNote.steps[i].ID];
					const Note& headNote = lastJointIdx < 0 ? score.notes[insHoldNote.start.ID] : score.notes[insHoldNote.steps[lastJointIdx].ID];
					if (nextJointIdx < int(i))
					{
						auto it = std::find_if(insHoldNote.steps.begin() + i + 1, insHoldNote.steps.end(), [](const HoldStep& step) { return step.type != HoldStepType::Skip; });
						if (it == insHoldNote.steps.end())
							nextJointIdx = insHoldNote.steps.size();
						else
							nextJointIdx = std::distance(insHoldNote.steps.begin(), it);
					}
					const Note& tailNote = nextJointIdx == insHoldNote.steps.size() ? score.notes[insHoldNote.end] : score.notes[insHoldNote.steps[nextJointIdx].ID];

					stepNote.width = std::min(headNote.width, tailNote.width);
					stepNote.lane = headNote.lane >= 6 ? headNote.lane + headNote.width - stepNote.width : headNote.lane;
				}
			}
		}

		return score;
	}

	SonolusSerializer::RealType SonolusSerializer::ticksToBeats(int ticks, int beatTicks)
	{
		return static_cast<RealType>(ticks) / beatTicks;
	}

	SonolusSerializer::RealType SonolusSerializer::widthToSize(int width)
	{
		return static_cast<RealType>(width) / 2;
	}

	SonolusSerializer::RealType SonolusSerializer::toSonolusLane(int lane, int width)
	{
		return (lane - 6) + (static_cast<RealType>(width) / 2);
	}

	int SonolusSerializer::toSonolusDirection(FlickType flick)
	{
		switch (flick)
		{
		case MikuMikuWorld::FlickType::Left:
			return -1;
		case MikuMikuWorld::FlickType::Right:
			return 1;
		default:
			return 0;
		}
	}

	int SonolusSerializer::toSonolusEase(EaseType ease)
	{
		switch (ease)
		{
		case EaseType::EaseIn:
			return 1;
		case EaseType::EaseOut:
			return -1;
		default:
			return 0;
		}
	}

	int SonolusSerializer::beatsToTicks(RealType beats, int beatTicks)
	{
		return std::lround(beats * beatTicks);
	}

	int SonolusSerializer::sizeToWidth(RealType size)
	{
		return size * 2;
	}

	int SonolusSerializer::toNativeLane(RealType lane, RealType size)
	{
		return lane - size + 6;
	}

	FlickType SonolusSerializer::toNativeFlick(int direction)
	{
		switch (direction)
		{
		case 0:
			return FlickType::Default;
		case 1:
			return FlickType::Right;
		case -1:
			return FlickType::Left;
		default:
			return FlickType::None;
		}
	}

	EaseType SonolusSerializer::toNativeEase(int ease)
	{
		switch (ease)
		{
		case -1:
			return EaseType::EaseOut;
		case 1:
			return EaseType::EaseIn;
		default:
			return EaseType::Linear;
		}
	}
}