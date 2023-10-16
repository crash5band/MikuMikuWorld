#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "ImGui/imgui_internal.h"
#include <type_traits>

#define TXT_ARR_SZ(arr) (sizeof(arr) / sizeof(const char*)) 

// Macro to allow usage of flags operators with types enums
#define DECLARE_ENUM_FLAG_OPERATORS(EnumType) \
    inline constexpr EnumType operator|(EnumType lhs, EnumType rhs) { return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) | static_cast<std::underlying_type_t<EnumType>>(rhs)); } \
    inline constexpr EnumType operator&(EnumType lhs, EnumType rhs) { return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) & static_cast<std::underlying_type_t<EnumType>>(rhs)); } \
    inline constexpr EnumType operator^(EnumType lhs, EnumType rhs) { return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) ^ static_cast<std::underlying_type_t<EnumType>>(rhs)); } \
    inline constexpr EnumType operator~(EnumType value) { return static_cast<EnumType>(~static_cast<std::underlying_type_t<EnumType>>(value)); } \
    inline constexpr EnumType& operator|=(EnumType& lhs, EnumType rhs) { lhs = lhs | rhs; return lhs; } \
    inline constexpr EnumType& operator&=(EnumType& lhs, EnumType rhs) { lhs = lhs & rhs; return lhs; } \
    inline constexpr EnumType& operator^=(EnumType& lhs, EnumType rhs) { lhs = lhs ^ rhs; return lhs; }

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