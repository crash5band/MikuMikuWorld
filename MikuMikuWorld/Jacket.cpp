#include "Jacket.h"
#include "ResourceManager.h"
#include "ImGuiManager.h"
#include "File.h"
#include "Math.h"
#include <algorithm>

namespace MikuMikuWorld
{
	Jacket::Jacket() : 
		imageOffset{ ImVec2{5, 5} }, imageSize{ ImVec2{250, 250} },
		previewSize{ImVec2{imageSize + (imageOffset * 2)}}
	{
		clear();
	}

	void Jacket::load(const std::string& filename)
	{
		if (!filename.size())
			return;

		ResourceManager::loadTexture(filename);
		int texIndex = ResourceManager::getTexture(File::getFilenameWithoutExtension(filename));
		
		if (texIndex != -1)
		{
			texID = ResourceManager::textures[texIndex].getID();
		}

		this->filename = filename;
	}

	void Jacket::clear()
	{
		filename = "";
		texID = 0;
	}

	void Jacket::draw()
	{
		if (!texID)
			return;

		if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 0.3f)
		{
			// animate opacity
			ImVec4 color{ 1.0f, 1.0f, 1.0f, 0.0f };
			color.w += std::clamp(lerp(0.0f, 1.0f, (GImGui->HoveredIdTimer - 0.3f) / 0.25f), 0.0f, 1.0f);

			ImGui::SetNextWindowSize(previewSize, ImGuiCond_Always);
			ImGui::SetNextWindowBgAlpha(color.w);

			ImGui::BeginTooltip();
			ImGui::GetWindowDrawList()->AddImage(
				(void*)texID,
				ImGui::GetWindowPos() + imageOffset,
				ImGui::GetWindowPos() + imageOffset + imageSize,
				ImVec2{ 0.0, 0.0f },
				ImVec2{ 1.0f, 1.0f },
				ImGui::ColorConvertFloat4ToU32(color)
			);
			ImGui::EndTooltip();
		}
	}

	const std::string& Jacket::getFilename() const
	{
		return filename;
	}

	int Jacket::getTexID() const
	{
		return texID;
	}
}