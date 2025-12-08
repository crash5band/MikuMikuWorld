#pragma once
#include "ScoreSerializer.h"
#include "ScoreContext.h"

namespace MikuMikuWorld
{
	class DefaultScoreSerializeController : public ScoreSerializeController
	{
		static bool openFileDialog(SerializeFormat format, std::string& filename);
		void createSerializer();
	public:
		DefaultScoreSerializeController(Score score);
		DefaultScoreSerializeController(Score score, const std::string& filename);

		SerializeResult update() override;
	private:
		std::unique_ptr<ScoreSerializer> serializer;
		SerializeFormat selectedFormat{ -1 };
	};

	class DefaultScoreDeserializeController : public ScoreSerializeController
	{
		void createDeserializer();
	public:
		DefaultScoreDeserializeController(const std::string& filename);

		SerializeResult update() override;
	private:
		std::unique_ptr<ScoreSerializer> deserializer;
		SerializeFormat selectedFormat{ -1 };
	};

	class ScoreEditor;

	class ScoreSerializeWindow
	{
	public:
		ScoreSerializeWindow() = default;
		void update(ScoreEditor& editor, ScoreContext& context, ScoreEditorTimeline& timeline);
		void serialize(const ScoreContext& context);
		void serialize(const ScoreContext& context, const std::string& filename);
		void deserialize(const std::string& filename);
	private:
		std::unique_ptr<ScoreSerializeController> controller{};
	};
}