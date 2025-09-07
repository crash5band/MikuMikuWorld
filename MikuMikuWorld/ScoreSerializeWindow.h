#pragma once
#include "ScoreSerializer.h"
#include "ScoreContext.h"

namespace MikuMikuWorld
{
	class ScoreEditor;

	class ScoreSerializeWindow
	{
		void openDeserializeDialogFromPicker();
		void updateDeserializerPicker();
	public:
		ScoreSerializeWindow() = default;
		void update(ScoreEditor& editor, ScoreContext& context, ScoreEditorTimeline& timeline);
		void openSerializeDialog(const ScoreContext& context, const std::string& filename);
		void openDeserializeDialog(const std::string& filename);
	private:
		std::unique_ptr<ScoreSerializeDialog> dialog;

		int selectedFormatIdx = -1;
		std::string pendingFilename;
	};
}