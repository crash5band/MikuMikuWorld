#pragma once
#include <string>
#include "Rendering/Texture.h"
#include "ImGui/imgui.h"

namespace MikuMikuWorld
{
	class Jacket
	{
	private:
		ImVec2 imageOffset;
		ImVec2 imageSize;
		ImVec2 previewSize;
		std::string filename;
		int texID;

	public:
		Jacket();

		void load(const std::string filename);
		void draw();
		void clear();

		const std::string& getFilename() const;
		int getTexID() const;
	};
}