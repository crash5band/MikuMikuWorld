#include "ScoreDiagnostics.h"
#include "IO.h"
#include "Constants.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>

namespace MikuMikuWorld
{
	namespace
	{
		struct MutableHoldState
		{
			HoldNoteType startType{ HoldNoteType::Normal };
			HoldNoteType endType{ HoldNoteType::Normal };
			std::unordered_map<int, HoldStepType> stepTypes{};
		};

		struct HoldNodeRef
		{
			int holdID{};
			int noteID{};
		};

		int gcd(int left, int right)
		{
			int a = std::abs(left);
			int b = std::abs(right);
			while (b != 0)
			{
				int t = a % b;
				a = b;
				b = t;
			}
			return std::max(a, 1);
		}

		bool noteExists(const Score& score, int noteID)
		{
			return score.notes.find(noteID) != score.notes.end();
		}

		bool notesOverlapWithSharedSide(const Note& left, const Note& right)
		{
			if (left.tick != right.tick)
				return false;

			const int leftStart = left.lane;
			const int leftEnd = left.lane + left.width;
			const int rightStart = right.lane;
			const int rightEnd = right.lane + right.width;
			return leftStart < rightEnd && rightStart < leftEnd &&
				(leftStart == rightStart || leftEnd == rightEnd);
		}

		bool isSolidTraceTap(const Note& note)
		{
			return note.getType() == NoteType::Tap && note.friction;
		}

		std::string makeTrackText(const Note& note)
		{
			const int start = note.lane + 1;
			const int end = note.lane + note.width;
			return note.width > 1 ?
				IO::formatString("Lane %d-%d", start, end) :
				IO::formatString("Lane %d", start);
		}

		int getMeasure(const Score& score, int tick)
		{
			if (score.timeSignatures.empty())
				return 0;

			return accumulateMeasures(tick, TICKS_PER_BEAT, score.timeSignatures);
		}

		std::string makePositionText(const Score& score, int tick, int measure)
		{
			if (score.timeSignatures.empty())
				return IO::formatString("M%03d Tick %d", measure, std::max(tick, 0));

			const int measureTick = measureToTicks(measure, TICKS_PER_BEAT, score.timeSignatures);
			const int tickInMeasure = std::max(0, tick - measureTick);

			if (tickInMeasure == 0)
				return IO::formatString("M%03d Start", measure);

			int signatureMeasure = findTimeSignature(measure, score.timeSignatures);
			auto it = score.timeSignatures.find(signatureMeasure);
			if (it == score.timeSignatures.end())
				return IO::formatString("M%03d Tick %d", measure, tickInMeasure);

			const int ticksPerMeasure = static_cast<int>(beatsPerMeasure(it->second) * TICKS_PER_BEAT);
			if (ticksPerMeasure <= 0)
				return IO::formatString("M%03d Tick %d", measure, tickInMeasure);

			const int div = gcd(tickInMeasure, ticksPerMeasure);
			return IO::formatString("M%03d %d/%d", measure, tickInMeasure / div, ticksPerMeasure / div);
		}

		std::string noteKindLabel(const Note& note)
		{
			if (note.getType() == NoteType::Hold)
				return "Hold Start";

			if (note.getType() == NoteType::HoldMid)
				return "Hold Relay";

			if (note.getType() == NoteType::HoldEnd)
				return note.isFlick() ? "Hold Flick End" : "Hold End";

			if (note.friction)
				return note.critical ? "Critical Trace" : "Trace";

			if (note.isFlick())
				return note.critical ? "Critical Flick" : "Flick";

			if (note.critical)
				return "Critical Tap";

			return "Tap";
		}

		std::unordered_map<int, MutableHoldState> buildHoldStates(const Score& score)
		{
			std::unordered_map<int, MutableHoldState> states;
			for (const auto& [holdID, hold] : score.holdNotes)
			{
				MutableHoldState state{};
				state.startType = hold.startType;
				state.endType = hold.endType;
				for (const HoldStep& step : hold.steps)
					state.stepTypes[step.ID] = step.type;

				states[holdID] = state;
			}
			return states;
		}

		bool isVisibleHoldMid(
			const Note& note,
			const std::unordered_map<int, MutableHoldState>& holdStates)
		{
			if (note.getType() != NoteType::HoldMid)
				return false;

			auto holdIt = holdStates.find(note.parentID);
			if (holdIt == holdStates.end())
				return false;

			const MutableHoldState& state = holdIt->second;
			if (state.startType == HoldNoteType::Guide || state.endType == HoldNoteType::Guide)
				return false;

			auto stepIt = state.stepTypes.find(note.ID);
			return stepIt != state.stepTypes.end() && stepIt->second == HoldStepType::Normal;
		}

		bool isVisibleHoldStart(
			const Note& note,
			const std::unordered_map<int, MutableHoldState>& holdStates)
		{
			if (note.getType() != NoteType::Hold)
				return false;

			auto holdIt = holdStates.find(note.ID);
			return holdIt != holdStates.end() && holdIt->second.startType == HoldNoteType::Normal;
		}

		bool isVisibleNote(
			const Note& note,
			const std::unordered_map<int, MutableHoldState>& holdStates)
		{
			switch (note.getType())
			{
			case NoteType::Tap:
				return true;

			case NoteType::Hold:
			{
				auto holdIt = holdStates.find(note.ID);
				return holdIt != holdStates.end() && holdIt->second.startType == HoldNoteType::Normal;
			}

			case NoteType::HoldEnd:
			{
				auto holdIt = holdStates.find(note.parentID);
				return holdIt != holdStates.end() && holdIt->second.endType == HoldNoteType::Normal;
			}

			case NoteType::HoldMid:
				return isVisibleHoldMid(note, holdStates);
			}

			return false;
		}

		std::string toCategoryText(ScoreDiagnosticCategory category)
		{
			switch (category)
			{
			case ScoreDiagnosticCategory::TapOverlap:
				return "Tap Overlap";
			case ScoreDiagnosticCategory::HoldCoverage:
				return "Hold Coverage";
			case ScoreDiagnosticCategory::VisibleOverlap:
				return "Visible Overlap";
			case ScoreDiagnosticCategory::HoldRelaySameTime:
				return "Hold Relay Collision";
			case ScoreDiagnosticCategory::HoldNodeExactOverlap:
				return "Hold Node Collision";
			}
			return "";
		}

		ScoreDiagnostic makeDiagnostic(
			const Score& score,
			ScoreDiagnosticCategory category,
			ScoreDiagnosticSeverity severity,
			const char* code,
			const std::string& title,
			const std::string& summary,
			const Note& target,
			int targetNoteID,
			int sourceNoteID)
		{
			const int measure = getMeasure(score, target.tick);
			ScoreDiagnostic diagnostic{};
			diagnostic.category = category;
			diagnostic.severity = severity;
			diagnostic.code = code;
			diagnostic.title = title;
			diagnostic.summary = summary;
			diagnostic.categoryText = toCategoryText(category);
			diagnostic.measure = measure;
			diagnostic.tick = target.tick;
			diagnostic.lane = target.lane;
			diagnostic.width = target.width;
			diagnostic.positionText = makePositionText(score, target.tick, measure);
			diagnostic.trackText = makeTrackText(target);
			diagnostic.sourceNoteID = sourceNoteID;
			diagnostic.targetNoteID = targetNoteID;
			return diagnostic;
		}
	}

	std::vector<ScoreDiagnostic> ScoreDiagnostics::analyze(const Score& score)
	{
		std::vector<ScoreDiagnostic> diagnostics;
		diagnostics.reserve(score.notes.size() / 2);

		std::unordered_set<std::string> diagnosticDedup;
		auto addUnique = [&diagnosticDedup, &diagnostics](const std::string& key, ScoreDiagnostic&& item)
		{
			if (diagnosticDedup.find(key) != diagnosticDedup.end())
				return;

			diagnosticDedup.insert(key);
			diagnostics.push_back(std::move(item));
		};

		std::unordered_map<int, MutableHoldState> holdStates = buildHoldStates(score);

		{
			for (const auto& [holdID, hold] : score.holdNotes)
			{
				if (hold.startType == HoldNoteType::Guide || hold.endType == HoldNoteType::Guide)
					continue;

				std::vector<int> stepIDs;
				stepIDs.reserve(hold.steps.size());
				for (const HoldStep& step : hold.steps)
				{
					auto it = score.notes.find(step.ID);
					if (it == score.notes.end() || it->second.getType() != NoteType::HoldMid)
						continue;

					stepIDs.push_back(step.ID);
				}

				std::sort(stepIDs.begin(), stepIDs.end(),
					[&score](int leftID, int rightID)
					{
						const Note& left = score.notes.at(leftID);
						const Note& right = score.notes.at(rightID);
						return left.tick == right.tick ? left.ID < right.ID : left.tick < right.tick;
					});

				for (size_t i = 0; i + 1 < stepIDs.size(); i++)
				{
					const Note& left = score.notes.at(stepIDs[i]);
					const Note& right = score.notes.at(stepIDs[i + 1]);
					if (left.tick != right.tick)
						continue;

					addUnique(
						IO::formatString("relay-%d-%d-%d", holdID, left.ID, right.ID),
						makeDiagnostic(
							score,
							ScoreDiagnosticCategory::HoldRelaySameTime,
							ScoreDiagnosticSeverity::Crash,
							"E1001",
							"Same-tick hold relay nodes may crash preview",
							"Two relay nodes on the same hold share an identical tick. This can crash official preview.",
							right,
							right.ID,
							left.ID
						));
				}
			}
		}

		{
			std::unordered_map<std::string, std::vector<HoldNodeRef>> buckets;
			for (const auto& [holdID, hold] : score.holdNotes)
			{
				if (hold.startType == HoldNoteType::Guide || hold.endType == HoldNoteType::Guide)
					continue;

				const std::array<int, 2> edgeNodes{ hold.start.ID, hold.end };
				for (int noteID : edgeNodes)
				{
					if (!noteExists(score, noteID))
						continue;

					const Note& note = score.notes.at(noteID);
					buckets[IO::formatString("%d-%d-%d", note.tick, note.lane, note.width)].push_back({ holdID, noteID });
				}

				for (const HoldStep& step : hold.steps)
				{
					if (!noteExists(score, step.ID))
						continue;

					const Note& note = score.notes.at(step.ID);
					if (note.getType() != NoteType::HoldMid)
						continue;

					buckets[IO::formatString("%d-%d-%d", note.tick, note.lane, note.width)].push_back({ holdID, step.ID });
				}
			}

			for (const auto& [key, refs] : buckets)
			{
				(void)key;
				if (refs.size() < 2)
					continue;

				for (size_t i = 0; i + 1 < refs.size(); i++)
				{
					for (size_t j = i + 1; j < refs.size(); j++)
					{
						if (refs[i].holdID == refs[j].holdID)
							continue;

						const Note& first = score.notes.at(refs[i].noteID);
						const Note& second = score.notes.at(refs[j].noteID);
						const int firstID = std::min(first.ID, second.ID);
						const int secondID = std::max(first.ID, second.ID);
						addUnique(
							IO::formatString("node-%d-%d", firstID, secondID),
							makeDiagnostic(
								score,
								ScoreDiagnosticCategory::HoldNodeExactOverlap,
								ScoreDiagnosticSeverity::Crash,
								"E1002",
								"Exact overlap between hold nodes may crash preview",
								"Nodes from different holds share the same tick, lane, and width. This can crash official preview.",
								second,
								second.ID,
								first.ID
							));
					}
				}
			}
		}

		std::unordered_set<int> activeNoteIDs;
		for (const auto& [noteID, note] : score.notes)
		{
			(void)note;
			activeNoteIDs.insert(noteID);
		}

		{
			std::vector<int> tapIDs;
			for (const auto& [noteID, note] : score.notes)
			{
				if (note.getType() == NoteType::Tap && !note.friction)
					tapIDs.push_back(noteID);
			}

			std::sort(tapIDs.begin(), tapIDs.end(), [&score](int leftID, int rightID)
				{
					const Note& left = score.notes.at(leftID);
					const Note& right = score.notes.at(rightID);
					return left.tick == right.tick ? left.ID < right.ID : left.tick < right.tick;
				});

			std::vector<int> keptTapIDs;
			keptTapIDs.reserve(tapIDs.size());
			for (int noteID : tapIDs)
			{
				const Note& candidate = score.notes.at(noteID);
				int keeperID = -1;
				for (int existingID : keptTapIDs)
				{
					const Note& existing = score.notes.at(existingID);
					if (existing.tick != candidate.tick)
						continue;

					if (notesOverlapWithSharedSide(existing, candidate))
					{
						keeperID = existingID;
						break;
					}
				}

				if (keeperID == -1)
				{
					keptTapIDs.push_back(noteID);
					continue;
				}

				const Note& keeper = score.notes.at(keeperID);
				addUnique(
					IO::formatString("tap-%d-%d", keeperID, noteID),
					makeDiagnostic(
						score,
						ScoreDiagnosticCategory::TapOverlap,
						ScoreDiagnosticSeverity::Warning,
						"W2001",
						"Overlapping note may be ignored",
						IO::formatString(
							"%s overlaps with earlier %s at the same tick and shared edge. The later note may be ignored.",
							noteKindLabel(candidate).c_str(),
							noteKindLabel(keeper).c_str()),
						candidate,
						candidate.ID,
						keeper.ID
					));
				activeNoteIDs.erase(noteID);
			}
		}

		{
			std::vector<int> noteIDs;
			noteIDs.reserve(score.notes.size());
			for (const auto& [noteID, note] : score.notes)
			{
				(void)note;
				noteIDs.push_back(noteID);
			}

			for (int sourceID : noteIDs)
			{
				if (activeNoteIDs.find(sourceID) == activeNoteIDs.end())
					continue;

				const Note& source = score.notes.at(sourceID);
				const bool visibleMid = isVisibleHoldMid(source, holdStates);
				const bool visibleStart = isVisibleHoldStart(source, holdStates);
				if ((!visibleMid && !visibleStart) || source.friction)
					continue;

				for (int targetID : noteIDs)
				{
					if (sourceID == targetID || activeNoteIDs.find(targetID) == activeNoteIDs.end())
						continue;

					const Note& target = score.notes.at(targetID);
					if (!notesOverlapWithSharedSide(source, target))
						continue;

					const bool shouldSuppress = visibleMid ?
						(target.getType() == NoteType::HoldEnd || target.isFlick() || target.friction) :
						target.isFlick();

					if (!shouldSuppress)
						continue;

					addUnique(
						IO::formatString("cover-%d-%d", sourceID, targetID),
						makeDiagnostic(
							score,
							ScoreDiagnosticCategory::HoldCoverage,
							ScoreDiagnosticSeverity::Warning,
							"W2002",
							"Hold covers another note",
							IO::formatString(
								"%s covers %s at the same tick. The covered note may be hidden.",
								noteKindLabel(source).c_str(),
								noteKindLabel(target).c_str()),
							target,
							target.ID,
							source.ID
						));

					if (target.getType() == NoteType::Tap)
					{
						activeNoteIDs.erase(targetID);
					}
					else if (target.getType() == NoteType::HoldEnd)
					{
						auto holdIt = holdStates.find(target.parentID);
						if (holdIt != holdStates.end())
							holdIt->second.endType = HoldNoteType::Hidden;
					}
				}
			}
		}

		{
			std::vector<int> noteIDs;
			noteIDs.reserve(score.notes.size());
			for (const auto& [noteID, note] : score.notes)
			{
				if (activeNoteIDs.find(noteID) != activeNoteIDs.end())
					noteIDs.push_back(noteID);

				(void)note;
			}

			std::sort(noteIDs.begin(), noteIDs.end(), [&score](int leftID, int rightID)
				{
					const Note& left = score.notes.at(leftID);
					const Note& right = score.notes.at(rightID);
					return left.tick == right.tick ? left.ID < right.ID : left.tick < right.tick;
				});

			std::vector<int> keptIDs;
			keptIDs.reserve(noteIDs.size());

			for (int noteID : noteIDs)
			{
				if (activeNoteIDs.find(noteID) == activeNoteIDs.end())
					continue;

				const Note& note = score.notes.at(noteID);
				if (!isVisibleNote(note, holdStates))
					continue;

				int keeperID = -1;
				for (int keptID : keptIDs)
				{
					if (activeNoteIDs.find(keptID) == activeNoteIDs.end())
						continue;

					const Note& kept = score.notes.at(keptID);
					if (!isVisibleNote(kept, holdStates))
						continue;

					const bool keptSolidTrace = isSolidTraceTap(kept);
					const bool noteSolidTrace = isSolidTraceTap(note);
					if (keptSolidTrace && noteSolidTrace)
					{
						if (kept.tick != note.tick)
							continue;

						const int keptLeft = kept.lane;
						const int keptRight = kept.lane + kept.width;
						const int noteLeft = note.lane;
						const int noteRight = note.lane + note.width;
						const bool overlaps = keptLeft < noteRight && noteLeft < keptRight;
						if (overlaps && keptLeft == noteLeft)
						{
							keeperID = keptID;
							break;
						}

						continue;
					}

					if ((kept.friction || note.friction) && !(keptSolidTrace && noteSolidTrace))
						continue;

					if (notesOverlapWithSharedSide(kept, note))
					{
						keeperID = keptID;
						break;
					}
				}

				if (keeperID == -1)
				{
					keptIDs.push_back(noteID);
					continue;
				}

				const Note& keeper = score.notes.at(keeperID);
				addUnique(
					IO::formatString("visible-%d-%d", keeperID, noteID),
					makeDiagnostic(
						score,
						ScoreDiagnosticCategory::VisibleOverlap,
						ScoreDiagnosticSeverity::Warning,
						"W2003",
						"Visible overlap hides a note",
						IO::formatString(
							"%s overlaps with earlier %s at the same tick and shared edge. The later note may be hidden.",
							noteKindLabel(note).c_str(),
							noteKindLabel(keeper).c_str()),
						note,
						note.ID,
						keeper.ID
					));

				switch (note.getType())
				{
				case NoteType::Tap:
					activeNoteIDs.erase(noteID);
					break;

				case NoteType::Hold:
				{
					auto holdIt = holdStates.find(note.ID);
					if (holdIt != holdStates.end())
						holdIt->second.startType = HoldNoteType::Hidden;
					break;
				}

				case NoteType::HoldEnd:
				{
					auto holdIt = holdStates.find(note.parentID);
					if (holdIt != holdStates.end())
						holdIt->second.endType = HoldNoteType::Hidden;
					break;
				}

				case NoteType::HoldMid:
				{
					auto holdIt = holdStates.find(note.parentID);
					if (holdIt != holdStates.end())
						holdIt->second.stepTypes[note.ID] = HoldStepType::Hidden;
					break;
				}
				}
			}
		}

		std::sort(diagnostics.begin(), diagnostics.end(),
			[](const ScoreDiagnostic& left, const ScoreDiagnostic& right)
			{
				if (left.severity != right.severity)
					return left.severity == ScoreDiagnosticSeverity::Crash;

				if (left.tick != right.tick)
					return left.tick < right.tick;

				if (left.lane != right.lane)
					return left.lane < right.lane;

				if (left.code != right.code)
					return left.code < right.code;

				return left.targetNoteID < right.targetNoteID;
			});

		return diagnostics;
	}
}
