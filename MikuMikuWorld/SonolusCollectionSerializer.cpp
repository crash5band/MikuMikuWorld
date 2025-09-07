#include "Application.h"
#include "SonolusCollectionSerializer.h"
#include "SonolusSerializer.h"
#include "ApplicationConfiguration.h"
#include "Colors.h"
#include "IO.h"

using namespace nlohmann;
using namespace Sonolus;
namespace fs = std::filesystem;

namespace MikuMikuWorld
{
	constexpr std::array<std::string_view, 2> SUPPORTED_ENGINE = { "pjsekai", "prosekaR" };
	inline static void throwFileNotFound(const std::string& filename)
	{
		throw SonolusSerializingError(IO::formatString("%s '%s' %s", getString("file_not_found_msg1"), filename, getString("file_not_found_msg2")));
	}

	inline static void throwFileCannotOpen(const std::string& filename, const std::string& reason)
	{
		if (reason.empty())
			throw SonolusSerializingError(
				IO::formatString("%s '%s' %s\n%s: %s", getString("error_open_file_msg1"), filename, getString("error_open_file_msg2"), getString("error_reason"), reason.c_str())
			);
		else
			throw SonolusSerializingError(
				IO::formatString("%s '%s' %s", getString("error_open_file_msg1"), filename, getString("error_open_file_msg2"))
			);
	}

	SonolusCollectionSerializer::SonolusCollectionSerializer(const std::string& rootPath)
	{
		fs::path path = fs::weakly_canonical(rootPath);
		fs::path root = path.root_path();
		bool invalidStructure = true;
		while (path != root)
		{
			path = path.parent_path();
			if (path.filename() == "sonolus" && fs::exists(path / "info"))
			{
				invalidStructure = false;
				break;
			}
		}
		if (invalidStructure)
			throw SonolusSerializingError("The file is not inside a valid sonolus directory structure");
		this->rootPath = path.parent_path().u8string();
	}

	SonolusArchiveCollectionSerializer::SonolusArchiveCollectionSerializer(const std::string& archiveFilename)
	{
		if (!IO::File::exists(archiveFilename))
		{
			scpArchive.open(archiveFilename, IO::ArchiveOpenMode::Create | IO::ArchiveOpenMode::Truncate);
			if (!scpArchive.isOpen()) throwFileCannotOpen(archiveFilename, scpArchive.getLastError().getStr());
			createCollection();
		}
		else
		{
			scpArchive.open(archiveFilename);
			if (!scpArchive.isOpen()) throwFileCannotOpen(archiveFilename, scpArchive.getLastError().getStr());
			if (scpArchive.getEntryIndex("sonolus/info") <= 0) throw SonolusSerializingError("Not a valid scp file!");
		}
	}

	void CollectionSerializer::serialize(const Score& score, const std::string& levelName, const std::string& oldLevelName)
	{
		SonolusSerializer scoreSerializer;
		LevelData levelData = scoreSerializer.serialize(score);
		json levelDataJson = levelData;
		std::string serializedData = levelDataJson.dump();
		std::vector<uint8_t> serializedBytes(serializedData.begin(), serializedData.end());
		if (compressLevelData)
			serializedBytes = IO::deflateGzip(serializedBytes);

		LevelItem item, oldItem;
		oldItem.name = oldLevelName;
		item.version = Sonolus::levelVersion;
		item.name = levelName.empty() ? item.name = "mmw-" + hash(serializedBytes) : levelName;
		item.rating = score.metadata.rating;
		item.title = score.metadata.title;
		item.artists = score.metadata.artist;
		item.author = score.metadata.author;
		item.engine = generateEngineItem();
		item.bgm = packageFile(score.metadata.musicFile);
		item.preview = packageFile(score.metadata.musicPreviewFile);
		item.cover = packageFile(score.metadata.jacketFile);
		item.data = packageData(serializedBytes);

		serializeJson(ItemDetail<LevelItem>{item}, "sonolus/levels/" + item.name);
		if (!oldItem.name.empty() && item.name != oldItem.name)
		{
			deserializeJson("sonolus/levels/" + oldItem.name).at("item").get_to(oldItem);
			if (removeOldFiles)
			{
				if (!oldItem.bgm.url.empty() && oldItem.bgm.hash != item.bgm.hash)
					removeFile(urlToEntry(oldItem.bgm.url));
				if (!oldItem.cover.url.empty() && oldItem.cover.hash != item.cover.hash)
					removeFile(urlToEntry(oldItem.cover.url));
				if (!oldItem.data.url.empty() && oldItem.data.hash != item.data.hash)
					removeFile(urlToEntry(oldItem.data.url));
				if (!oldItem.preview.url.empty() && oldItem.preview.hash != item.preview.hash)
					removeFile(urlToEntry(oldItem.preview.url));
			}
			removeFile("sonolus/levels/" + oldItem.name);
		}

		auto levelInfoJson = deserializeJson("sonolus/levels/info");
		updateLevelInfo(levelInfoJson, item, oldItem);
		serializeJson(levelInfoJson, "sonolus/levels/info");

		auto levelListJson = deserializeJson("sonolus/levels/list");
		updateLevelList(levelListJson, item, oldItem);
		serializeJson(levelListJson, "sonolus/levels/list");
	}

	Score CollectionSerializer::deserialize(const std::string& levelName)
	{
		LevelItem item;
		deserializeJson("sonolus/levels/" + levelName).at("item").get_to(item);

		std::vector<uint8_t> bytes = unpackData(urlToEntry(item.data.url));
		if (IO::isGzipCompressed(bytes))
			bytes = IO::inflateGzip(bytes);
		std::string text(bytes.begin(), bytes.end());
		LevelData levelData = json::parse(text).get<LevelData>();

		SonolusSerializer scoreSerializer;
		Score score = scoreSerializer.deserialize(levelData);
		score.metadata.title = item.title;
		score.metadata.artist = item.artists;
		score.metadata.author = item.author;
		score.metadata.rating = item.rating;
		score.metadata.musicFile = unpackFile(urlToEntry(item.bgm.url), ".mp3");
		score.metadata.musicPreviewFile = unpackFile(urlToEntry(item.preview.url), ".mp3");
		score.metadata.jacketFile = unpackFile(urlToEntry(item.cover.url), ".png");
		return score;
	}

	std::string CollectionSerializer::urlToEntry(const std::string& url)
	{
		fs::path path = url;
		if (path.has_root_directory())
			path = path.relative_path();
		return path.generic_u8string();
	}

	std::string CollectionSerializer::entryToUrl(const std::string& entry)
	{
		fs::path path = entry;
		if (!path.has_root_directory())
			path = "/" / path;
		return path.generic_u8string();
	}

	void CollectionSerializer::updateLevelInfo(json& infoJson, const LevelItem& item, const LevelItem& oldItem)
	{
		auto matchItem = [&itemName = oldItem.name](const json& item) { return item.at("name").get<std::string>() == itemName; };
		for (json& section : infoJson.at("sections"))
		{
			json& items = section.at("items");
			auto it = std::find_if(items.begin(), items.end(), matchItem);
			if (it != items.end())
				*it = item;
			else if (section.at("title").get<std::string>() == "#NEWEST")
				items.push_back(item);
		}
	}

	void CollectionSerializer::updateLevelList(json& listJson, const LevelItem& item, const LevelItem& oldItem)
	{
		json& items = listJson.at("items");
		auto it = std::find_if(items.begin(), items.end(), [&itemName = oldItem.name](const json& item) { return item.at("name").get<std::string>() == itemName; });
		if (it != items.end())
			*it = item;
		else
			items.push_back(item);
	}

	void SonolusCollectionSerializer::serializeJson(const json& j, const std::string& entryPath)
	{
		std::string text = j.dump();
		fs::path path = rootPath / entryPath;
		if (!fs::exists(path.parent_path())) fs::create_directories(path.parent_path());
		IO::File file(path, IO::FileMode::Write);
		file.write(text);
		file.flush();
		file.close();
	}

	void SonolusArchiveCollectionSerializer::serializeJson(const json& j, const std::string& entryPath)
	{
		std::string text = j.dump();
		IO::ArchiveFile file = scpArchive.openFile(entryPath);
		if (!file.isOpen()) throwFileCannotOpen(entryPath, scpArchive.getLastError().getStr());
		file.writeAllText(text);
		if (file.getLastError().get() != 0) throw SonolusSerializingError(scpArchive.getLastError().getStr());
		file.close();
	}

	json SonolusCollectionSerializer::deserializeJson(const std::string& entryPath)
	{
		fs::path entry = rootPath / entryPath;
		if (!fs::exists(entry)) throwFileNotFound(entryPath);
		IO::File file(entry, IO::FileMode::Read);
		std::string data = file.readAllText();
		return json::parse(data);
	}

	json SonolusArchiveCollectionSerializer::deserializeJson(const std::string& entryPath)
	{
		int64_t index = scpArchive.getEntryIndex(entryPath, false);
		if (index < 0) throwFileNotFound(entryPath);
		IO::ArchiveFile entryFile = scpArchive.openFile(index);
		if (!entryFile.isOpen()) throwFileCannotOpen(entryPath ,scpArchive.getLastError().getStr());
		std::string data = entryFile.readAllText();
		if (!data.size() && entryFile.getLastError().get() != 0) throw SonolusSerializingError(entryFile.getLastError().getStr());
		return json::parse(data);
	}

	std::pair<std::string, std::string> SonolusCollectionSerializer::packageData(const std::vector<uint8_t>& buffer)
	{
		std::string fileHash = hash(buffer);
		std::string fileEntry = "sonolus/repository/" + fileHash;
		fs::path path = rootPath / fileEntry;
		if (!fs::exists(path.parent_path())) fs::create_directories(path.parent_path());
		IO::File file(path, IO::FileMode::WriteBinary);
		file.writeAllBytes(buffer);
		file.flush();
		file.close();
		return { std::move(fileHash), entryToUrl(fileEntry) };
	}

	std::pair<std::string, std::string> SonolusArchiveCollectionSerializer::packageData(const std::vector<uint8_t>& buffer)
	{
		std::string fileHash = hash(buffer);
		std::string fileEntry = "sonolus/repository/" + fileHash;
		IO::ArchiveFile arcFile = scpArchive.openFile(fileEntry);
		if (!arcFile.isOpen()) throwFileNotFound(fileEntry);
		arcFile.writeAllBytes(buffer);
		if (arcFile.getLastError().get() != 0) throw SonolusSerializingError(arcFile.getLastError().getStr());
		return { fileHash, entryToUrl(fileEntry) };
	}

	std::pair<std::string, std::string> SonolusCollectionSerializer::packageFile(const std::string& filePath)
	{
		if (filePath.empty() || !IO::File::exists(filePath))
			return {};
		IO::File file(filePath, IO::FileMode::ReadBinary);
		std::vector<uint8_t> data = file.readAllBytes();
		file.close();

		fs::path fileSrc = filePath;
		bool isRelativeToRoot = (std::mismatch(fileSrc.begin(), fileSrc.end(), rootPath.begin(), rootPath.end()).second == rootPath.end());
		if (isRelativeToRoot)
		{
			std::string fileHash = hash(data);
			return { fileHash, entryToUrl(fs::relative(fileSrc, rootPath).u8string()) };
		}
		else
			return packageData(data);
	}
	
	std::pair<std::string, std::string> SonolusArchiveCollectionSerializer::packageFile(const std::string& filePath)
	{
		if (filePath.empty() || !IO::File::exists(filePath))
			return {};
		IO::File file(filePath, IO::FileMode::ReadBinary);
		return packageData(file.readAllBytes());
	}

	std::vector<uint8_t> SonolusCollectionSerializer::unpackData(const std::string& entryPath)
	{
		fs::path entry = rootPath / entryPath;
		if (!fs::exists(entry)) throwFileNotFound(entryPath);
		IO::File file(entry, IO::FileMode::ReadBinary);
		return file.readAllBytes();
	}

	std::vector<uint8_t> SonolusArchiveCollectionSerializer::unpackData(const std::string& entryPath)
	{
		int64_t index = scpArchive.getEntryIndex(entryPath, false);
		if (index < 0) throwFileNotFound(entryPath);
		IO::ArchiveFile entryFile = scpArchive.openFile(index);
		if (!entryFile.isOpen()) throwFileCannotOpen(entryPath, scpArchive.getLastError().getStr());
		std::vector<uint8_t> data = entryFile.readAllBytes();
		if (!data.size() && entryFile.getLastError().get() != 0) throw SonolusSerializingError(entryFile.getLastError().getStr());
		return data;
	}

	std::string SonolusCollectionSerializer::unpackFile(const std::string& entryPath, const std::string_view& outputExtension)
	{
		if (entryPath.empty()) return "";
		fs::path filePath = fs::weakly_canonical(rootPath / entryPath);
		if (!fs::exists(filePath)) return "";
		if (filePath.extension() == outputExtension) return filePath.u8string();
		fs::path fileDest = Application::getAppDir() + "extracted\\" + filePath.filename().u8string();
		fileDest.replace_extension(outputExtension);
		if (!fs::copy_file(filePath, fileDest, fs::copy_options::overwrite_existing))
			return "";
		return fileDest.u8string();
	}

	std::string SonolusArchiveCollectionSerializer::unpackFile(const std::string& entryPath, const std::string_view& outputExtension)
	{
		int64_t index;
		if (entryPath.empty() || (index = scpArchive.getEntryIndex(entryPath)) < 0) return {};
		IO::ArchiveFile archiveFile = scpArchive.openFile(index);
		if (!archiveFile.isOpen()) return {};
		std::vector<uint8_t> bytes = archiveFile.readAllBytes();
		archiveFile.close();
		if (archiveFile.getLastError().get() != 0) return {};
		std::filesystem::path filePath = entryPath;
		filePath.replace_extension(outputExtension);
		filePath = Application::getAppDir() + "extracted\\" + filePath.filename().u8string();
		std::filesystem::create_directory(filePath.parent_path());
		IO::File extractedFile(filePath, IO::FileMode::WriteBinary);
		extractedFile.writeAllBytes(bytes);
		extractedFile.flush();
		extractedFile.close();
		return filePath.u8string();
	}

	void SonolusCollectionSerializer::removeFile(const std::string& entryPath)
	{
		fs::remove(rootPath / entryPath);
	}

	void SonolusArchiveCollectionSerializer::removeFile(const std::string& entryPath)
	{
		scpArchive.removeFile(entryPath);
	}

	bool SonolusCollectionSerializer::exists(const std::string& entryPath)
	{
		if (entryPath.empty()) return false;
		fs::path filePath = fs::weakly_canonical(rootPath / entryPath);
		if (!fs::exists(filePath)) return false;
		return true;
	}

	bool SonolusArchiveCollectionSerializer::exists(const std::string& entryPath)
	{
		return !entryPath.empty() && scpArchive.getEntryIndex(entryPath) >= 0;
	}

	EngineItem CollectionSerializer::generateEngineItem()
	{
		std::string enginePath = Application::getAppDir() + "res\\scp\\" + "Engine.scp";
		IO::Archive archive(enginePath, IO::ArchiveOpenMode::Read);
		if (!archive.isOpen())
			throw SonolusSerializingError(IO::formatString("Failed to open '%s'.\nReason: %s", enginePath, archive.getLastError().getStr()));

		int64_t index = archive.getEntryIndex("sonolus/engines/" + engineName);
		if (index < 0)
			throw SonolusSerializingError(IO::formatString("The engine '%s' was not found or supported.", engineName));

		IO::ArchiveFile engineDetailFile = archive.openFile(index);
		if (!engineDetailFile.isOpen())
			throw SonolusSerializingError(archive.getLastError().getStr());

		std::string data = engineDetailFile.readAllText();
		if (!data.size() && engineDetailFile.getLastError().get() != 0)
			throw SonolusSerializingError(engineDetailFile.getLastError().getStr());

		auto engineItem = json::parse(data).at("item").template get<EngineItem>();
		return engineItem;
	}

	void CollectionSerializer::createCollection()
	{
		json serverPackageJson;
		serverPackageJson["shouldUpdate"] = true;
		serializeJson(serverPackageJson, "sonolus/package");

		json serverInfoJson;
		serverInfoJson["title"] = APP_NAME " Collection";
		serverInfoJson["buttons"] = {
			{ { "type", "post" } },
			{ { "type", "playlist" } },
			{ { "type", "level" } },
			{ { "type", "replay" } },
			{ { "type", "skin" } },
			{ { "type", "background" } },
			{ { "type", "effect" } },
			{ { "type", "particle" } },
			{ { "type", "engine" } },
			{ { "type", "configuration" } },
		};
		serverInfoJson["configuration"] = { { "options", nlohmann::json::array() } };
		serializeJson(serverInfoJson, "sonolus/info");

		json levelInfoJson;
		levelInfoJson["sections"].push_back(ItemSection<LevelItem>{"#NEWEST"});
		serializeJson(levelInfoJson, "sonolus/levels/info");

		json levelListJson;
		levelListJson["pageCount"] = 1;
		levelListJson["items"] = json::array();
		serializeJson(levelListJson, "sonolus/levels/list");
	}
	
	SonolusSerializeDialog::SonolusSerializeDialog(Score score, const std::string& filename)
	{
		isSerializing = true;
		this->score = std::move(score);
		std::vector<LevelItem> levels;
		try
		{
			std::string fileExt = IO::File::getFileExtension(filename);
			std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);
			if (fileExt == SCP_EXTENSION)
				serializer = std::make_unique<SonolusArchiveCollectionSerializer>(filename);
			else
				serializer = std::make_unique<SonolusCollectionSerializer>(filename);
			serializer->deserializeJson("sonolus/levels/list").at("items").get_to(levels);
		}
		catch (std::exception& ex)
		{
			errorMessage = ex.what();
			serializer.reset();
			return;
		}
		levelInfos.reserve(levels.size());
		for (auto& level : levels)
		{
			levelInfos.push_back({
				std::move(level.name),
				std::move(level.title),
				SonolusLevelMetadata::None
			});
		}
		levelInfos.push_back({ "", "Insert a new level...", SonolusLevelMetadata::None });
	}

	SonolusSerializeDialog::SonolusSerializeDialog(const std::string& filename)
	{
		isSerializing = false;
		std::vector<LevelItem> levels;
		try
		{
			if (!IO::File::exists(filename)) throwFileNotFound(filename);

			std::string fileExt = IO::File::getFileExtension(filename);
			std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);
			if (fileExt == SCP_EXTENSION)
				serializer = std::make_unique<SonolusArchiveCollectionSerializer>(filename);
			else
				serializer = std::make_unique<SonolusCollectionSerializer>(filename);
			serializer->deserializeJson("sonolus/levels/list").at("items").get_to(levels);
			if (levels.size() == 0)
			{
				errorMessage = "No levels found in the collection!";
				serializer.reset();
				return;
			}
		}
		catch (std::exception& ex)
		{
			errorMessage = ex.what();
			serializer.reset();
		}

		for (auto& level : levels)
		{
			bool isEngineSupported = std::any_of(std::begin(SUPPORTED_ENGINE), std::end(SUPPORTED_ENGINE), [&](const std::string_view& str) { return str == level.engine.name; });
			bool isLevelDataIncluded = serializer->exists(CollectionSerializer::urlToEntry(level.data.url));
			SonolusLevelMetadata::Flag flag = isEngineSupported ? SonolusLevelMetadata::None : SonolusLevelMetadata::EngineNotRegconized;
			flag = static_cast<SonolusLevelMetadata::Flag>(flag | (isLevelDataIncluded ? SonolusLevelMetadata::None : SonolusLevelMetadata::LevelDataMissing));
			levelInfos.push_back({ std::move(level.name), std::move(level.title), flag });
		}
	}

	SerializeDialogResult SonolusSerializeDialog::update()
	{
		if (!serializer)
			return SerializeDialogResult::Error;
		SerializeDialogResult result = SerializeDialogResult::None;
		if (updateFileSelector(result) && result == SerializeDialogResult::None)
		{
			if (isSerializing)
			{
				try
				{
					serializer->serialize(score, insertLevelName, levelInfos[selectedItem].name);
					return SerializeDialogResult::SerializeSuccess;
				}
				catch (const std::exception& err)
				{
					errorMessage = IO::formatString(
						"%s\n"
						"%s: %s",
						getString("error_save_score_file"),
						getString("error"), err.what()
					);
					return SerializeDialogResult::Error;
				}
			}
			else
			{
				int nextIdBackup = nextID;
				try
				{
					resetNextID();
					score = serializer->deserialize(levelInfos[selectedItem].name);
					return SerializeDialogResult::DeserializeSuccess;
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
					return SerializeDialogResult::Error;
				}
			}
		}
		return result;
	}

	bool SonolusSerializeDialog::updateFileSelector(SerializeDialogResult& result)
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
		if (ImGui::BeginPopupModal(IO::formatString(APP_NAME " - %s###scp_menu", getString("sonolus_collection")).c_str(), NULL, ImGuiWindowFlags_NoResize))
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImGui::Text(getString("select_level"));
			if (isSerializing)
			{
				ImGui::BeginDisabled(selectedItem < 0);
				ImGui::InputTextWithHint("##lvl_id_input", getString("level_id_input_hint"), &insertLevelName, ImGuiInputTextFlags_CharsNoBlank);
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
					bool haveIssue = level.flags != SonolusLevelMetadata::None;
					bool drawToolTip = false;
					std::string label = "";
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
					if (level.flags & SonolusLevelMetadata::EngineNotRegconized)
						label = IO::formatString("%s" APP_NAME "%s", getString("level_data_incompat_msg1"), getString("level_data_incompat_msg2"));
					if (level.flags & SonolusLevelMetadata::LevelDataMissing)
						label = getString("level_data_not_found");
					if (haveIssue)
					{
						ImGui::SameLine(availWidth - 25);
						ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), ICON_FA_EXCLAMATION_TRIANGLE);
					}
					ImGui::PopID();
					ImGui::EndDisabled();
					if (haveIssue && drawToolTip)
					{
						float txtWidth = ImGui::CalcTextSize(label.c_str()).x + (ImGui::GetStyle().WindowPadding.x * 2);
						ImGui::SetNextWindowSize({ 250, -1 });
						ImGui::BeginTooltipEx(ImGuiTooltipFlags_OverridePreviousTooltip, ImGuiWindowFlags_NoResize);
						ImGui::TextWrapped(label.c_str());
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
			if (ImGui::Button(getString("select"), { btnWidth, btnHeight }))
			{
				ImGui::EndDisabled();
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return true;
			}
			ImGui::EndDisabled();
			ImGui::SetCursorPos(btnBarPos + ImVec2{ 0, btnHeight + style.ItemSpacing.y });
			if (ImGui::Button(getString("cancel"), { btnWidth, btnHeight }))
			{
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				result = SerializeDialogResult::Cancel;
				return true;
			}
			ImGui::EndPopup();
		}
		return false;
	}

}