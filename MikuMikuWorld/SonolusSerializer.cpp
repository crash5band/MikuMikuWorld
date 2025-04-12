#include "SonolusSerializer.h"
#include "json.hpp"
#include "Constants.h"
#include "IO.h"
#include "File.h"
#include "JsonIO.h"
#include "Utilities.h"
#include <zlib.h>
#include <fstream>

using namespace nlohmann;

namespace MikuMikuWorld
{
	constexpr int halfBeat = TICKS_PER_BEAT / 2;

	std::vector<DataValue> getSlideConnectorData(const std::string& start, const std::string& end, const std::string& head, const std::string &tail, int ease)
	{
		return std::vector<DataValue>
		{
			{ "start", start },
			{ "end", end },
			{ "head", head },
			{ "tail", tail },
			DataValue("ease", ease)
		};
	}

	HoldNoteType getHoldNoteTypeFromConnection(const SlideConnection& connection, bool hidden)
	{
		if (hidden)
			return HoldNoteType::Hidden;

		return !connection.active ? HoldNoteType::Guide : HoldNoteType::Normal;
	}

	const json& findEntityByName(const json& entities, const std::string& name)
	{
		for (const auto& entity : entities)
		{
			const std::string nameProperty = jsonIO::tryGetValue<std::string>(entity, "name", "");
			if (nameProperty == name)
				return entity;
		}

		// BAD!
		return {};
	}

	void SonolusSerializer::serialize(const Score& score, std::string filename)
	{
		json entities = json::array();
		entities.push_back({ { "archetype", "Initialization" }, {"data", json::array()} });
		entities.push_back({ { "archetype", "Stage" }, {"data", json::array()} });

		std::vector<ArchetypeData> entitiesData;
		for (const auto& speed : score.hiSpeedChanges)
		{
			std::vector<DataValue> data;
			data.push_back({ "#BEAT", ticksToBeats(speed.tick) });
			data.push_back({ "#TIMESCALE", speed.speed });
			
			entitiesData.push_back({ "", "#TIMESCALE_CHANGE", data });
		}

		for (const auto& bpm : score.tempoChanges)
		{
			std::vector<DataValue> data;
			data.push_back({ "#BEAT", ticksToBeats(bpm.tick) });
			data.push_back({ "#BPM", bpm.bpm });

			entitiesData.push_back({ "", "#BPM_CHANGE", data });
		}

		for (const auto& [id, note] : score.notes)
		{
			std::vector<DataValue> data;
			data.push_back({ "#BEAT", ticksToBeats(note.tick) });
			data.push_back({ "lane", toSonolusLane(note.lane, note.width) });
			data.push_back({ "size", static_cast<float>(note.width) / 2.0f });

			if (note.isFlick())
			{
				data.push_back(DataValue("direction", flickToDirection(note.flick)));
			}

			if (note.getType() == NoteType::HoldMid)
			{
				const HoldNote& hold = score.holdNotes.at(note.parentID);
				const int stepIndex = findHoldStep(hold, note.ID);
				if (isArrayIndexInBounds(stepIndex, hold.steps) && hold.steps[stepIndex].type == HoldStepType::Skip)
					data.push_back({ "attach", getSlideConnectorId(score.holdNotes.at(note.parentID).start) });
			}
			else if (note.getType() == NoteType::HoldEnd)
			{
				const HoldNote& hold = score.holdNotes.at(note.parentID);
				int lastStepIndex = hold.steps.size() - 1;
				while (lastStepIndex >= 0 && hold.steps[lastStepIndex].type == HoldStepType::Skip)
					lastStepIndex--;

				const HoldStep& connector = lastStepIndex < 0 ? hold.start : hold.steps.at(lastStepIndex);
				if (hold.endType == HoldNoteType::Normal)
					data.push_back({ "slide", getSlideConnectorId(connector) });
			}

			entitiesData.push_back({ std::to_string(id), noteToArchetype(score, note), data });
		}

		for (const auto& [id, hold] : score.holdNotes)
		{
			if (hold.isGuide())
				continue;

			int startTick = score.notes.at(id).tick;
			int endTick = score.notes.at(hold.end).tick;

			startTick += halfBeat;
			if (startTick % halfBeat)
				startTick -= (startTick % halfBeat);

			if (endTick % halfBeat)
				endTick += halfBeat - (endTick % halfBeat);

			for (int i = startTick; i < endTick; i += halfBeat)
			{
				std::vector<DataValue> data{ { "#BEAT", ticksToBeats(i) }, { "attach", std::to_string(id) } };
				entitiesData.push_back({ "", "HiddenSlideTickNote", data});
			}
		}

		for (const auto& [id, hold] : score.holdNotes)
		{
			std::string start = std::to_string(id);
			std::string end = std::to_string(hold.end);

			std::string slideConnectorArchetype = getSlideConnectorArchetype(score, hold);

			int s1 = -1, s2 = 0;
			if (!hold.steps.empty())
			{
				while (s2 < hold.steps.size() && hold.steps[s2].type == HoldStepType::Skip)
					s2++;

				if (s2 < hold.steps.size())
				{
					entitiesData.push_back({
						getSlideConnectorId(hold.start),
						slideConnectorArchetype,
						getSlideConnectorData(start, end, start, std::to_string(hold.steps[s2].ID), toSonolusEase(hold.start.ease))
					});
					s1 = s2;
				}
			}

			for (int currentStepIndex = s1 + 1; currentStepIndex < hold.steps.size(); currentStepIndex++)
			{
				if (hold.steps[currentStepIndex].type == HoldStepType::Skip)
					continue;

				s2 = currentStepIndex;

				entitiesData.push_back({
					getSlideConnectorId(hold.steps[s1]),
					slideConnectorArchetype,
					getSlideConnectorData(start, end, std::to_string(hold.steps[s1].ID), std::to_string(hold.steps[s2].ID), toSonolusEase(hold.steps[s1].ease))
				});

				s1 = s2;
			}

			entitiesData.push_back({
				getSlideConnectorId(s1 == -1 ? hold.start : hold.steps[s1]),
				slideConnectorArchetype,
				getSlideConnectorData(start, end, std::to_string(s1 == -1 ? id : hold.steps[s1].ID), std::to_string(hold.end), toSonolusEase(s1 == -1 ? hold.start.ease : hold.steps[s1].ease))
			});
		}

		const auto& simLineNotes = buildTickNoteMap(score);
		for (const auto& [tick, notes] : simLineNotes)
		{
			for (int i = 0; i < notes.size() - 1; i++)
			{
				std::vector<DataValue> data{ { "a", std::to_string(notes[i].ID) }, { "b", std::to_string(notes[i + 1].ID) } };
				entitiesData.push_back({ "", "SimLine", data });
			}
		}

		json entitiesJson = entitiesData;
		std::copy(entitiesJson.begin(), entitiesJson.end(), std::back_inserter(entities));

		json usc{};
		usc["bgmOffset"] = score.metadata.musicOffset;
		usc["entities"] = entities;

		std::string serializedUsc = usc.dump();
		std::vector<uint8_t> serializedUscBytes(serializedUsc.begin(), serializedUsc.end());

		if (useGzip)
			serializedUscBytes = IO::deflateGzip(serializedUscBytes);

		IO::File uscFile(filename, IO::FileMode::WriteBinary);
		uscFile.writeAllBytes(serializedUscBytes);
		uscFile.flush();
		uscFile.close();
	}

	Score SonolusSerializer::deserialize(std::string filename)
	{
		if (!IO::File::exists(filename))
			return {};

		IO::File gZippedUscFile(filename, IO::FileMode::ReadBinary);
		std::vector<uint8_t> uscBytes = gZippedUscFile.readAllBytes();
		gZippedUscFile.close();

		if (IO::isGzipCompressed(uscBytes))
			uscBytes = IO::inflateGzip(uscBytes);

		std::string uscJsonString(uscBytes.begin(), uscBytes.end());
		ordered_json uscJson = ordered_json::parse(uscJsonString);

		Score score{};
		score.metadata.musicOffset = jsonIO::tryGetValue<float>(uscJson, "bgmOffset", 0);

		json entities = uscJson["entities"];
		
		json timescaleEntities = getEntitiesByArchetype(entities, "#TIMESCALE_CHANGE");
		json bpmChangeEntities = getEntitiesByArchetype(entities, "#BPM_CHANGE");
		json noteEntities = getEntitiesByArchetypeEndingWith(entities, "Note");
		json slideConnectorEntities = getEntitiesByArchetypeEndingWith(entities, "Connector");

		json slideStartEntities{};
		std::copy_if(entities.begin(), entities.end(), std::back_inserter(slideStartEntities), [](const auto& e) {
			return IO::endsWith(e["archetype"], "SlideTraceNote") || IO::endsWith(e["archetype"], "SlideStartNote");
		});

		for (const auto& timescale : timescaleEntities)
		{
			const json& dataArray = timescale["data"];
			int tick = beatsToTicks(getEntityDataOrDefault<float>(dataArray, "#BEAT", 0));
			float speed = getEntityDataOrDefault<float>(dataArray, "#TIMESCALE", 1);
			
			score.hiSpeedChanges.push_back({ tick, speed });
		}

		if (!bpmChangeEntities.empty())
			score.tempoChanges.clear();

		for (const auto& bpmChange : bpmChangeEntities)
		{
			const json& dataArray = bpmChange["data"];

			int tick = beatsToTicks(getEntityDataOrDefault<float>(dataArray, "#BEAT", 0));
			float bpm = getEntityDataOrDefault<float>(dataArray, "#BPM", 120.0f);

			score.tempoChanges.push_back({ tick, bpm });
		}

		// We'll use this as a linked list to walk back and find which slide tick belongs to which slide
		std::unordered_map<std::string, SlideConnection> slideConnectionMapping{};
		std::unordered_map<std::string, std::string> tailToHeadMapping{};
		tailToHeadMapping.reserve(slideConnectorEntities.size());

		for (const auto& connector : slideConnectorEntities)
		{
			const json& dataArray = connector["data"];
			const std::string& head = getEntityDataOrDefault<std::string>(dataArray, "head", "");
			const std::string& tail = getEntityDataOrDefault<std::string>(dataArray, "tail", "");
			const std::string& archetype = jsonIO::tryGetValue<std::string>(connector, "archetype", "NormalActiveSlideConnector");

			bool active = archetype.find("Active") != std::string::npos;
			bool critical = archetype.find("Critical") != std::string::npos;
			int ease = getEntityDataOrDefault<int>(dataArray, "ease", 0);

			if (jsonIO::keyExists(connector, "name"))
			{
				slideConnectionMapping[connector["name"]] = { head, tail, ease, active, critical };
			}

			slideConnectionMapping[head] = { head, tail, ease, active, critical };
			tailToHeadMapping[tail] = head;
		}

		std::unordered_map<std::string, int> slideEntityMapping{};
		slideEntityMapping.reserve(slideStartEntities.size());

		for (const auto& slideEntity : slideStartEntities)
		{
			Note note = createNote(slideEntity, NoteType::Hold);

			HoldNote hold{};
			hold.start = { note.ID, HoldStepType::Normal, EaseType::Linear };
			auto connIt = slideConnectionMapping.find(slideEntity["name"]);

			if (connIt != slideConnectionMapping.end())
			{
				hold.start.ease = toNativeEase(connIt->second.ease);
			}

			score.notes[note.ID] = note;
			score.holdNotes[note.ID] = hold;
			slideEntityMapping[slideEntity["name"]] = note.ID;
		}

		/*
			Keeps track of which hold notes have been created with one of its steps.
			This is used to avoid created duplicate hidden slide steps
		*/
		std::unordered_set<std::string> skipNotes{};

		for (const auto& entity : noteEntities)
		{
			const std::string& archetype = jsonIO::tryGetValue<std::string>(entity, "archetype", "");
			const std::string& entityName = jsonIO::tryGetValue<std::string>(entity, "name", "");

			if (skipNotes.find(entityName) != skipNotes.end())
				continue;

			// The auto combo note isn't really a note to us
			if (archetype == "HiddenSlideTickNote")
				continue;

			const NoteType noteType = getNoteTypeFromArchetype(archetype);
			
			// We already handled slide start notes
			if (noteType == NoteType::Hold)
				continue;

			const json& dataArray = entity["data"];
			Note note = createNote(entity, noteType);

			if (noteType == NoteType::HoldMid)
			{
				std::string noteKey = IO::endsWith(archetype, "AttachedSlideTickNote") ?
					getEntityDataOrDefault<std::string>(dataArray, "attach", "") :
					jsonIO::tryGetValue<std::string>(entity, "name", "");

				std::string parentKey = getParentSlideConnector(tailToHeadMapping, noteKey);

				const auto& parentConnectionIt = slideConnectionMapping.find(parentKey);
				if (parentConnectionIt == slideConnectionMapping.end())
					continue;

				const auto& tailIt = tailToHeadMapping.find(noteKey);
				std::string childKey = getLastSlideConnector(slideConnectionMapping, tailIt == tailToHeadMapping.end() ? noteKey : tailIt->second);

				auto connectionIt = slideConnectionMapping.find(noteKey);
				if (connectionIt == slideConnectionMapping.end())
					connectionIt =  slideConnectionMapping.find(tailToHeadMapping[noteKey]);

				if (connectionIt == slideConnectionMapping.end())
					throw("Note key is neither a head nor tail?!");

				const SlideConnection& parentConnection = parentConnectionIt->second;
				const SlideConnection& lastConnection = slideConnectionMapping.at(childKey);
				const bool isTailNote = entityName == lastConnection.tail;

				HoldStep step{ note.ID, HoldStepType::Normal, EaseType::Linear };
				if (archetype == "IgnoredSlideTickNote" || IO::endsWith(archetype, "AttachedSlideTickNote"))
				{
					// Special case for guides/hidden start/end slide points
					const auto& holdIt = slideEntityMapping.find(parentConnection.head);
					if (holdIt == slideEntityMapping.end())
					{
						const bool isHeadNote = entityName == parentConnection.head;
						Note noteAsHold = Note(NoteType::Hold, note.tick, note.lane, note.width);
						noteAsHold.ID = note.ID;
						noteAsHold.critical = parentConnection.critical;
						if (!isHeadNote)
						{
							const json& parentEntity = findEntityByName(entities, parentConnection.head);
							const float size = getEntityDataOrDefault<float>(parentEntity["data"], "size", 1.5f);
							noteAsHold.tick = beatsToTicks(getEntityDataOrDefault<float>(parentEntity["data"], "#BEAT", 0));
							noteAsHold.lane = toNativeLane(getEntityDataOrDefault<float>(parentEntity["data"], "lane", 0), size);
							noteAsHold.width = static_cast<int>(size * 2);
							noteAsHold.ID = nextID++;

							skipNotes.insert(parentConnection.head);
						}

						const SlideConnection& connection = connectionIt->second;

						HoldStep start{ noteAsHold.ID, HoldStepType::Normal, toNativeEase(connection.ease) };
						HoldNoteType startType = getHoldNoteTypeFromConnection(connection, true);
						HoldNoteType endType = getHoldNoteTypeFromConnection(connection, false);
						HoldNote hold{ start, {}, 0, startType, endType };

						score.notes[noteAsHold.ID] = noteAsHold;
						score.holdNotes[noteAsHold.ID] = hold;
						slideEntityMapping[parentConnection.head] = noteAsHold.ID;

						if (isHeadNote)
							continue;
					}

					if (isTailNote)
					{
						const auto& holdIt = slideEntityMapping.find(parentConnection.head);
						if (holdIt == slideEntityMapping.end())
							throw("Hidden/Guide slide head created but not found?!");

						Note noteAsHoldEnd = Note(NoteType::HoldEnd, note.tick, note.lane, note.width);
						noteAsHoldEnd.ID = note.ID;
						noteAsHoldEnd.critical = score.notes.at(holdIt->second).critical;
						noteAsHoldEnd.parentID = holdIt->second;

						HoldNote& hold = score.holdNotes.at(holdIt->second);
						hold.end = noteAsHoldEnd.ID;
						hold.endType = hold.isGuide() ? HoldNoteType::Guide : HoldNoteType::Hidden;

						score.notes[noteAsHoldEnd.ID] = noteAsHoldEnd;
						continue;
					}

					step.type = HoldStepType::Hidden;
				}

				if (IO::endsWith(archetype, "AttachedSlideTickNote"))
				{
					step.type = HoldStepType::Skip;
				}

				const auto& holdIt = slideEntityMapping.find(parentConnection.head);
				if (holdIt == slideEntityMapping.end())
					throw("A slide tick without a parent?!");

				const SlideConnection& connection = connectionIt->second;
				step.ease = toNativeEase(connection.ease);

				HoldNote& hold = score.holdNotes.at(holdIt->second);
				const Note& parentNote = score.notes.at(hold.start.ID);
				note.critical = parentNote.critical;
				note.parentID = parentNote.ID;

				hold.steps.push_back(step);
			}
			else if (noteType == NoteType::HoldEnd)
			{
				std::string parentKey = getEntityDataOrDefault<std::string>(dataArray, "slide", "");
				const auto& connectionIt = slideConnectionMapping.find(parentKey);

				if (connectionIt == slideConnectionMapping.end())
					continue;

				std::string parentConnectionKey = getParentSlideConnector(tailToHeadMapping, connectionIt->second.head);

				const auto& parentSlide = slideEntityMapping.find(parentConnectionKey);
				if (parentSlide == slideEntityMapping.end())
					throw ("A slide end without a connection?!");

				const auto& parentHoldIt = score.holdNotes.find(parentSlide->second);
				if (parentHoldIt == score.holdNotes.end())
					throw ("A slide end without a start?!");

				HoldNote& parentHold = parentHoldIt->second;
				parentHold.end = note.ID;
				note.parentID = parentHold.start.ID;
			}

			score.notes[note.ID] = note;
		}

		for (auto& [id, hold] : score.holdNotes)
			sortHold(score, hold);

		return score;
	}

	float SonolusSerializer::ticksToBeats(int ticks)
	{
		return static_cast<float>(ticks) / TICKS_PER_BEAT;
	}

	float SonolusSerializer::toSonolusLane(int lane, int width)
	{
		return (lane - 6) + (static_cast<float>(width) / 2.0f);
	}

	int SonolusSerializer::flickToDirection(FlickType flick)
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

	int SonolusSerializer::beatsToTicks(float beats)
	{
		return beats * TICKS_PER_BEAT;
	}

	int SonolusSerializer::toNativeLane(float lane, float width)
	{
		return lane - width + 6;
	}

	FlickType SonolusSerializer::directionToFlick(int direction)
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

	json SonolusSerializer::getEntitiesByArchetype(const json& j, const std::string& archetype)
	{
		json entities{};
		std::copy_if(j.begin(), j.end(), std::back_inserter(entities),
			[&archetype](auto& j) { return jsonIO::tryGetValue<std::string>(j, "archetype", "") == archetype; });

		return entities;
	}

	json SonolusSerializer::getEntitiesByArchetypeEndingWith(const json& j, const std::string& archetype)
	{
		json entities{};
		std::copy_if(j.begin(), j.end(), std::back_inserter(entities),
			[&archetype](auto& j) { return IO::endsWith(jsonIO::tryGetValue<std::string>(j, "archetype", ""), archetype); });

		return entities;
	}

	std::string MikuMikuWorld::SonolusSerializer::getSlideConnectorId(const HoldStep& step)
	{
		return "connect_" + std::to_string(step.ID);
	}

	std::string SonolusSerializer::noteToArchetype(const Score& score, const Note& note)
	{
		std::string noteType = note.critical ? "Critical" : "Normal";
		if (note.friction)
			noteType.append("Trace");

		if (note.isFlick())
			noteType.append("Flick");

		if (note.getType() == NoteType::Hold)
		{
			if (score.holdNotes.at(note.ID).startType != HoldNoteType::Normal)
				return "IgnoredSlideTickNote";

			noteType = note.critical ? "Critical" : "Normal";
			noteType += note.friction ? "SlideTrace" : "SlideStart";
		}
		else if (note.getType() == NoteType::HoldMid)
		{
			const HoldNote& parent = score.holdNotes.at(note.parentID);
			const HoldStep& step = parent.steps[findHoldStep(parent, note.ID)];

			if (step.type == HoldStepType::Hidden)
				return "IgnoredSlideTickNote";
			else if (step.type == HoldStepType::Skip)
				noteType.append("AttachedSlideTick");
			else
				noteType.append("SlideTick");
		}
		else if (note.getType() == NoteType::HoldEnd)
		{
			if (score.holdNotes.at(note.parentID).endType != HoldNoteType::Normal)
				return "IgnoredSlideTickNote";

			noteType = "SlideEnd" + noteType;
		}
		else if (!note.friction && !note.isFlick())
		{
			noteType.append("Tap");
		}

		noteType.append("Note");
		return noteType;
	}

	std::map<int, std::vector<Note>> SonolusSerializer::buildTickNoteMap(const Score& score)
	{
		std::map<int, std::vector<Note>> allNotes{};
		std::map<int, std::vector<Note>> map{};
		for (const auto& [id, note] : score.notes)
		{
			if (note.getType() == NoteType::Hold)
			{
				const HoldNote& hold = score.holdNotes.at(id);
				if (hold.startType != HoldNoteType::Normal)
					continue;
			}
			
			if (note.getType() == NoteType::HoldEnd)
			{
				const HoldNote& hold = score.holdNotes.at(note.parentID);
				if (hold.endType != HoldNoteType::Normal)
					continue;
			}

			if (note.getType() == NoteType::HoldMid)
				continue;

			allNotes[note.tick].push_back(note);
		}

		std::copy_if(allNotes.cbegin(), allNotes.cend(), std::inserter(map, map.begin()),
			[](const std::pair<int, std::vector<Note>>& x) { return x.second.size() > 1; });

		for (auto& [tick, notes] : map)
		{
			std::sort(notes.begin(), notes.end(),
				[](const Note& a, const Note& b) { return a.lane < b.lane; });
		}

		return map;
	}

	std::string SonolusSerializer::getSlideConnectorArchetype(const Score& score, const HoldNote& hold)
	{
		std::string archetype = score.notes.at(hold.start.ID).critical ? "Critical" : "Normal";	
		if (!hold.isGuide())
		{
			archetype.append("Active");
		}

		archetype.append("SlideConnector");
		return archetype;
	}

	std::string SonolusSerializer::getParentSlideConnector(const std::unordered_map<std::string, std::string>& connections, const std::string& slideKey)
	{
		std::string head = slideKey;
		auto it = connections.find(head);
		while (it != connections.end() && it->second != head)
		{
			head = it->second;
			it = connections.find(head);
		}

		return head;
	}

	std::string SonolusSerializer::getLastSlideConnector(const std::unordered_map<std::string, SlideConnection>& connections, const std::string& slideKey)
	{
		std::string head = slideKey;
		auto it = connections.find(head);
		while (it != connections.end())
		{
			head = it->second.head;
			it = connections.find(it->second.tail);
		}

		return head;
	}

	Note SonolusSerializer::createNote(const json& entity, NoteType type)
	{
		const std::string& archetype = entity["archetype"];
		const json& data = entity["data"];
		const float size = getEntityDataOrDefault<float>(data, "size", 1.5f);

		Note note(type);
		note.ID = nextID++;
		note.tick = beatsToTicks(getEntityDataOrDefault<float>(data, "#BEAT", 0.0f));
		note.width = static_cast<int>(size * 2.0f);
		note.lane = toNativeLane(getEntityDataOrDefault<float>(data, "lane", 0.0f), size);
		note.friction = archetype.find("Trace") != std::string::npos;
		note.critical = archetype.find("Critical") != std::string::npos;
		note.flick = directionToFlick(getEntityDataOrDefault<int>(data, "direction", 2));

		return note;
	}

	NoteType SonolusSerializer::getNoteTypeFromArchetype(const std::string& archetype)
	{
		if (archetype.find("SlideStart") != std::string::npos || archetype.find("SlideTrace") != std::string::npos)
		{
			return NoteType::Hold;
		}
		else if (archetype.find("TickNote") != std::string::npos)
		{
			return NoteType::HoldMid;
		}
		else if (archetype.find("SlideEnd") != std::string::npos)
		{
			return NoteType::HoldEnd;
		}

		return NoteType::Tap;
	}

	void to_json(json& j, const ArchetypeData& data)
	{
		ordered_json dataArray = ordered_json::array();
		for (const auto& entry : data.data)
		{
			if (entry.getType() == DataValueType::Value)
			{
				dataArray.push_back({
					{ "name", entry.getName() },
					{ "value", entry.getNumType() == NumberType::Integer ? entry.getIntValue() : entry.getFloatValue() }
				});
			}
			else if (entry.getType() == DataValueType::Ref)
			{
				dataArray.push_back({ { "name", entry.getName() }, {"ref", entry.getRef()} });
			}
		}

		j["archetype"] = data.archetype;
		j["data"] = dataArray;

		if (!data.name.empty())
			j["name"] = data.name;
	}
}