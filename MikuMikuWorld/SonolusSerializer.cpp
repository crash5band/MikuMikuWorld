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
				dv = std::div(static_cast<long long>(index), static_cast<long long>(base));
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
}