#include <string>

#define TXT_ARR_SZ(arr) (sizeof(arr) / sizeof(const char*)) 

namespace MikuMikuWorld
{
	class Utilities
	{
	public:
		static std::string getCurrentDateTime();
		static std::string getSystemLocale();
		static std::string getDivisionString(int div);
		static float centerImGuiItem(const float width);
		static void ImGuiCenteredText(const std::string& str);
		static void ImGuiCenteredText(const char* str);
	};
}