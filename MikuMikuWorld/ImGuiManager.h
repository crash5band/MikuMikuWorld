#include "UI.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include <string>

struct GLFWwindow;

namespace MikuMikuWorld
{
	class Result;

	class ImGuiManager
	{
	private:
		ImGuiIO* io;
		std::string configFilename;

		int accentColor;
		BaseTheme baseTheme;

	public:
		ImGuiManager();

		Result initialize(GLFWwindow* window);
		void initializeLayout();
		void shutdown();
		void begin();
		void loadFont(const std::string& filename, float size);
		void loadIconFont(const std::string& filename, int start, int end, float size);
		void draw(GLFWwindow* window);

		void setBaseTheme(BaseTheme theme);
		BaseTheme getBaseTheme() const;

		void applyAccentColor(int colIndex);
		int getAccentColor() const;
	};
}