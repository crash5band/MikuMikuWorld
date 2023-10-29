#include "Jacket.h"
#include "File.h"
#include "ImGuiManager.h"
#include "Math.h"
#include "ResourceManager.h"
#include <algorithm>

namespace MikuMikuWorld
{
	constexpr ImVec2 imageOffset{ 5, 5 };
	constexpr ImVec2 imageSize{ 250, 250 };
	const ImVec2 previewSize{ imageSize + (imageOffset * 2) };

	Jacket::Jacket() { clear(); }

	void Jacket::load(const std::string& filename)
	{
		this->filename = filename;
		if (texture)
		{
			texture->dispose();
			texture = nullptr;
		}

		if (filename.empty() || !IO::File::exists(filename))
			return;

		texture = std::make_unique<Texture>(filename);
	}

	void Jacket::clear()
	{
		if (texture)
		{
			texture->dispose();
			texture = nullptr;
		}

		filename = "";
	}

	void Jacket::draw()
	{
		if (texture == nullptr)
			return;

		if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 0.3f)
		{
			// animate opacity
			ImVec4 color{ 1.0f, 1.0f, 1.0f, 0.0f };
			color.w +=
			    std::clamp(lerp(0.0f, 1.0f, (GImGui->HoveredIdTimer - 0.3f) / 0.25f), 0.0f, 1.0f);

			ImGui::SetNextWindowSize(previewSize, ImGuiCond_Always);
			ImGui::SetNextWindowBgAlpha(color.w);

			ImGui::BeginTooltip();
			ImGui::GetWindowDrawList()->AddImage(
			    (void*)texture->getID(), ImGui::GetWindowPos() + imageOffset,
			    ImGui::GetWindowPos() + imageOffset + imageSize, ImVec2{ 0.0, 0.0f },
			    ImVec2{ 1.0f, 1.0f }, ImGui::ColorConvertFloat4ToU32(color));
			ImGui::EndTooltip();
		}
	}

	const std::string& Jacket::getFilename() const { return filename; }

	int Jacket::getTexID() const
	{
		if (texture)
			return texture->getID();

		return 0;
	}
}