#include "Application.h"
#include "ScoreSerializeWindow.h"

#include "NativeScoreSerializer.h"
#include "SusSerializer.h"
#include "SonolusSerializer.h"

#include "Localization.h"
#include "ApplicationConfiguration.h"
#include "Colors.h"
#include "ScoreEditor.h"

namespace MikuMikuWorld
{
	static constexpr size_t FORMAT_COUNT = static_cast<size_t>(SerializeFormat::FormatCount);

	static const std::array<std::string, FORMAT_COUNT> FORMAT_NAMES = {
		IO::formatString("%s (%s)", IO::mmwsFilter.filterName.c_str(), MMWS_EXTENSION),
		IO::formatString("%s (%s)", IO::susFilter.filterName.c_str(), SUS_EXTENSION),
		IO::lvlDatFilter.filterName,
	};

	DefaultScoreSerializeController::DefaultScoreSerializeController(Score score)
	{
		this->score = std::move(score);
		// Check if user selected a format before, we default to serialize it
		// If they cancel, continue as normal and ask for a new format
		if (static_cast<int>(SerializeFormat::SusFormat) <= config.lastSelectedExportIndex
			&& config.lastSelectedExportIndex < static_cast<int>(SerializeFormat::FormatCount)
			&& openFileDialog(static_cast<SerializeFormat>(config.lastSelectedExportIndex), this->filename))
				createSerializer();
	}

	DefaultScoreSerializeController::DefaultScoreSerializeController(Score score, const std::string& filename)
	{
		this->score = std::move(score);
		this->filename = filename;
		createSerializer();
	}

    bool DefaultScoreSerializeController::openFileDialog(SerializeFormat format, std::string &filename)
	{
        IO::FileDialog fileDialog{};
		fileDialog.title = "Export Score";
		fileDialog.filters = { getFormatFilter(format) };
		fileDialog.defaultExtension = getFormatDefaultExtension(format);
		fileDialog.parentWindowHandle = Application::windowState.windowHandle;

		if (fileDialog.saveFile() == IO::FileDialogResult::OK)
		{
			filename = fileDialog.outputFilename;
			return true;
		}
		else
			return false;
    }

	void DefaultScoreSerializeController::createSerializer() {
		SerializeFormat format = toSerializeFormat(filename);
		switch (format) {
		case SerializeFormat::NativeFormat:
			serializer = std::make_unique<NativeScoreSerializer>();
			break;
		case SerializeFormat::SusFormat:
			serializer = std::make_unique<SusSerializer>();
			break;
		case SerializeFormat::LvlDataFormat:
			serializer = std::make_unique<SonolusSerializer>(std::make_unique<PySekaiEngine>(), IO::endsWith(filename, GZ_JSON_EXTENSION));
		break;
		default:
			errorMessage = "No serializer found!";
			serializer.reset();
			return;
		}

		if (format == SerializeFormat::NativeFormat)
			scoreFilename = filename;
	}

	SerializeResult DefaultScoreSerializeController::update()
	{
		if (serializer && !filename.empty())
		{
			try
			{
				serializer->serialize(score, filename);
				serializer.reset();
				return SerializeResult::SerializeSuccess;
			}
			catch (const PartialScoreSerializeError& partial_err)
			{
				errorMessage = partial_err.what();
				return SerializeResult::PartialSerializeSuccess;
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
		else if (filename.empty())
		{
			SerializeResult result = SerializeResult::None;
			if (!ImGui::IsPopupOpen("###serializer_picker"))
				ImGui::OpenPopup("###serializer_picker");

			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSizeConstraints({ 250, 150 }, { FLT_MAX, FLT_MAX });
			ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize / 2.5f, ImGuiCond_Always);
			ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
			if (ImGui::BeginPopupModal(APP_NAME "###serializer_picker", NULL, ImGuiWindowFlags_NoResize))
			{
				const ImGuiStyle& style = ImGui::GetStyle();
				ImGui::Text(getString("export_as_file_format"));
				ImVec2 avail = ImGui::GetContentRegionAvail();
				float btnWidth = avail.x / 2 - style.ItemSpacing.x * 2, btnHeight = ImGui::GetFrameHeight();
				if (ImGui::BeginListBox("###score_format_list", { avail.x, avail.y - btnHeight - style.ItemSpacing.y * 2 }))
				{
					for (int i = static_cast<int>(SerializeFormat::SusFormat); i < FORMAT_NAMES.size(); ++i)
					{
						bool isSelected = (static_cast<int>(selectedFormat) == i);
						if (isSelected)
						{
							const ImVec4 color = UI::accentColors[config.accentColor];
							const ImVec4 darkColor = generateDarkColor(color);
							const ImVec4 lightColor = generateHighlightColor(color);
							ImGui::PushStyleColor(ImGuiCol_Header, color);
							ImGui::PushStyleColor(ImGuiCol_HeaderHovered, darkColor);
							ImGui::PushStyleColor(ImGuiCol_HeaderActive, lightColor);
						}
						if (ImGui::Selectable(FORMAT_NAMES[i].data(), isSelected))
							selectedFormat = static_cast<SerializeFormat>(i);
						if (isSelected)
						{
							ImGui::PopStyleColor(3);
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndListBox();
				}

				ImGui::BeginDisabled(!isValidFormat(selectedFormat));
				if (ImGui::Button(getString("select"), { btnWidth, btnHeight }))
				{
					if (openFileDialog(selectedFormat, this->filename))
					{
						createSerializer();
						ImGui::CloseCurrentPopup();
						config.lastSelectedExportIndex = static_cast<int>(selectedFormat);
					}
				}
				ImGui::EndDisabled();
				ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 2);
				if (ImGui::Button(getString("cancel"), { btnWidth, btnHeight }))
				{
					ImGui::CloseCurrentPopup();
					result = SerializeResult::Cancel;
					config.lastSelectedExportIndex = 0;
				}
				ImGui::EndPopup();
			}
			return result;
		}
		else
			// Has filename but no valid serializer
			return SerializeResult::Error;
	}

	DefaultScoreDeserializeController::DefaultScoreDeserializeController(const std::string& filename)
	{
		this->filename = filename;
		SerializeFormat format = toSerializeFormat(filename);
		if (isValidFormat(format))
		{
			selectedFormat = format;
			createDeserializer();
		}
	}

	void DefaultScoreDeserializeController::createDeserializer()
	{
		switch (selectedFormat)
		{
		case SerializeFormat::NativeFormat:
			deserializer = std::make_unique<NativeScoreSerializer>();
			break;
		case SerializeFormat::SusFormat:
			deserializer = std::make_unique<SusSerializer>();
			break;
		case SerializeFormat::LvlDataFormat:
			deserializer = std::make_unique<SonolusSerializer>(std::make_unique<PySekaiEngine>());
			break;
		default:
			deserializer.reset();
			filename.clear();
			errorMessage = "No deserializer found!";
			return;
		}

		if (selectedFormat == SerializeFormat::NativeFormat)
			this->scoreFilename = filename;
	}

	SerializeResult DefaultScoreDeserializeController::update()
	{
		if (deserializer && !filename.empty())
		{
			try
			{
				score = deserializer->deserialize(filename);
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
				errorMessage = IO::formatString(
					"%s\n"
				    "%s: %s\n"
				    "%s: %s",
				    getString("error_load_score_file"),
					getString("score_file"), filename.c_str(),
					getString("error"), error.what());
				return SerializeResult::Error;
			}
		}
		else if (!filename.empty())
		{
			SerializeResult result = SerializeResult::None;
			if (!ImGui::IsPopupOpen("###deserializer_picker"))
				ImGui::OpenPopup("###deserializer_picker");

			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSizeConstraints({ 450, 300 }, { FLT_MAX, FLT_MAX });
			ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize / 2.f, ImGuiCond_Always);
			ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
			if (ImGui::BeginPopupModal(APP_NAME "###deserializer_picker", NULL, ImGuiWindowFlags_NoResize))
			{
				const ImGuiStyle& style = ImGui::GetStyle();
				ImGui::Text(
					"%s '%s'\n"
					"%s",
					getString("unknown_file_format"), filename.c_str(),
					getString("open_as_file_format")
				);
				ImVec2 avail = ImGui::GetContentRegionAvail();
				float btnWidth = avail.x / 2 - style.ItemSpacing.x * 2, btnHeight = ImGui::GetFrameHeight();

				if (ImGui::BeginListBox("###score_format_list", { avail.x, avail.y - btnHeight - style.ItemSpacing.y * 2 }))
				{
					// Temporarily disable lvldata format
					for (int i = 0, n = static_cast<int>(SerializeFormat::LvlDataFormat); i < n; ++i)
					{
						bool isSelected = (static_cast<int>(selectedFormat) == i);
						if (isSelected)
						{
							const ImVec4 color = UI::accentColors[config.accentColor];
							const ImVec4 darkColor = generateDarkColor(color);
							const ImVec4 lightColor = generateHighlightColor(color);
							ImGui::PushStyleColor(ImGuiCol_Header, color);
							ImGui::PushStyleColor(ImGuiCol_HeaderHovered, darkColor);
							ImGui::PushStyleColor(ImGuiCol_HeaderActive, lightColor);
						}
						if (ImGui::Selectable(FORMAT_NAMES[i].data(), isSelected))
							selectedFormat = static_cast<SerializeFormat>(i);
						if (isSelected)
						{
							ImGui::PopStyleColor(3);
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndListBox();
				}

				ImGui::BeginDisabled(!isValidFormat(selectedFormat));
				if (ImGui::Button(getString("select"), { btnWidth, btnHeight }))
				{
					createDeserializer();
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndDisabled();
				ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 2);
				if (ImGui::Button(getString("cancel"), { btnWidth, btnHeight }))
				{
					result = SerializeResult::Cancel;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			return result;
		}
		else
			return SerializeResult::Error;
	}

	void ScoreSerializeWindow::update(ScoreEditor& editor, ScoreContext& context, ScoreEditorTimeline& timeline)
	{
		if (!controller)
			return;
		switch (controller->update())
		{
		case SerializeResult::PartialSerializeSuccess:
			IO::messageBox(
				APP_NAME,
				controller->getErrorMessage(),
				IO::MessageBoxButtons::Ok,
				IO::MessageBoxIcon::Warning,
				Application::windowState.windowHandle
			);
			controller.reset();
			break;
		case SerializeResult::SerializeSuccess:
			IO::messageBox(
				APP_NAME,
				IO::formatString(getString("export_successful")),
				IO::MessageBoxButtons::Ok,
				IO::MessageBoxIcon::Information,
				Application::windowState.windowHandle
			);
			controller.reset();
			break;
		case SerializeResult::PartialDeserializeSuccess:
			IO::messageBox(
				APP_NAME,
				controller->getErrorMessage(),
				IO::MessageBoxButtons::Ok,
				IO::MessageBoxIcon::Warning,
				Application::windowState.windowHandle
			);
			[[fallthrough]];
		case SerializeResult::DeserializeSuccess:
			context.clearSelection();
			context.history.clear();
			context.score = std::move(controller->getScore());
			context.workingData = EditorScoreData(context.score.metadata, controller->getScoreFilename());

			editor.asyncLoadMusic(context.workingData.musicFilename);
			context.audio.setMusicOffset(0, context.workingData.musicOffset);

			context.scoreStats.calculateStats(context.score);
			context.scorePreviewDrawData.calculateDrawData(context.score);
			timeline.calculateMaxOffsetFromScore(context.score);

			UI::setWindowTitle((context.workingData.filename.size() ? IO::File::getFilename(context.workingData.filename) : windowUntitled));
			context.upToDate = true;
			if (!controller->getFilename().empty())
				editor.updateRecentFilesList(controller->getFilename());

			controller.reset();
			break;
		default:
		case SerializeResult::Error:
			IO::messageBox(
				APP_NAME,
				controller->getErrorMessage(),
				IO::MessageBoxButtons::Ok,
				IO::MessageBoxIcon::Error,
				Application::windowState.windowHandle
			);
			controller.reset();
			break;
		case SerializeResult::Cancel:
			controller.reset();
			break;
		case SerializeResult::None:
			break;
		}
	}

	void ScoreSerializeWindow::serialize(const ScoreContext & context)
	{
		Score score = context.score;
		score.metadata = context.workingData.toScoreMetadata();
		controller = std::make_unique<DefaultScoreSerializeController>(std::move(score));
	}

	void ScoreSerializeWindow::serialize(const ScoreContext &context, const std::string &filename)
	{
		Score score = context.score;
		score.metadata = context.workingData.toScoreMetadata();
		controller = std::make_unique<DefaultScoreSerializeController>(std::move(score), filename);
	}

	void ScoreSerializeWindow::deserialize(const std::string& filename)
	{
		controller = std::make_unique<DefaultScoreDeserializeController>(filename);
	}
}