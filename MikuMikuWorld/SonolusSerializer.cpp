#include "Application.h"
#include "SonolusSerializer.h"
#include "json.hpp"
#include "Constants.h"
#include "IO.h"
#include "File.h"
#include "JsonIO.h"
#include "Utilities.h"
#include "UI.h"
#include "Colors.h"
#include "Sonolus.h"
#include "ApplicationConfiguration.h"
#include "Archive.h"

using namespace nlohmann;

namespace MikuMikuWorld
{
	constexpr std::array<std::string_view, 2> SUPPORTED_ENGINE = { "pjsekai", "prosekaR" };
	constexpr char ARCH_PATH_SEP = '\n';
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

	inline static std::string extractFile(IO::Archive& scpArchive, const std::string& entryPath, const std::string_view& outputExtension)
	{
		int64_t entryIdx;
		if (entryPath.empty() || (entryIdx = scpArchive.getEntryIndex(entryPath)) < 0)
			return {};
		IO::ArchiveFile archiveFile = scpArchive.openFile(entryIdx);
		if (!archiveFile.isOpen())
			return {};
		std::vector<uint8_t> bytes = archiveFile.readAllBytes();
		archiveFile.close();
		if (archiveFile.getLastError().get() != 0)
			return {};
		std::filesystem::path filePath = entryPath;
		filePath.replace_extension(outputExtension);
		filePath = (Application::getAppDir() + "extracted\\") / filePath.filename();
		std::filesystem::create_directory(filePath.parent_path());
		IO::File extractedFile(filePath.native(), IO::FileMode::WriteBinary);
		extractedFile.writeAllBytes(bytes);
		extractedFile.flush();
		extractedFile.close();
		return filePath.string();
	}

	void SonolusSerializer::serialize(const Score& score, std::string filename)
	{
		size_t sepIdx = filename.find(ARCH_PATH_SEP);
		std::string archiveFilename;
		std::string levelName;
		if (sepIdx != std::string::npos)
		{
			archiveFilename.append(filename.begin(), filename.begin() + sepIdx);
			levelName.append(filename.begin() + sepIdx + 1, filename.end());
		}

		Sonolus::LevelData levelData;
		levelData.bgmOffset = score.metadata.musicOffset;
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

		std::string serializedData = json(levelData).dump();
		std::vector<uint8_t> serializedBytes(serializedData.begin(), serializedData.end());

		serializedBytes = IO::deflateGzip(serializedBytes);

		if (!archiveFilename.empty())
		{
			IO::Archive scpArch(archiveFilename);
			if (!scpArch.isOpen())
				throw Sonolus::SonolusSerializingError(IO::formatString("Failed to open archive file \"%s\".\nReason: %s", archiveFilename, scpArch.getLastError().getStr()));
			Sonolus::ItemDetail<Sonolus::LevelItem> details = { Sonolus::generateLevelItem(score) };
			Sonolus::LevelItem& item = details.item;
			if (levelName.empty())
				levelName.append("mmw-").append(Sonolus::hash(serializedBytes));
			item.name = levelName;
			if (!score.metadata.musicFile.empty()) item.bgm = Sonolus::packageFile<Sonolus::ResourceType::LevelBgm>(scpArch, score.metadata.musicFile);
			if (!score.metadata.jacketFile.empty()) item.cover = Sonolus::packageFile<Sonolus::ResourceType::LevelCover>(scpArch, score.metadata.jacketFile);
			item.data = Sonolus::packageData<Sonolus::ResourceType::LevelData>(scpArch, serializedBytes);
			Sonolus::serializeLevelDetails(scpArch, details);

			auto levelInfo = Sonolus::deserializeLevelInfo(scpArch);
			auto levelList = Sonolus::deserializeLevelList(scpArch);

			auto lit = std::find_if(levelList.items.begin(), levelList.items.end(), [&](Sonolus::LevelItem& item) { return item.name == levelName; });
			if (lit == levelList.items.end())
				levelList.items.push_back(item);
			else
			{
				if (!lit->bgm.url.empty() && lit->bgm.hash != item.bgm.hash) scpArch.removeFile(lit->bgm.url);
				if (!lit->cover.url.empty() && lit->cover.hash != item.cover.hash) scpArch.removeFile(lit->cover.url);
				if (!lit->data.url.empty() && lit->data.hash != item.data.hash) scpArch.removeFile(lit->data.url);
				if (!lit->preview.url.empty() && lit->preview.hash != item.preview.hash) scpArch.removeFile(lit->preview.url);
				*lit = item;
			}

			for (auto& section : levelInfo.sections)
			{
				auto iit = std::find_if(section.items.begin(), section.items.end(), [&](Sonolus::LevelItem& item) { return item.name == levelName; });
				if (iit != section.items.end())
					*iit = item;
				else if (section.title == "#NEWEST")
					section.items.push_back(item);
			}

			Sonolus::serializeLevelInfo(scpArch, levelInfo);
			Sonolus::serializeLevelList(scpArch, levelList);
		}
		else
		{
			IO::File lvlFile(filename, IO::FileMode::WriteBinary);
			lvlFile.writeAllBytes(serializedBytes);
			lvlFile.flush();
			lvlFile.close();
		}
	}

	Score SonolusSerializer::deserialize(std::string filename)
	{
		size_t sepIdx = filename.find(ARCH_PATH_SEP);
		Sonolus::LevelData lvlData;
		Sonolus::LevelItem lvlItem;
		std::string jacketFile;
		std::string musicFile;

		if (sepIdx != std::string::npos)
		{
			std::string archiveFilename(filename.begin(), filename.begin() + sepIdx);
			std::string levelName(filename.begin() + sepIdx + 1, filename.end());
			IO::Archive scpArch(archiveFilename, IO::ArchiveOpenMode::Read);
			if (!scpArch.isOpen())
				throw Sonolus::SonolusSerializingError(IO::formatString("Failed to open archive file \"%s\".\nReason: %s", archiveFilename, scpArch.getLastError().getStr()));
			lvlItem = Sonolus::deserializeLevelDetails(scpArch, levelName).item;

			if (lvlItem.data.url.empty())
				throw Sonolus::SonolusSerializingError("Level data not found!");
			IO::ArchiveFile levelDataGzip = scpArch.openFile(lvlItem.data.url);
			if(!levelDataGzip.isOpen())
				throw Sonolus::SonolusSerializingError(IO::formatString("Failed to open level file \"%s\".\nReason: %s", levelName, scpArch.getLastError().getStr()));
			std::vector<uint8_t> bytes = levelDataGzip.readAllBytes();
			levelDataGzip.close();
			if (IO::isGzipCompressed(bytes))
				bytes = IO::inflateGzip(bytes);
			nlohmann::json lvlDatajson = nlohmann::json::parse(std::string(bytes.begin(), bytes.end()));
			lvlDatajson.get_to(lvlData);

			jacketFile = extractFile(scpArch, lvlItem.cover.url, ".png");
			musicFile = extractFile(scpArch, lvlItem.bgm.url, ".mp3");
		}
		else
		{
			if (!IO::File::exists(filename.c_str()))
				return {};
			IO::File gzLvlFile(filename, IO::FileMode::ReadBinary);
			std::vector<uint8_t> bytes = gzLvlFile.readAllBytes();
			gzLvlFile.close();
			if (IO::isGzipCompressed(bytes))
				bytes = IO::inflateGzip(bytes);
			nlohmann::json lvlDatajson = nlohmann::json::parse(std::string(bytes.begin(), bytes.end()));
			lvlDatajson.get_to(lvlData);
		}

		Score score;
		score.metadata.title = lvlItem.title;
		score.metadata.artist = lvlItem.artists;
		score.metadata.author = lvlItem.author;
		score.metadata.musicOffset = lvlData.bgmOffset;
		score.metadata.jacketFile = jacketFile;
		score.metadata.musicFile = musicFile;
		
		auto isTimescaleEntity = [](const Sonolus::LevelDataEntity& ent) { return ent.archetype == "#TIMESCALE_CHANGE"; };
		auto isBpmChangeEntity = [](const Sonolus::LevelDataEntity& ent) { return ent.archetype == "#BPM_CHANGE"; };
		auto isTapNoteEntity = [](const Sonolus::LevelDataEntity& ent) { return IO::endsWith(ent.archetype, "Note") && ent.archetype.find("Slide") == std::string::npos; };
		auto isSlideConnectorEntity = [](const Sonolus::LevelDataEntity& ent) { return IO::endsWith(ent.archetype, "SlideConnector"); };
		auto isAttachedTickNote = [](const Sonolus::LevelDataEntity& ent) { return IO::endsWith(ent.archetype, "AttachedSlideTickNote"); };
		
		for (const auto& timescaleEntity : lvlData.entities)
		{
			if (!isTimescaleEntity(timescaleEntity)) continue;
			float beat = 0, speed = 1;
			if (timescaleEntity.tryGetDataValue("#BEAT", beat) && timescaleEntity.tryGetDataValue("#TIMESCALE", speed))
				score.hiSpeedChanges.push_back({ beatsToTicks(beat), speed });
		}

		score.tempoChanges.clear();
		for (const auto& bpmChangeEntity : lvlData.entities)
		{
			if (!isBpmChangeEntity(bpmChangeEntity)) continue;
			float beat, bpm;
			if (bpmChangeEntity.tryGetDataValue("#BEAT", beat) && bpmChangeEntity.tryGetDataValue("#BPM", bpm))
				score.tempoChanges.push_back({ beatsToTicks(beat), bpm });
		}
		if (score.tempoChanges.empty())
			score.tempoChanges.push_back(Tempo{});

		for (const auto& tapNoteEntity : lvlData.entities)
		{
			if (!isTapNoteEntity(tapNoteEntity)) continue;
			Note note(NoteType::Tap);
			if (!tapNoteArchetypeToNativeNote(tapNoteEntity, note))
				continue;
			note.ID = nextID;
			score.notes[nextID++] = note;
		}

		std::unordered_map<std::string_view, size_t> entityNameMap;
		for (size_t i = 0; i < lvlData.entities.size(); ++i)
		{
			const auto& entity = lvlData.entities[i];
			if (!entity.name.empty())
				entityNameMap.emplace(entity.name, i);
		}
		// We'll use this as a linked list to walk forward and collect all the hold steps
		std::unordered_map<std::string, LinkedHoldStep> headToTailMap;
		// This is where we'll start building the hold note
		std::unordered_set<std::string> startRefSet;
		
		for (const auto& connectorEntity : lvlData.entities)
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
		for (size_t i = 0; i < lvlData.entities.size(); ++i)
		{
			auto& attachTickEntity = lvlData.entities[i];
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
			const auto& connectorEntity = lvlData.entities[entIt->second];
			if (!connectorEntity.tryGetDataValue("head", headRef))
				continue;
			auto headIt = headToTailMap.find(headRef);
			if (headIt == headToTailMap.end())
				continue;
			if (attachTickEntity.name.empty())
			{
				attachTickEntity.name = ("__mmw__" + idMgr.getNextRef());
				entityNameMap[attachTickEntity.name] = i;
			}
			// Insert the attach note between the head and tail
			headToTailMap[attachTickEntity.name] = LinkedHoldStep{ headIt->second.nextTail , LinkedHoldStep::Attach };
			headIt->second.nextTail = attachTickEntity.name;
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
				auto& holdStepEntity = lvlData.entities[it->second];
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
			if (validHold)
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

	float SonolusSerializer::ticksToBeats(int ticks, int beatTicks)
	{
		return static_cast<float>(ticks) / beatTicks;
	}

	float SonolusSerializer::widthToSize(int width)
	{
		return static_cast<float>(width) / 2.f;
	}

	float SonolusSerializer::toSonolusLane(int lane, int width)
	{
		return (lane - 6) + (width / 2.0f);
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

	int SonolusSerializer::beatsToTicks(float beats, int beatTicks)
	{
		return std::lround(beats * beatTicks);
	}

	int SonolusSerializer::sizeToWidth(float size)
	{
		return size * 2;
	}

	int SonolusSerializer::toNativeLane(float lane, float size)
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

	void SonolusSerializationDialog::openSerializingDialog(const ScoreContext& context, const std::string& filename)
	{
		ScoreSerializationDialog::openSerializingDialog(context, filename);
		if (!serializer)
			return;
		try
		{
			selectedItem = -1;
			Sonolus::ItemList<Sonolus::LevelItem> levelList;
			if (IO::File::exists(filename))
				levelList = Sonolus::deserializeLevelList(filename);
			else
				if (!Sonolus::createSonolusCollection(filename))
				{
					errorMessage = "Error opening " + filename;
					serializer.reset();
					return;
				}
			levelInfos.clear();
			for (auto& level : levelList.items)
			{
				levelInfos.push_back({
					std::move(level.name),
					level.rating,
					std::move(level.title),
					std::move(level.artists),
					std::move(level.author),
					LevelMetadata::None
				});
			}
			levelInfos.push_back({ "", 0, "Insert a new level..." });
		}
		catch (std::exception& ex)
		{
			errorMessage = ex.what();
			serializer.reset();
		}
	}

	void SonolusSerializationDialog::openDeserializingDialog(const std::string &filename)
	{
		ScoreSerializationDialog::openDeserializingDialog(filename);
		if (!serializer)
			return;
		try
		{
			selectedItem = -1;
			IO::Archive scpArchive(filename, IO::ArchiveOpenMode::Read);
			if (!scpArchive.isOpen())
			{
				errorMessage = "Error openning file " + filename;
				serializer.reset();
				return;
			}
			auto levelList = Sonolus::deserializeLevelList(scpArchive);
			if (levelList.items.size() == 0)
			{
				errorMessage = "No levels found in the collection!";
				serializer.reset();
				return;
			}
			levelInfos.clear();
			for (auto& level : levelList.items)
			{
				bool isEngineSupported = std::any_of(std::begin(SUPPORTED_ENGINE), std::end(SUPPORTED_ENGINE), [&](const std::string_view& str) { return str == level.engine.name; });
				bool isLevelDataIncluded = !level.data.url.empty() && scpArchive.getEntryIndex(level.data.url) >= 0;
				LevelMetadata::Flag flag = isEngineSupported ? LevelMetadata::None : LevelMetadata::EngineNotRegconized;
				flag = static_cast<LevelMetadata::Flag>(flag | (isLevelDataIncluded ? LevelMetadata::None : LevelMetadata::LevelDataMissing));
				levelInfos.push_back({
					std::move(level.name),
					level.rating,
					std::move(level.title),
					std::move(level.artists),
					std::move(level.author),
					flag
				});
			}
		}
		catch (std::exception& ex)
		{
			errorMessage = ex.what();
			serializer.reset();
		}
	}

	SerializationDialogResult SonolusSerializationDialog::update()
	{
		if (!serializer)
			return SerializationDialogResult::Error;
		SerializationDialogResult result = SerializationDialogResult::None;
		if (updateFileSelector(result) && result == SerializationDialogResult::None)
		{
			if (isSerializing)
			{
				try
				{
					std::string levelFileName = filename;
					levelFileName.append(1, ARCH_PATH_SEP).append(insertLevelName);
					serializer->serialize(score, levelFileName);
					return SerializationDialogResult::SerializeSuccess;
				}
				catch (const std::exception& err)
				{
					errorMessage = IO::formatString("An error occured while saving the score file\n%s", err.what());
					return SerializationDialogResult::Error;
				}
			}
			else
			{
				int nextIdBackup = nextID;
				try
				{
					resetNextID();
					std::string levelFileName = filename;
					levelFileName.append(1, ARCH_PATH_SEP).append(levelInfos[selectedItem].name);
					score = serializer->deserialize(levelFileName);
					return SerializationDialogResult::DeserializeSuccess;
				}
				catch (std::exception& error)
				{
					nextID = nextIdBackup;
					errorMessage = IO::formatString("%s\n%s: %s\n%s: %s",
						getString("error_load_score_file"),
						getString("score_file"),
						filename.c_str(),
						getString("error"),
						error.what()
					);
					return SerializationDialogResult::Error;
				}
			}
		}
		return result;
	}

	bool SonolusSerializationDialog::updateFileSelector(SerializationDialogResult& result)
	{
		if (open)
		{
			ImGui::OpenPopup("###scp_menu");
			open = false;
		}

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSizeConstraints({ 750, 600 }, { FLT_MAX, FLT_MAX });
		ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize / 1.25f, ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(IO::formatString("%s - %s###scp_menu", APP_NAME, "Sonolus Collection").c_str(), NULL, ImGuiWindowFlags_NoResize))
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::TextUnformatted(isSerializing ? "Select a level to save" : "Select a level to open");
			if (isSerializing)
			{
				ImGui::BeginDisabled(selectedItem < 0);
				ImGui::InputTextWithHint("##lvl_id_input", "Level ID (leaves empty for auto generate id)", &insertLevelName, ImGuiInputTextFlags_CharsNoBlank);
				// TODO: Add options to set Tags
				ImGui::EndDisabled();
			}
			float listBoxWidth = ImGui::GetContentRegionAvail().x - 160.f, listBoxHeight = ImGui::GetContentRegionAvail().y;
			if (ImGui::BeginListBox("##listbox", ImVec2(listBoxWidth, listBoxHeight)))
			{
				ImDrawList* drawList = ImGui::GetWindowDrawList();
				ImVec2 listBoxTopLeft = ImGui::GetCursorPos();
				constexpr float separatorRatio = 0.35f;
				ImGui::SetCursorPosY(listBoxTopLeft.y + 4); // Padding
				for (size_t i = 0; i != levelInfos.size(); i++)
				{
					auto& level = levelInfos[i];
					bool isSelected = selectedItem == i;
					bool haveIssue = level.flags != LevelMetadata::None;
					bool drawToolTip = false;
					const char* label = "";
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
					ImGui::BeginDisabled(haveIssue);
					ImGui::PushID(i);
					if (ImGui::Selectable("", isSelected, ImGuiSelectableFlags_AllowItemOverlap, { availWidth, lineHeight }))
					{
						selectedItem = i;
						if (isSerializing && selectedItem == levelInfos.size() - 1)
							insertLevelName.clear();
						else
							insertLevelName = levelInfos[selectedItem].name;
					}
					drawToolTip = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && GImGui->HoveredIdTimer > 0.5f;
					if (level.flags & LevelMetadata::EngineNotRegconized)
						label = "Level data is incompatible with " APP_NAME;
					if (level.flags & LevelMetadata::LevelDataMissing)
						label = "Level data is missing! Try exporting the collection in Full mode.";
					if (haveIssue)
					{
						ImGui::SameLine(availWidth - 25);
						ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), ICON_FA_EXCLAMATION_TRIANGLE);
					}
					ImGui::PopID();
					ImGui::EndDisabled();
					if (haveIssue && drawToolTip)
					{
						float txtWidth = ImGui::CalcTextSize(label).x + (ImGui::GetStyle().WindowPadding.x * 2);
						ImGui::SetNextWindowSize({ 250, -1 });
						ImGui::BeginTooltipEx(ImGuiTooltipFlags_OverridePreviousTooltip, ImGuiWindowFlags_NoResize);
						ImGui::TextWrapped(label);
						ImGui::EndTooltip();
					}
					if (isSelected)
					{
						ImGui::PopStyleColor(3);
						ImGui::SetItemDefaultFocus();
					}
					ImRect nameBB(cursor, cursor + ImVec2{ availWidth * separatorRatio - style.ItemSpacing.x, lineHeight });
					ImGui::RenderTextEllipsis(drawList, nameBB.Min, nameBB.Max, nameBB.Max.x, nameBB.Max.x, level.name.c_str(), nullptr, nullptr);
					ImVec2 titleTxtSz = ImGui::CalcTextSize(level.title.c_str());
					float titleLeft = availWidth * separatorRatio + style.ItemSpacing.x, titleWidth;
					if ((titleWidth = availWidth - titleLeft) > titleTxtSz.x)
					{
						float titleTxtLeft = titleLeft + (titleWidth - titleTxtSz.x) / 2;
						ImGui::RenderText(cursor + ImVec2{titleTxtLeft, 0}, level.title.c_str());
					}
					else
					{
						ImRect titleBB(cursor + ImVec2{ titleLeft, 0 }, cursor + ImVec2{ availWidth, lineHeight });
						ImGui::RenderTextEllipsis(drawList, titleBB.Min, titleBB.Max, titleBB.Max.x, titleBB.Max.x, level.title.c_str(), nullptr, nullptr);
					}
				}
				ImVec2 separatorPos = ImGui::GetWindowPos() + listBoxTopLeft + ImVec2{ ImGui::GetContentRegionAvail().x * separatorRatio, 0 };
				drawList->AddLine(separatorPos, separatorPos + ImVec2{ 0, listBoxHeight - style.FramePadding.y * 2 }, ImGui::GetColorU32(ImGuiCol_Border), 1);
				ImGui::EndListBox();
			}
			ImGui::SameLine(0, style.FramePadding.x);
			ImVec2 btnBarPos = ImGui::GetCursorPos();
			float btnWidth = ImGui::GetContentRegionAvail().x, btnHeight = ImGui::GetFrameHeight();

			ImGui::BeginDisabled(!isArrayIndexInBounds(selectedItem, levelInfos));
			if (ImGui::Button("Select", { btnWidth, btnHeight }))
			{
				ImGui::EndDisabled();
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return true;
			}
			ImGui::EndDisabled();
			ImGui::SetCursorPos(btnBarPos + ImVec2{ 0, btnHeight + style.ItemSpacing.y });
			if (ImGui::Button("Cancel", { btnWidth, btnHeight }))
			{
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				result = SerializationDialogResult::Cancel;
				return true;
			}
			ImGui::EndPopup();
		}
		return false;
	}
}