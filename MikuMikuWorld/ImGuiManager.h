#include "UI.h"
#include <string>

struct GLFWwindow;

namespace MikuMikuWorld
{
	class Result;

	class ImGuiManager
	{
	private:
		std::string configFilename{};
		
		BaseTheme theme{};
		int accentColor{ 1 };

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
		void applyAccentColor(int colIndex);

		inline constexpr BaseTheme getBaseTheme() const { return theme; }
		inline constexpr int getAccentColor() const { return accentColor; }
	};
}