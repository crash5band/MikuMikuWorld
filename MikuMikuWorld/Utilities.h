#include <string>

namespace MikuMikuWorld
{
	class Utilities
	{
	public:
		static std::string getCurrentDateTime();
		static float centerImGuiItem(const float width);
		static void ImGuiCenteredText(const std::string& str);
		static void ImGuiCenteredText(const char* str);
	};
}