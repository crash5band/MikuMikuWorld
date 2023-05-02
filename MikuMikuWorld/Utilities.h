#include <string>
#include <vector>
#include <algorithm>
#include "ImGui/imgui_internal.h"

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

		template<typename T>
		static void sort(std::vector<T>& list, bool (*func)(const T& a, const T& b))
		{
			std::sort(list.begin(), list.end(), func);
		}
	};

	static const char* boolToString(bool value)
	{
		return value ? "true" : "false";
	}

	template <typename T>
	static int findArrayItem(T item, const T array[], int length)
	{
		for (int i = 0; i < length; ++i)
		{
			if (array[i] == item)
				return i;
		}

		return -1;
	}

	static int findArrayItem(const char* item, const char* const array[], int length)
	{
		for (int i = 0; i < length; ++i)
		{
			if (!strcmp(item, array[i]))
				return i;
		}

		return -1;
	}

	void drawShadedText(ImDrawList* drawList, ImVec2 textPos, float fontSize, ImU32 fontColor, const char* text);
}