#include "SonolusSerializer.h"
#include "Constants.h"
#include "IO.h"
#include "File.h"
#include "Application.h"
#include "ApplicationConfiguration.h"
#include "Colors.h"

#ifdef _DEBUG
#define PRINT_DEBUG(...) \
	do { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while (0)
#else
#define PRINT_DEBUG(...) \
	do { } while (0)
#endif // _DEBUG

using json = nlohmann::json;
using namespace Sonolus;

namespace MikuMikuWorld
{
	constexpr int halfBeat = TICKS_PER_BEAT / 2;

	void SonolusSerializer::serialize(const Score& score, std::string filename)
	{
		LevelData levelData = engine->serialize(score);
		std::string serializedData = json(levelData).dump(prettyDump ? 2 : -1);
		std::vector<uint8_t> serializedBytes(serializedData.begin(), serializedData.end());
		if (compressData)
			serializedBytes = IO::deflateGzip(serializedBytes);

		IO::File levelFile(filename, IO::FileMode::WriteBinary);
		levelFile.writeAllBytes(serializedBytes);
		levelFile.flush();
		levelFile.close();
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
		LevelData levelData;
		levelDataJson.get_to(levelData);

		return engine->deserialize(levelData);
	}

#pragma region HelperFunctions

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
		inline static std::string toRef(int64_t index, int64_t base)
		{
			// Basically base 10 to base n
			if (index < 0)
				return toRef(-index, base).insert(0, "-");
			std::string ref;
			lldiv_t dv;
			do
			{
				dv = std::div(index, base);
				ref += dv.rem < 10 ? ('0' + dv.rem) : ('a' + (dv.rem - 10));
				index = dv.quot;
			} while (dv.quot);
			return { ref.rbegin(), ref.rend() };
		}
	public:
		IdManager(int64_t base = 36ll, int64_t nextID = 0) : nextID(nextID), base(base), indexToID() { }

		inline void clear() { indexToID.clear(); }
		inline std::string getStartRef() { return toRef(getID(START_INDEX), base); }
		inline std::string getEndRef() { return toRef(getID(END_INDEX), base); }
		inline std::string getRef(size_t index) { return toRef(getID(index), base); }
		inline std::string getExistingRef(size_t index) const { return hasIdx(index) ? toRef(indexToID.at(index), base) : ""; }
		inline std::string getNextRef() { return toRef(nextID++, base); }
		
	private:
		int64_t nextID, base;
		std::map<size_t, int64_t> indexToID;
		static constexpr size_t START_INDEX = size_t(-1);
		static constexpr size_t END_INDEX = size_t(-2);
	};

	struct RefFunctor
	{
		std::string ref;
		IdManager& idMgr;
		size_t index;

		RefFunctor(IdManager& idMgr, size_t index) : ref{}, idMgr(idMgr), index(index) { }

		inline std::string operator()()
		{
			if (ref.empty())
				ref = idMgr.getRef(index);
			return ref;
		}

		inline void setRef(size_t idx)
		{
			if (idx != index)
			{
				index = idx;
				ref.clear();
			}
		}
	};

	double SonolusEngine::toBgmOffset(float musicOffset)
	{
		return musicOffset == 0 ? 0.0 : roundOff(-musicOffset / 1000.0);
	}

	LevelDataEntity SonolusEngine::toBpmChangeEntity(const Tempo &tempo)
	{
		return {
			"#BPM_CHANGE",
			{
				{ "#BEAT", ticksToBeats(tempo.tick) },
				{ "#BPM", tempo.bpm }
			}
		};
	}

	SonolusEngine::RealType SonolusEngine::ticksToBeats(TickType ticks, TickType beatTicks)
	{
		return roundOff(static_cast<RealType>(ticks) / beatTicks);
	}

	SonolusEngine::RealType SonolusEngine::widthToSize(int width)
	{
		return static_cast<RealType>(width) / 2;
	}

	SonolusEngine::RealType SonolusEngine::toSonolusLane(int lane, int width)
	{
		return (lane - 6) + (static_cast<RealType>(width) / 2);
	}

	float SonolusEngine::fromBgmOffset(double bgmOffset)
	{
		return bgmOffset == 0 ? 0.f : -bgmOffset * 1000;
	}

	bool SonolusEngine::fromBpmChangeEntity(const Sonolus::LevelDataEntity &bpmChangeEntity, Tempo &tempo)
	{
		float beat;
		if (!bpmChangeEntity.tryGetDataValue("#BEAT", beat))
		{
			PRINT_DEBUG("Missing #BEAT key on #BPM_CHANGE");
			return false;
		}
		tempo.tick = beatsToTicks(beat);
		if (!bpmChangeEntity.tryGetDataValue("#BPM", tempo.bpm))
		{
			PRINT_DEBUG("Missing #BPM key on #BPM_CHANGE");
			return false;
		}
		return true;
	}

	SonolusEngine::TickType SonolusEngine::beatsToTicks(RealType beats, TickType beatTicks)
	{
		return std::lround(beats * beatTicks);
	}

	int SonolusEngine::sizeToWidth(RealType size)
	{
		return size * 2;
	}

	int SonolusEngine::toNativeLane(RealType lane, RealType size)
	{
		return lane - size + 6;
	}

	bool SonolusEngine::isValidNoteState(const Note &note)
	{
		return note.tick >= 0
			&& note.width >= MIN_NOTE_WIDTH
			&& note.width <= MAX_NOTE_WIDTH
			&& note.lane >= MIN_LANE
			&& note.lane <= MAX_LANE;
	}

	bool SonolusEngine::isValidHoldNotes(const std::vector<Note>& holdNotes)
	{
		if (holdNotes.size() < 2)
		{
			PRINT_DEBUG("Hold notes missing start/end");
			return false;
		}
		const Note& startNote = holdNotes.front();
		const Note& endNote = holdNotes.back();
		if (startNote.isFlick()
		|| (startNote.critical && !endNote.critical)
		|| (!startNote.critical && endNote.critical && !endNote.friction && !endNote.isFlick()))
		{
			PRINT_DEBUG("Invalid start-end hold note configuration");
			return false;
		}
		if (std::any_of(holdNotes.begin() + 1, holdNotes.end(), [startTick = startNote.tick, endTick = endNote.tick](const Note& note) { return note.tick < startTick || note.tick > endTick; }))
		{
			PRINT_DEBUG("Tick note outside hold note range");
			return false;
		}
		return true;
	}

	std::string SonolusEngine::getTapNoteArchetype(const Note& note)
	{
		std::string archetype = (note.critical ? "Critical" : "Normal");
		if (note.friction) archetype += "Trace";
		if (note.isFlick()) archetype += "Flick";
		if (!note.friction && !note.isFlick()) archetype += "Tap";
		archetype += "Note";
		return archetype;
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

#pragma endregion

#pragma region SekaiBestEngine
	// Serialize to PJSekai engine's leveldata
	// Note: While the logic is the same, due to sorted containers and json formatting, the output might be slightly different
	LevelData SekaiBestEngine::serialize(const Score& score)
	{
		LevelData levelData;
		levelData.bgmOffset = toBgmOffset(score.metadata.musicOffset);
		levelData.entities.emplace_back("Initialization");
		levelData.entities.emplace_back("Stage");

		for (const auto& speed : score.hiSpeedChanges)
			levelData.entities.emplace_back(toSpeedChangeEntity(speed));

		for (const auto& bpm : score.tempoChanges)
			levelData.entities.emplace_back(toBpmChangeEntity(bpm));

		std::multimap<TickType, size_t> simBuilder;
		for (const auto& [id, note] : score.notes)
		{
			if (note.getType() != NoteType::Tap) continue;
			simBuilder.emplace(note.tick, levelData.entities.size());
			levelData.entities.emplace_back(toTapNoteEntity(note));
		}

		IdManager idMgr;
		std::vector<HoldJoint> joints;
		for (const auto& [id, hold] : score.holdNotes)
		{
			idMgr.clear();
			joints.clear();
			
			const Note& startNote = score.notes.at(hold.start.ID), & endNote = score.notes.at(hold.end);
			
			joints.push_back({ levelData.entities.size(), hold.start.ease });
			if (hold.startType == HoldNoteType::Normal)
				simBuilder.emplace(startNote.tick, levelData.entities.size());
			levelData.entities.emplace_back(toStartHoldNoteEntity(startNote, hold));
			
			bool isHead = true;
			RefFunctor getCurrJoint(idMgr, joints.size());
			const Note* headNote = &startNote;
			for (auto& tailStep : hold.steps)
			{
				const Note& tailNote = score.notes.at(tailStep.ID);
				if (!hold.isGuide())
				{
					insertHiddenTickNote(*headNote, tailNote, isHead, levelData.entities, getCurrJoint);
					isHead = false;
				}

				headNote = &tailNote;
				if (tailStep.type != HoldStepType::Skip)
				{
					joints.push_back({ levelData.entities.size(), tailStep.ease });
					getCurrJoint.setRef(joints.size());
				}

				levelData.entities.emplace_back(toTickNoteEntity(tailNote, tailStep, getCurrJoint));
			}
			if (!hold.isGuide())
				insertHiddenTickNote(*headNote, endNote, isHead, levelData.entities, getCurrJoint);
			joints.push_back({ levelData.entities.size(), EaseType::EaseTypeCount });
			if (hold.endType == HoldNoteType::Normal)
				simBuilder.emplace(endNote.tick, levelData.entities.size());
			levelData.entities.emplace_back(toEndHoldNoteEntity(endNote, hold, getCurrJoint));

			// Insert the hold connectors
			std::string connArchetype = (startNote.critical ? "Critical" : "Normal");
			connArchetype += !hold.isGuide() ? "ActiveSlideConnector" : "SlideConnector";
			RefType startRef = idMgr.getStartRef(), endRef = idMgr.getEndRef();
			RefType headRef, tailRef = startRef;
			for (size_t connHeadIdx = 0, connTailIdx = 1; connTailIdx < joints.size(); ++connHeadIdx, ++connTailIdx)
			{
				headRef = std::move(tailRef);
				tailRef = (connTailIdx == joints.size() - 1) ? idMgr.getEndRef() : idMgr.getNextRef();
				RefType connectorRef = idMgr.getExistingRef(connTailIdx);
				const auto& headJoint = joints[connHeadIdx];
				levelData.entities[headJoint.entityIdx].name = headRef;
				levelData.entities[joints[connTailIdx].entityIdx].name = tailRef;
				int connEase = toEaseNumeric(headJoint.ease);
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

	Score SekaiBestEngine::deserialize(const Sonolus::LevelData& levelData)
	{
		size_t errorCount = 0;
		Score score;
		score.metadata.musicOffset = fromBgmOffset(levelData.bgmOffset);
		
		auto isTimescaleEntity = [](const Sonolus::LevelDataEntity& ent) { return ent.archetype == "#TIMESCALE_CHANGE"; };
		auto isBpmChangeEntity = [](const Sonolus::LevelDataEntity& ent) { return ent.archetype == "#BPM_CHANGE"; };
		auto isTapNoteEntity = [](const Sonolus::LevelDataEntity& ent) { return IO::endsWith(ent.archetype, "Note") && ent.archetype.find("Slide") == std::string::npos; };
		auto isSlideConnectorEntity = [](const Sonolus::LevelDataEntity& ent) { return IO::endsWith(ent.archetype, "SlideConnector"); };
		auto isAttachedTickNote = [](const Sonolus::LevelDataEntity& ent) { return IO::endsWith(ent.archetype, "AttachedSlideTickNote"); };

		score.hiSpeedChanges.clear();
		for (const auto& timescaleEntity : levelData.entities)
		{
			if (!isTimescaleEntity(timescaleEntity)) continue;
			HiSpeedChange hispeed;
			if (fromSpeedChangeEntity(timescaleEntity, hispeed))
				score.hiSpeedChanges.emplace_back(std::move(hispeed));
			else
				errorCount++;
		}
		std::sort(score.hiSpeedChanges.begin(), score.hiSpeedChanges.end(), [](const HiSpeedChange& sp1, const HiSpeedChange& sp2) { return sp1.tick < sp2.tick; });

		score.tempoChanges.clear();
		for (const auto& bpmChangeEntity : levelData.entities)
		{
			if (!isBpmChangeEntity(bpmChangeEntity)) continue;
			Tempo tempo;
			if (fromBpmChangeEntity(bpmChangeEntity, tempo))
				score.tempoChanges.emplace_back(std::move(tempo));
			else
				errorCount++;
		}
		if (score.tempoChanges.empty())
			score.tempoChanges.push_back(Tempo{});
		std::sort(score.tempoChanges.begin(), score.tempoChanges.end(), [](const Tempo& t1, const Tempo& t2) { return t1.tick < t2.tick; });

		for (const auto& tapNoteEntity : levelData.entities)
		{
			if (!isTapNoteEntity(tapNoteEntity)) continue;
			Note note(NoteType::Tap);
			if (!fromTapNoteEntity(tapNoteEntity, note))
			{
				errorCount++;
				continue;
			}
			note.ID = nextID;
			score.notes[nextID++] = note;
		}

		std::unordered_map<RefType, size_t> entityNameMap;
		for (size_t i = 0; i < levelData.entities.size(); ++i)
		{
			const auto& entity = levelData.entities[i];
			if (!entity.name.empty())
				entityNameMap.emplace(entity.name, i);
		}
		// We'll use this as a linked list to walk forward and collect all the hold steps
		std::unordered_map<RefType, LinkedHoldStep> headToTailMap;
		// This is where we'll start building the hold note
		std::unordered_set<RefType> startRefSet;

		for (const auto& connectorEntity : levelData.entities)
		{
			if (!isSlideConnectorEntity(connectorEntity)) continue;
			HoldSegment segment;
			if (!fromConnectorEntity(connectorEntity, segment))
			{
				errorCount++;
				continue;	
			}

			startRefSet.insert(segment.startRef);
			LinkedHoldStep::Modifier mod = static_cast<LinkedHoldStep::Modifier>((segment.active ? LinkedHoldStep::None : LinkedHoldStep::Guide) | (segment.critical ? LinkedHoldStep::Critical : LinkedHoldStep::None));
			LinkedHoldStep endStep = { "", LinkedHoldStep::EndHold, mod };
			LinkedHoldStep endStepInserted = headToTailMap.try_emplace(segment.endRef, endStep).first->second;

			// Check if the connector is consistent with other connector we'd found
			if (endStep.mod != endStepInserted.mod)
			{
				PRINT_DEBUG("Inconsistent connector type (%s)", connectorEntity.name.c_str());
				errorCount++;
				continue;
			}

			headToTailMap[segment.headRef] = { segment.tailRef, LinkedHoldStep::Joint, LinkedHoldStep::None, fromEaseNumeric(segment.connEase) };
		}

		IdManager idMgr;
		// Since the connector already tell us where to find each joint of the hold note
		// The only thing missing from the holds are attached notes (or HoldStepType::Skip)
		for (size_t i = 0; i < levelData.entities.size(); ++i)
		{
			auto& attachTickEntity = levelData.entities[i];
			if (!isAttachedTickNote(attachTickEntity)) continue;
			RefType connRef;
			if (!fromAttachTickEntity(attachTickEntity, connRef))
			{
				errorCount++;
				continue;
			}
			auto entIt = entityNameMap.find(connRef);
			if (entIt == entityNameMap.end())
				continue;
			RefType headRef = levelData.entities[entIt->second].getDataValue<RefType>("head");
			auto headIt = headToTailMap.find(headRef);
			if (headIt == headToTailMap.end())
				continue;
			RefType attachName;
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

			RefType stepRef = startRef;
			HoldStep* step = &holdNote.start;
			NoteType nextNoteType = NoteType::Hold;
			auto toNativeNote = fromStartHoldNoteEntity;
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
					if (!fromEndHoldNoteEntity(holdStepEntity, endNote))
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
				toNativeNote = fromTickNoteEntity;
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
			else
				++errorCount;
		}
		if (errorCount)
		{
			PRINT_DEBUG("Total of %d error(s) found", errorCount);
			throw PartialScoreDeserializeError(score, IO::formatString("%s\n%s", getString("error_load_score_file"), getString("score_partially_missing")));
		}
		return score;
	}

	LevelDataEntity SekaiBestEngine::toSpeedChangeEntity(const HiSpeedChange& hispeed)
	{
		return {
			"#TIMESCALE_CHANGE",
			{
				{ "#BEAT",      ticksToBeats(hispeed.tick) },
				{ "#TIMESCALE", roundOff(hispeed.speed)    }
			}
		};
	}

	LevelDataEntity SekaiBestEngine::toTapNoteEntity(const Note& note)
	{
		LevelDataEntity tapNoteEnt = {
			SonolusEngine::getTapNoteArchetype(note),
			{
				{ "#BEAT", ticksToBeats(note.tick)              },
				{ "lane", toSonolusLane(note.lane, note.width)  },
				{ "size", widthToSize(note.width) 				}
			}
		};
		if (note.isFlick())
			tapNoteEnt.data.emplace("direction", toDirectionNumeric(note.flick));

		return tapNoteEnt;
	}

	LevelDataEntity SekaiBestEngine::toStartHoldNoteEntity(const Note &note, const HoldNote& holdNote)
	{
		std::string archetype = (note.critical ? "Critical" : "Normal");
		archetype += "Slide";
		if (holdNote.startType != HoldNoteType::Normal)
			archetype = "IgnoredSlideTick";
		else if (note.friction)
			archetype += "Trace";
		else
			archetype += "Start";
		archetype += "Note";
		return {
			std::move(archetype),
			{
				{ "#BEAT", ticksToBeats(note.tick)             },
				{ "lane", toSonolusLane(note.lane, note.width) },
				{ "size", widthToSize(note.width)              }
			}
		};
	}

	LevelDataEntity SekaiBestEngine::toEndHoldNoteEntity(const Note &note, const HoldNote &holdNote, const FuncRef& getCurrJoint)
	{
		std::string archetype = (note.critical ? "Critical" : "Normal");
		archetype += "Slide";
		LevelDataEntity::MapDataType data = {
			{ "#BEAT", ticksToBeats(note.tick)             },
			{ "lane", toSonolusLane(note.lane, note.width) },
			{ "size", widthToSize(note.width)              },
		};
		if (holdNote.endType == HoldNoteType::Normal)
		{
			archetype += "End";
			if (note.friction)
				archetype += "Trace";
			if (note.isFlick())
			{
				archetype += "Flick";
				data.emplace("direction", toDirectionNumeric(note.flick));
			}
			if (note.friction && note.isFlick())
				// Special case
				archetype = getTapNoteArchetype(note);
			data.emplace("slide", getCurrJoint());
			archetype += "Note";
		}
		else
			archetype = "IgnoredSlideTickNote";
		return { std::move(archetype), std::move(data) };
	}

	LevelDataEntity SekaiBestEngine::toTickNoteEntity(const Note &note, const HoldStep &step, const FuncRef &getCurrJoint)
	{
		std::string archetype = (note.critical ? "Critical" : "Normal");
		LevelDataEntity::MapDataType data = { { "#BEAT", ticksToBeats(note.tick) } };
		switch (step.type)
		{
		case HoldStepType::Skip:
			archetype += "Attached";
			data.emplace("attach", getCurrJoint());
			break;
		case HoldStepType::Normal:
			data.emplace("lane", toSonolusLane(note.lane, note.width));
			data.emplace("size", widthToSize(note.width));
			break;
		default:
		case HoldStepType::Hidden:
			archetype = "Ignored";
			data.emplace("lane", toSonolusLane(note.lane, note.width));
			data.emplace("size", widthToSize(note.width));
			break;
		}
		archetype += "SlideTickNote";
		return { archetype, data };
	}

	LevelDataEntity SekaiBestEngine::toHiddenTickNoteEntity(TickType tick, const RefType &ref)
	{
		return {
			"HiddenSlideTickNote",
			{
				{ "#BEAT", ticksToBeats(tick) },
				{ "attach", ref }
			}
		};
	}

	void SekaiBestEngine::insertHiddenTickNote(const Note &headNote, const Note &tailNote, bool isHead, std::vector<Sonolus::LevelDataEntity> &entities, const FuncRef &getCurrJoint)
	{
		int skips = (isHead || headNote.tick % halfBeat != 0) ? 1 : 0;
		TickType startTick = (headNote.tick / halfBeat + skips) * halfBeat;
		TickType endTick = std::ceil(RealType(tailNote.tick) / halfBeat) * halfBeat;
		for (int tick = startTick; tick < endTick; tick += halfBeat)
			entities.push_back(toHiddenTickNoteEntity(tick, getCurrJoint()));
	}

	int SekaiBestEngine::toDirectionNumeric(FlickType flick)
	{
		switch (flick)
		{
		case FlickType::Left:
			return -1;
		case FlickType::Right:
			return 1;
		case FlickType::Default:
			return 0;
		default:
			PRINT_DEBUG("Unknown FlickType");
			return 0;
		}
	}

	int SekaiBestEngine::toEaseNumeric(EaseType ease)
	{
		switch (ease)
		{
		case EaseType::EaseIn:
			return 1;
		case EaseType::EaseOut:
			return -1;
		case EaseType::Linear:
			return 0;
		default:
			PRINT_DEBUG("Unknown FlickType");
			return 0;
		}
	}

	bool SekaiBestEngine::fromSpeedChangeEntity(const Sonolus::LevelDataEntity &speedChangeEntity, HiSpeedChange &hispeed)
	{
		float beat = 0;
		if (!speedChangeEntity.tryGetDataValue("#BEAT", beat))
		{
			PRINT_DEBUG("Missing #BEAT key on #TIMESCALE_CHANGE");
			return false;
		}
		hispeed.tick = beatsToTicks(beat);
		if (!speedChangeEntity.tryGetDataValue("#TIMESCALE", hispeed.speed))
		{
			PRINT_DEBUG("Missing #TIMESCALE key on #TIMESCALE_CHANGE");
			return false;
		}
		return true;
	}

	bool SekaiBestEngine::fromNoteEntity(const Sonolus::LevelDataEntity &noteEntity, Note &note)
	{
		RealType beat, lane, size;
		if (!noteEntity.tryGetDataValue("#BEAT", beat))
		{
			PRINT_DEBUG("Missing #BEAT key on %s (%s)", noteEntity.archetype.c_str(), noteEntity.name.c_str());
			return false;	
		}
		if (!noteEntity.tryGetDataValue("lane", lane))
		{
			PRINT_DEBUG("Missing 'lane' key on %s (%s)", noteEntity.archetype.c_str(), noteEntity.name.c_str());
			return false;
		}
		if (!noteEntity.tryGetDataValue("size", size))
		{
			PRINT_DEBUG("Missing 'size' key on %s (%s)", noteEntity.archetype.c_str(), noteEntity.name.c_str());
			return false;
		}
		note.tick = beatsToTicks(beat);
		note.width = sizeToWidth(size);
		note.lane = toNativeLane(lane, size);
		if (!isValidNoteState(note))
		{
			PRINT_DEBUG("Invalid note state (t:%d, w:%d, l:%d) on %s (%s)", note.tick, note.width, note.lane, noteEntity.archetype.c_str(), noteEntity.name.c_str());
			return false;
		}
		return true;
	}

	bool SekaiBestEngine::fromTapNoteEntity(const Sonolus::LevelDataEntity &tapNoteEntity, Note &note)
	{
		if (!fromNoteEntity(tapNoteEntity, note))
			return false;

		size_t offset = 0;
		if (stringMatching(tapNoteEntity.archetype, "Critical", offset))
			note.critical = true;
		else if (!stringMatching(tapNoteEntity.archetype, "Normal", offset))
		{
			PRINT_DEBUG("Invalid note archetype %s (%s)", tapNoteEntity.archetype.c_str(), tapNoteEntity.name.c_str());
			return false;
		}
		if (stringMatchAll(tapNoteEntity.archetype, "TraceFlickNote", offset))
		{
			// Special case of hold end
			if (tapNoteEntity.data.find("slide") != tapNoteEntity.data.end())
			{
				PRINT_DEBUG("Missing 'slide' key on %s (%s)", tapNoteEntity.archetype.c_str(), tapNoteEntity.name.c_str());
				return false;
			}
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
			{
				PRINT_DEBUG("Missing 'direction' key on %s (%s)", tapNoteEntity.archetype.c_str(), tapNoteEntity.name.c_str());
				return false;
			}
			note.flick = fromDirectionNumeric(direction);
		}
		if ((!hasModifier && !stringMatching(tapNoteEntity.archetype, "Tap", offset))
		|| !stringMatchAll(tapNoteEntity.archetype, "Note", offset))
		{
			PRINT_DEBUG("Invalid note archetype %s (%s)", tapNoteEntity.archetype.c_str(), tapNoteEntity.name.c_str());
			return false;
		}
		return true;
	}

	bool SekaiBestEngine::fromConnectorEntity(const Sonolus::LevelDataEntity &connectorEntity, HoldSegment& segment)
	{
		size_t offset = 0;
		if (stringMatching(connectorEntity.archetype, "Critical", offset))
			segment.critical = true;
		else if (!stringMatching(connectorEntity.archetype, "Normal", offset))
		{
			PRINT_DEBUG("Invalid connector archetype %s (%s)", connectorEntity.archetype.c_str(), connectorEntity.name.c_str());
			return false;
		}
		if (stringMatching(connectorEntity.archetype, "Active", offset))
			segment.active = true;
		if (!stringMatchAll(connectorEntity.archetype, "SlideConnector", offset))
		{
			PRINT_DEBUG("Invalid connector archetype %s (%s)", connectorEntity.archetype.c_str(), connectorEntity.name.c_str());
			return false;
		}
		if (!connectorEntity.tryGetDataValue("ease", segment.connEase))
		{
			PRINT_DEBUG("Missing 'ease' key on %s (%s)", connectorEntity.archetype.c_str(), connectorEntity.name.c_str());
			return false;
		}
		if (!connectorEntity.tryGetDataValue("start", segment.startRef) || segment.startRef.empty())
		{
			PRINT_DEBUG("Missing 'start' key on %s (%s)", connectorEntity.archetype.c_str(), connectorEntity.name.c_str());
			return false;
		}
		if (!connectorEntity.tryGetDataValue("end", segment.endRef) || segment.endRef.empty())
		{
			PRINT_DEBUG("Missing 'end' key on %s (%s)", connectorEntity.archetype.c_str(), connectorEntity.name.c_str());
			return false;
		}
		if (!connectorEntity.tryGetDataValue("head", segment.headRef) || segment.headRef.empty())
		{
			PRINT_DEBUG("Missing 'head' key on %s (%s)", connectorEntity.archetype.c_str(), connectorEntity.name.c_str());
			return false;
		}
		if (!connectorEntity.tryGetDataValue("tail", segment.tailRef) || segment.tailRef.empty())
		{
			PRINT_DEBUG("Missing 'tail' key on %s (%s)", connectorEntity.archetype.c_str(), connectorEntity.name.c_str());
			return false;
		}
		return true;
	}

	bool SekaiBestEngine::fromAttachTickEntity(const Sonolus::LevelDataEntity &attachEntity, RefType& connectorRef)
	{
		size_t offset = 0;
		if ((!stringMatching(attachEntity.archetype, "Critical", offset) && !stringMatching(attachEntity.archetype, "Normal", offset))
		  || !stringMatchAll(attachEntity.archetype, "AttachedSlideTickNote", offset))
		{
			PRINT_DEBUG("Invalid tick note archetype %s", attachEntity.archetype.c_str());
			return false;
		}
		if (!attachEntity.tryGetDataValue("attach", connectorRef))
		{
			PRINT_DEBUG("Missing 'attach' key on %s", attachEntity.archetype.c_str());
			return false;
		}
		return true;
	}

	bool SekaiBestEngine::fromStartHoldNoteEntity(const Sonolus::LevelDataEntity &noteEntity, Note &startNote)
	{
		if (!fromNoteEntity(noteEntity, startNote))
			return false;

		size_t offset = 0;
		if (noteEntity.archetype == "IgnoredSlideTickNote")
			return true;
		if (stringMatching(noteEntity.archetype, "Critical", offset))
			startNote.critical = true;
		else if (!stringMatching(noteEntity.archetype, "Normal", offset))
		{
			PRINT_DEBUG("Invalid slide note archetype %s (%s)", noteEntity.archetype.c_str(), noteEntity.name.c_str());
			return false;
		}

		if (!stringMatching(noteEntity.archetype, "Slide", offset))
		{
			PRINT_DEBUG("Invalid slide note archetype %s (%s)", noteEntity.archetype.c_str(), noteEntity.name.c_str());
			return false;
		}

		if (stringMatching(noteEntity.archetype, "Trace", offset))
			startNote.friction = true;
		else if (!stringMatching(noteEntity.archetype, "Start", offset))
		{
			PRINT_DEBUG("Invalid slide note archetype %s (%s)", noteEntity.archetype.c_str(), noteEntity.name.c_str());
			return false;
		}

		if (!stringMatchAll(noteEntity.archetype, "Note", offset))
		{
			PRINT_DEBUG("Invalid slide note archetype %s (%s)", noteEntity.archetype.c_str(), noteEntity.name.c_str());
			return false;
		}
		return true;
	}

	bool SekaiBestEngine::fromEndHoldNoteEntity(const Sonolus::LevelDataEntity &noteEntity, Note &endNote)
	{
		if (!fromNoteEntity(noteEntity, endNote))
			return false;

		size_t offset = 0;
		int direction;
		if (noteEntity.archetype == "IgnoredSlideTickNote")
			return true;
		if (stringMatching(noteEntity.archetype, "Critical", offset))
			endNote.critical = true;
		else if (!stringMatching(noteEntity.archetype, "Normal", offset))
		{
			PRINT_DEBUG("Invalid slide end note archetype %s (%s)", noteEntity.archetype.c_str(), noteEntity.name.c_str());
			return false;
		}
		if (stringMatchAll(noteEntity.archetype, "TraceFlickNote", offset))
		{
			// Special case
			if (!noteEntity.tryGetDataValue("direction", direction))
			{
				PRINT_DEBUG("Missing 'direction' key on %s (%s)", noteEntity.archetype.c_str(), noteEntity.name.c_str());
				return false;
			}
			endNote.flick = fromDirectionNumeric(direction);
			endNote.friction = true;
			return true;
		}
		if (!stringMatching(noteEntity.archetype, "SlideEnd", offset))
		{
			PRINT_DEBUG("Invalid slide end note archetype %s (%s)", noteEntity.archetype.c_str(), noteEntity.name.c_str());
			return false;
		}
		if (stringMatching(noteEntity.archetype, "Trace", offset))
			endNote.friction = true;
		if (stringMatching(noteEntity.archetype, "Flick", offset))
		{
			if (!noteEntity.tryGetDataValue("direction", direction))
			{
				PRINT_DEBUG("Missing 'direction' key on %s (%s)", noteEntity.archetype.c_str(), noteEntity.name.c_str());
				return false;
			}
			endNote.flick = fromDirectionNumeric(direction);
		}
		if (!stringMatchAll(noteEntity.archetype, "Note", offset))
		{
			PRINT_DEBUG("Invalid slide end note archetype %s (%s)", noteEntity.archetype.c_str(), noteEntity.name.c_str());
			return false;
		}
		return true;
	}

	bool SekaiBestEngine::fromTickNoteEntity(const Sonolus::LevelDataEntity &tickEntity, Note &tickNote)
	{
		bool hidden = (tickEntity.archetype == "IgnoredSlideTickNote");
		size_t offset = 0;
		float beat, lane, size;
		if (stringMatching(tickEntity.archetype, "Critical", offset))
			tickNote.critical = true;
		else if (!hidden && !stringMatching(tickEntity.archetype, "Normal", offset))
		{
			PRINT_DEBUG("Invalid tick note archetype %s (%s)", tickEntity.archetype.c_str(), tickEntity.name.c_str());
			return false;
		}
		if (hidden || stringMatchAll(tickEntity.archetype, "SlideTickNote", offset))
		{
			if (!fromNoteEntity(tickEntity, tickNote))
				return false;
			return true;
		}
		else if (stringMatchAll(tickEntity.archetype, "AttachedSlideTickNote", offset))
		{
			if (!tickEntity.tryGetDataValue("#BEAT", beat))
			{
				PRINT_DEBUG("Missing #BEAT key on %s (%s)", tickEntity.archetype.c_str(), tickEntity.name.c_str());
				return false;	
			}
			tickNote.tick = beatsToTicks(beat);
			// We'll estimate lane and width later
			tickNote.lane = 0;
			tickNote.width = 1;
			return true;
		}
		return false;
	}

	FlickType SekaiBestEngine::fromDirectionNumeric(int direction)
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

	EaseType SekaiBestEngine::fromEaseNumeric(int ease)
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

#pragma endregion

#pragma region PysekaiEngine
	LevelData PySekaiEngine::serialize(const Score& score)
	{
		IdManager idMgr(16);
		LevelData levelData;
		levelData.bgmOffset = toBgmOffset(score.metadata.musicOffset);
		levelData.entities.emplace_back("Initialization");

		for (const auto& tempo : score.tempoChanges)
			levelData.entities.emplace_back(toBpmChangeEntity(tempo));
	
		RefType defaultGroupName = idMgr.getNextRef();
		size_t defaultGroupEntIndex = levelData.entities.size();
		size_t lastSpeedIndex = levelData.entities.size();
		levelData.entities.emplace_back(defaultGroupName, "#TIMESCALE_GROUP", LevelDataEntity::MapDataType{});

		for (const auto& speed : score.hiSpeedChanges)
		{
			RefType entName = idMgr.getNextRef();
			if (lastSpeedIndex == defaultGroupEntIndex)
				levelData.entities[defaultGroupEntIndex].data["first"] = entName;
			else
				levelData.entities[lastSpeedIndex].data["next"] = entName;
			lastSpeedIndex = levelData.entities.size();
			levelData.entities.emplace_back(toSpeedChangeEntity(speed, defaultGroupName)).name = std::move(entName);
		}

		std::multimap<TickType, size_t> simBuilder;
		for (const auto& [id, note] : score.notes)
		{
			if (note.getType() != NoteType::Tap) continue;
			simBuilder.emplace(note.tick, levelData.entities.size());
			levelData.entities.emplace_back(toNoteEntity(note, getTapNoteArchetype(note), defaultGroupName));
		}

		std::vector<size_t> entityJoints;
		std::vector<std::pair<size_t, size_t>> attachEntities;
		for (const auto& [id, hold] : score.holdNotes)
		{
			entityJoints.clear();
			attachEntities.clear();

			const Note& startNote = score.notes.at(hold.start.ID), &endNote = score.notes.at(hold.end);
			const HoldStep endStep = { hold.end, HoldStepType::Normal, EaseType::Linear };
			size_t lastEntityIndex = levelData.entities.size();
			const RealType totalSteps = hold.steps.size() + 1;

			entityJoints.push_back(lastEntityIndex);
			if (hold.startType == HoldNoteType::Normal)
				simBuilder.emplace(startNote.tick, lastEntityIndex);
			levelData.entities.emplace_back(toNoteEntity(startNote, getHoldNoteArchetype(startNote, hold), defaultGroupName, hold.startType, hold.start.type, hold.start.ease, hold.isGuide(), 1.0));

			for (size_t stepIdx = 0; stepIdx <= hold.steps.size(); ++stepIdx)
			{
				const HoldStep& step = stepIdx < hold.steps.size() ? hold.steps[stepIdx] : endStep;
				const Note& tickNote = score.notes.at(step.ID);
				double alpha = hold.isGuide() ? (1.0 - stepIdx / totalSteps) : 1.0;
				RefType entName = idMgr.getRef(tickNote.ID);
				levelData.entities[lastEntityIndex].data.emplace("next", entName);
				lastEntityIndex = levelData.entities.size();
				levelData.entities.emplace_back(toNoteEntity(tickNote, getHoldNoteArchetype(tickNote, hold), defaultGroupName, hold.startType, step.type, step.ease, hold.isGuide(), alpha)).name = std::move(entName);
				if (step.canEase())
					entityJoints.push_back(lastEntityIndex);
				else
					attachEntities.emplace_back(std::make_pair(lastEntityIndex, entityJoints.size() - 1));
			}
			if (hold.endType == HoldNoteType::Normal)
				simBuilder.emplace(endNote.tick, lastEntityIndex);
			if (!hold.isGuide())
				levelData.entities[lastEntityIndex].data.emplace("activeHead", idMgr.getRef(startNote.ID));

			RefType segStartRef = idMgr.getRef(startNote.ID), segEndRef = idMgr.getRef(endNote.ID);
			RefType lastHeadRef = levelData.entities[entityJoints[0]].name = segStartRef;
			for (size_t connHeadIdx = 0, connTailIdx = 1; connTailIdx < entityJoints.size(); ++connHeadIdx, ++connTailIdx)
			{
				const auto& headEnt = levelData.entities[entityJoints[connHeadIdx]];
				const auto& tailEnt = levelData.entities[entityJoints[connTailIdx]];
				RefType tailRef = tailEnt.name;
				
				if (!hold.isGuide())
					insertTransientTickNote(headEnt, tailEnt, connHeadIdx == 0, levelData.entities);
				
				levelData.entities.emplace_back(toConnector(hold, lastHeadRef, tailRef, segStartRef, segEndRef));
				lastHeadRef = std::move(tailRef);
			}

			for (auto&& [entityIndex, jointIndex] : attachEntities)
			{
				auto& attachEntity = levelData.entities[entityIndex];
				auto& headEntity = levelData.entities[entityJoints[jointIndex]];
				auto& tailEntity = levelData.entities[entityJoints[jointIndex + 1]];
				attachEntity.data.emplace("attachHead", headEntity.name);
				attachEntity.data.emplace("attachTail", tailEntity.name);
				if (attachEntity.data.find("lane") != attachEntity.data.end())
				{
					int easeNumeric;
					RealType headBeat, headLane, headSize, tailBeat, tailLane, tailSize, attachBeat;
					if (   !headEntity.tryGetDataValue("#BEAT", headBeat) || !headEntity.tryGetDataValue("lane", headLane) || !headEntity.tryGetDataValue("size", headSize)
						|| !tailEntity.tryGetDataValue("#BEAT", tailBeat) || !tailEntity.tryGetDataValue("lane", tailLane) || !tailEntity.tryGetDataValue("size", tailSize)
						|| !headEntity.tryGetDataValue("connectorEase", easeNumeric) || !attachEntity.tryGetDataValue("#BEAT", attachBeat))
						continue;
					RealType ratio = unlerpD(headBeat, tailBeat, attachBeat);
					float (*easeFunc)(float, float, float);
					switch (easeNumeric)
					{
					default: case 1: easeFunc = lerp; break;
					case 2: easeFunc = easeIn; break;
					case 3: easeFunc = easeOut; break;
					}
					attachEntity.data["lane"] = easeFunc(headLane, tailLane, ratio);
					attachEntity.data["size"] = easeFunc(headSize, tailSize, ratio);
				}
			}
		}
	
		std::vector<size_t> simEntities;
		for (auto it = simBuilder.begin(), end = simBuilder.end(); it != end; )
		{
			auto [startEnt, endEnt] = simBuilder.equal_range(it->first);
			simEntities.clear();
			std::transform(startEnt, endEnt, std::back_inserter(simEntities), [](const std::multimap<TickType, size_t>::value_type& val) { return val.second; });
			std::sort(simEntities.begin(), simEntities.end(), [&](size_t aEnt, size_t bEnt){ return levelData.entities[aEnt].getDataValue<RealType>("lane") < levelData.entities[bEnt].getDataValue<RealType>("lane"); });

			for (size_t i = 1; i < simEntities.size(); ++i)
			{
				auto& left = levelData.entities[simEntities[i - 1]].name, &right = levelData.entities[simEntities[i]].name;
				if (left.empty()) left = idMgr.getNextRef();
				if (right.empty()) right = idMgr.getNextRef();
				levelData.entities.push_back({ "SimLine", { {"left", left}, {"right", right} } });
			}
			it = endEnt;
		}

		return levelData;
	}

	Score PySekaiEngine::deserialize(const Sonolus::LevelData& levelData)
	{
		throw std::runtime_error("Importing is not supported!");
	}

	LevelDataEntity PySekaiEngine::toSpeedChangeEntity(const HiSpeedChange &hispeed, const RefType& groupName)
	{
		return {
			"#TIMESCALE_CHANGE",
			{
				{ "#BEAT",				ticksToBeats(hispeed.tick)	},
				{ "#TIMESCALE",			roundOff(hispeed.speed) 	},
				{ "#TIMESCALE_SKIP",	RealType(0) 				},
				{ "#TIMESCALE_EASE",	0 							},
				{ "#TIMESCALE_GROUP",	groupName					},
			}
		};
	}

	Sonolus::LevelDataEntity PySekaiEngine::toNoteEntity(const Note &note, const std::string& archetype, const RefType& groupName, HoldNoteType hold, HoldStepType step, EaseType easing, bool isGuide, double alpha)
	{
		return {
			archetype,
			{
				{ "#TIMESCALE_GROUP", groupName						},
				{ "#BEAT", ticksToBeats(note.tick)              	},
				{ "lane", toSonolusLane(note.lane, note.width)  	},
				{ "size", widthToSize(note.width)					},
				{ "direction", toDirectionNumeric(note.flick)		},
				{ "isAttached", step == HoldStepType::Skip ? 1 : 0	},
				{ "isSeparator", isGuide ? 1 : 0					},
				{ "connectorEase", toEaseNumeric(easing)			},
				{ "segmentKind", toKindNumeric(note.critical, hold)	},
				{ "segmentAlpha", alpha								}
			}
		};
	}

	Sonolus::LevelDataEntity PySekaiEngine::toConnector(const HoldNote &hold, const RefType &head, const RefType &tail, const RefType &segmentHead, const RefType &segmentTail)
	{
		LevelDataEntity::MapDataType data = {
			{ "head",   head },
			{ "tail",   tail },
		};
		if (hold.isGuide())
		{
			data.emplace("segmentHead", head);
			data.emplace("segmentTail", tail);
		}
		else
		{
			data.emplace("activeHead",	segmentHead);
			data.emplace("activeTail",	segmentTail);
			data.emplace("segmentHead",	segmentHead);
			data.emplace("segmentTail",	segmentTail);
		}
		return { "Connector", std::move(data) };
	}

	std::string PySekaiEngine::getHoldNoteArchetype(const Note &note, const HoldNote &holdNote)
	{
		std::string archetype = (note.critical ? "Critical" : "Normal");
		if (note.ID == holdNote.start.ID)
		{
			if (holdNote.startType != HoldNoteType::Normal)
				return "AnchorNote";
			archetype += "Head";
			if (note.friction)
				archetype += "Trace";
			else
				archetype += "Tap";
		}
		else if (note.ID == holdNote.end)
		{
			if (holdNote.endType != HoldNoteType::Normal)
				return "AnchorNote";
			archetype += "Tail";
			if (note.friction)
				archetype += "Trace";
			if (note.isFlick())
				archetype += "Flick";
			if (!note.friction && !note.isFlick())
				archetype += "Release";
		}
		else
		{
			int idx = findHoldStep(holdNote, note.ID);
			switch (holdNote.steps[idx].type)
			{
			case HoldStepType::Normal:
			case HoldStepType::Skip:
				archetype += "Tick";
				break;
			case HoldStepType::Hidden:
				return "AnchorNote";
			default: // Do something about this
				archetype += "Tap";
				break;
			}
		}
		archetype += "Note";
		return archetype;
	}

	void PySekaiEngine::insertTransientTickNote(const Sonolus::LevelDataEntity& head, const Sonolus::LevelDataEntity& tail, bool isHead, std::vector<Sonolus::LevelDataEntity>& entities)
	{
		double headHalfBeat;
		double headFracHalfBeat = std::modf(head.getDataValue<RealType>("#BEAT") * 2, &headHalfBeat);
		bool skips = (isHead || headFracHalfBeat != 0) ? 1 : 0;
		int endHalfBeat = std::ceil(tail.getDataValue<RealType>("#BEAT") * 2);
		// Copying head and tail since they are apart of entities list. They may relocate when inserting.
		std::string headName = head.name, tailName = tail.name;
		for (int halfBeat = headHalfBeat + skips; halfBeat < endHalfBeat; ++halfBeat)
		{
			entities.emplace_back(LevelDataEntity{
				"TransientHiddenTickNote",
				{
					{ "#BEAT", halfBeat / 2. },
					{ "isAttached",		   1 },
					{ "attachHead", headName },
					{ "attachTail", tailName }
				}
			});
		}
	}

	int PySekaiEngine::toDirectionNumeric(FlickType flick)
	{
		switch (flick)
		{
		case FlickType::Left:
			return 1;
		case FlickType::Right:
			return 2;
		default:
			PRINT_DEBUG("Unknown FlickType");
		case FlickType::None:
		case FlickType::Default:
			return 0;
		}
	}

	int PySekaiEngine::toEaseNumeric(EaseType ease)
	{
		switch (ease)
		{
		case EaseType::Linear:
			return 1;
		case EaseType::EaseIn:
			return 2;
		case EaseType::EaseOut:
			return 3;
		default:
			PRINT_DEBUG("Unknown EaseType");
			return 0;
		}
	}

	int PySekaiEngine::toKindNumeric(bool critical, HoldNoteType holdType)
	{
		switch (holdType)
		{
		case HoldNoteType::Normal:
		case HoldNoteType::Hidden:
			return (critical ? 2 : 1);
		case HoldNoteType::Guide:
			return (critical ? 105 : 103);
		default:
			PRINT_DEBUG("Unknown HoldNoteType");
			return 0;
		}
	}

#pragma endregion

	static constexpr size_t TOTAL_ENGINES = 2;
	static const std::array<std::string_view, TOTAL_ENGINES> ENGINE_NAMES = { "Sonolus Sekai Best", "Sonolus Next Sekai" };
	static const std::array<bool, TOTAL_ENGINES> ENGINE_SERIALIZABLE = { true, true };
	static const std::array<bool, TOTAL_ENGINES> ENGINE_DESERIALIZABLE = { true, false };

	bool SonolusEngineController::updateEngineSelector(SerializeResult& result)
	{
		bool selected = false;

		if (open)
		{
			ImGui::OpenPopup("###engine_menu");
			open = false;
		}

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSizeConstraints({ 450, 300 }, { FLT_MAX, FLT_MAX });
		ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize / 2.f, ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(IO::formatString(APP_NAME " - %s###engine_menu", getString("select_engine")).c_str(), NULL, ImGuiWindowFlags_NoResize))
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::Text(getString("select_engine"));
			if (isSerializing && config.debugEnabled)
			{
				ImGui::Checkbox(getString("compress"), &compress);
			}
			ImVec2 listBoxSize = ImGui::GetContentRegionAvail() - ImVec2(0, ImGui::GetFrameHeightWithSpacing());
			if (ImGui::BeginListBox("##listbox", listBoxSize))
			{
				for (size_t i = 0; i != TOTAL_ENGINES; i++)
				{
					if ((isSerializing && !ENGINE_SERIALIZABLE[i]) || ((!isSerializing && !ENGINE_DESERIALIZABLE[i]))) continue;
					bool isSelected = selectedItem == i;
					if (isSelected)
					{
						const ImVec4 color = UI::accentColors[config.accentColor];
						const ImVec4 darkColor = generateDarkColor(color);
						const ImVec4 lightColor = generateHighlightColor(color);
						ImGui::PushStyleColor(ImGuiCol_Header, color);
						ImGui::PushStyleColor(ImGuiCol_HeaderHovered, darkColor);
						ImGui::PushStyleColor(ImGuiCol_HeaderActive, lightColor);
					}
					ImVec2 cursor = ImGui::GetCursorScreenPos();
					float lineHeight = ImGui::GetTextLineHeightWithSpacing();
					float availWidth = ImGui::GetContentRegionAvail().x;
					ImGui::PushID(i);
					if (ImGui::Selectable(ENGINE_NAMES[i].data(), isSelected, ImGuiSelectableFlags_AllowItemOverlap, {availWidth, lineHeight}))
						selectedItem = i;
					ImGui::PopID();
					if (isSelected)
					{
						ImGui::PopStyleColor(3);
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndListBox();
			}
			ImVec2 btnBarPos = ImGui::GetCursorPos();
			float btnWidth = ImGui::GetContentRegionAvail().x / 2 - style.ItemSpacing.x, btnHeight = ImGui::GetFrameHeight();

			bool closePopup = false;
			ImGui::BeginDisabled(selectedItem < 0 || selectedItem >= ENGINE_NAMES.size());
			if (ImGui::Button(getString("select"), { btnWidth, btnHeight }))
			{
				selected = true;
				closePopup = true;
			}
			ImGui::EndDisabled();
			ImGui::SameLine(0, style.ItemSpacing.x);
			if (ImGui::Button(getString("cancel"), { btnWidth, btnHeight }))
			{
				closePopup = true;
				result = SerializeResult::Cancel;
			}
			if (closePopup)
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		
		return selected;
	}

	SonolusEngineController::SonolusEngineController(Score score, const std::string& filename)
	{
		isSerializing = true;
		this->score = std::move(score);
		this->filename = filename;
	}

	SonolusEngineController::SonolusEngineController(const std::string& filename)
	{
		isSerializing = false;
		this->filename = filename;
	}
	SerializeResult SonolusEngineController::update()
	{
		SerializeResult result = SerializeResult::None;
		if (!serializer)
		{
			if (!updateEngineSelector(result))
				return result;
			switch (selectedItem)
			{
			case 0:
				serializer = std::make_unique<SonolusSerializer>(std::make_unique<SekaiBestEngine>(), compress);
				break;
			case 1:
				serializer = std::make_unique<SonolusSerializer>(std::make_unique<PySekaiEngine>(), compress);
				break;
			default:
				return SerializeResult::Error;
			}
		}
		if (isSerializing)
		{
			try
			{
				serializer->serialize(score, filename);
				return SerializeResult::SerializeSuccess;
			}
			catch (const std::exception& err)
			{
				errorMessage = IO::formatString(
					"%s\n"
					"%s: %s",
					getString("error_save_score_file"),
					getString("error"), err.what()
				);
				return SerializeResult::Error;
			}
		}
		else
		{
			int nextIdBackup = nextID;
			try
			{
				resetNextID();
				score = serializer->deserialize(filename);
				return SerializeResult::DeserializeSuccess;
			}
			catch (PartialScoreDeserializeError& partialError)
			{
				score = partialError.getScore();
				errorMessage = partialError.what();
				return SerializeResult::PartialDeserializeSuccess;
			}
			catch (std::exception& error)
			{
				nextID = nextIdBackup;
				errorMessage = IO::formatString(
					"%s\n"
					"%s: %s\n"
					"%s: %s",
					getString("error_load_score_file"),
					getString("error"), error.what()
				);
				return SerializeResult::Error;
			}
		}
	}
}