#include "Application.h"
#include "ScoreSerializeWindow.h"

#include "NativeScoreSerializer.h"
#include "SusSerializer.h"
#include "SonolusSerializer.h"
#include "SonolusCollectionSerializer.h"

#include "Localization.h"
#include "ApplicationConfiguration.h"
#include "Colors.h"
#include "ScoreEditor.h"

namespace MikuMikuWorld
{
	void ScoreSerializeWindow::update(ScoreEditor& editor, ScoreContext& context, ScoreEditorTimeline& timeline)
	{
		if (dialog)
		{
			switch (dialog->update())
			{
			case SerializeDialogResult::SerializeSuccess:
				IO::messageBox(
					APP_NAME,
					IO::formatString(getString("export_successful")),
					IO::MessageBoxButtons::Ok,
					IO::MessageBoxIcon::Information,
					Application::windowState.windowHandle
				);
				dialog.reset();
				break;
			case SerializeDialogResult::DeserializeSuccess:
				context.clearSelection();
				context.history.clear();
				context.score = std::move(dialog->getScore());
				context.workingData = EditorScoreData(context.score.metadata, dialog->getScoreFilename());

				editor.asyncLoadMusic(context.workingData.musicFilename);
				context.audio.setMusicOffset(0, context.workingData.musicOffset);

				context.scoreStats.calculateStats(context.score);
				context.scorePreviewDrawData.calculateDrawData(context.score);
				timeline.calculateMaxOffsetFromScore(context.score);

				UI::setWindowTitle((context.workingData.filename.size() ? IO::File::getFilename(context.workingData.filename) : windowUntitled));
				context.upToDate = true;
				dialog.reset();
				break;
			default:
			case SerializeDialogResult::Error:
				IO::messageBox(
					APP_NAME,
					dialog->getErrorMessage(),
					IO::MessageBoxButtons::Ok,
					IO::MessageBoxIcon::Error,
					Application::windowState.windowHandle
				);
				dialog.reset();
				break;
			case SerializeDialogResult::Cancel:
				dialog.reset();
				break;
			case SerializeDialogResult::None:
				break;
			}
		}
		else
		{
			updateDeserializerPicker();
		}
	}

	void ScoreSerializeWindow::openSerializeDialog(const ScoreContext &context, const std::string &filename)
	{
		Score score = context.score;
		score.metadata = context.workingData.toScoreMetadata();
		std::string extension = IO::File::getFileExtension(filename);
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
		dialog.reset();
		if (extension == MMWS_EXTENSION)
		{
			dialog = std::make_unique<GenericScoreSerializeDialog>(std::make_unique<NativeScoreSerializer>(), std::move(score), filename);
		}
		else if (extension == SUS_EXTENSION)
		{
			dialog = std::make_unique<GenericScoreSerializeDialog>(std::make_unique<SusSerializer>(), std::move(score), filename);
		}
		else if (extension == LVLDAT_EXTENSION)
		{
			dialog = std::make_unique<GenericScoreSerializeDialog>(std::make_unique<SonolusSerializer>(false), std::move(score), filename);
		}
		else if (extension == SCP_EXTENSION || IO::File::getFilename(filename) == "info")
		{
			dialog = std::make_unique<SonolusSerializeDialog>(std::move(score), filename);
		}
	}

	void ScoreSerializeWindow::openDeserializeDialog(const std::string& filename)
	{
		std::string extension = IO::File::getFileExtension(filename);
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
		dialog.reset();
		if (extension == MMWS_EXTENSION)
		{
			dialog = std::make_unique<GenericScoreSerializeDialog>(std::make_unique<NativeScoreSerializer>(), filename);
		}
		else if (extension == SUS_EXTENSION)
		{
			dialog = std::make_unique<GenericScoreSerializeDialog>(std::make_unique<SusSerializer>(), filename);
		}
		else if (extension == LVLDAT_EXTENSION)
		{
			dialog = std::make_unique<GenericScoreSerializeDialog>(std::make_unique<SonolusSerializer>(), filename);
		}
		else if (extension == SCP_EXTENSION || IO::File::getFilename(filename) == "info")
		{
			dialog = std::make_unique<SonolusSerializeDialog>(filename);
		}
		else
		{
			pendingFilename = filename;
			selectedFormatIdx = -1;
		}
	}

	const std::array<std::string, 4> FORMAT_OPTIONS = {
		IO::formatString("%s (%s)", IO::mmwsFilter.filterName, MMWS_EXTENSION),
		IO::formatString("%s (%s)", IO::susFilter.filterName, SUS_EXTENSION),
		IO::lvlDatFilter.filterName,
		IO::formatString("%s (%s)", IO::scpFilter.filterName, SCP_EXTENSION)
	};

	void ScoreSerializeWindow::openDeserializeDialogFromPicker()
	{
		switch (selectedFormatIdx)
		{
		case 0: // MMWS
			dialog = std::make_unique<GenericScoreSerializeDialog>(std::make_unique<NativeScoreSerializer>(), pendingFilename);
			break;
		case 1: // SUS
			dialog = std::make_unique<GenericScoreSerializeDialog>(std::make_unique<SusSerializer>(), pendingFilename);
			break;
		case 2: // Sonolus Level Data
			dialog = std::make_unique<GenericScoreSerializeDialog>(std::make_unique<SonolusSerializer>(), pendingFilename);
			break;
		case 3: // Sonolus Collection
			dialog = std::make_unique<SonolusSerializeDialog>(pendingFilename);
			break;
		default:
			dialog.reset();
			break;
		}
	}

	void ScoreSerializeWindow::updateDeserializerPicker()
	{
		if (pendingFilename.empty())
			return;

		if (!ImGui::IsPopupOpen("###deserializer_picker"))
			ImGui::OpenPopup("###deserializer_picker");

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSizeConstraints({ 450, 300 }, { FLT_MAX, FLT_MAX });
		ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize / 1.5f, ImGuiCond_Always);
		ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
		if (ImGui::BeginPopupModal(APP_NAME "###deserializer_picker", NULL, ImGuiWindowFlags_NoResize))
		{
			const ImGuiStyle& style = ImGui::GetStyle();
			ImGui::Text("%s '%s'\n%s", getString("unknown_file_format"), pendingFilename.c_str(), getString("open_as_file_format"));
			ImVec2 avail = ImGui::GetContentRegionAvail();
			float btnWidth = avail.x / 2 - style.ItemSpacing.x * 2, btnHeight = ImGui::GetFrameHeight();
			
			if (ImGui::BeginListBox("###score_format_list", { avail.x, avail.y - btnHeight - style.ItemSpacing.y * 2 }))
			{
				for (int i = 0; i < FORMAT_OPTIONS.size(); ++i)
				{
					bool isSelected = (selectedFormatIdx == i);
					if (isSelected)
					{
						const ImVec4 color = UI::accentColors[config.accentColor];
						const ImVec4 darkColor = generateDarkColor(color);
						const ImVec4 lightColor = generateHighlightColor(color);
						ImGui::PushStyleColor(ImGuiCol_Header, color);
						ImGui::PushStyleColor(ImGuiCol_HeaderHovered, darkColor);
						ImGui::PushStyleColor(ImGuiCol_HeaderActive, lightColor);
					}
					if (ImGui::Selectable(FORMAT_OPTIONS[i].data(), isSelected))
						selectedFormatIdx = i;
					if (isSelected)
					{
						ImGui::PopStyleColor(3);
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndListBox();
			}
			
			ImGui::BeginDisabled(selectedFormatIdx < 0 || selectedFormatIdx >= FORMAT_OPTIONS.size());
			if (ImGui::Button(getString("select"), { btnWidth, btnHeight }))
			{
				openDeserializeDialogFromPicker();
				pendingFilename.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndDisabled();
			ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 2);
			if (ImGui::Button(getString("cancel"), { btnWidth, btnHeight }))
			{
				pendingFilename.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}
}