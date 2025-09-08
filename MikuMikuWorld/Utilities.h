#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "ImGui/imgui_internal.h"
#include <type_traits>

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
	};

	enum class ResultStatus
	{
		Success, Warning, Error
	};

	class Result
	{
	private:
		ResultStatus status;
		std::string message;

	public:
		Result(ResultStatus _status, std::string _msg = "")
			: status{ _status }, message{ _msg }
		{
		}

		ResultStatus getStatus() const { return status; }
		std::string getMessage() const { return message; }
		bool isOk() const { return status == ResultStatus::Success; }

		static Result Ok() { return Result(ResultStatus::Success); }
	};

	constexpr static const char* boolToString(bool value)
	{
		return value ? "true" : "false";
	}

	template<typename ArrayType>
	static size_t arrayLength(const ArrayType& arr)
	{
		static_assert(std::is_array_v<ArrayType>);
		return (sizeof(arr) / sizeof(arr[0]));
	}

	template<typename Array>
	static inline bool isArrayIndexInBounds(size_t index, const Array& arr)
	{
		return index >= 0 && index < arrayLength(arr);
	}

	template<typename T>
	static inline bool isArrayIndexInBounds(size_t index, const std::vector<T>& arr)
	{
		return index >= 0 && index < arr.size();
	}

	template<typename Type>
	static size_t findArrayItem(const Type& item, const Type array[], size_t length)
	{
		for (int i = 0; i < length; i++)
		{
			if (array[i] == item)
				return i;
		}

		return -1;
	}

	static size_t findArrayItem(const char* item, const char* const array[], size_t length)
	{
		for (int i = 0; i < length; i++)
		{
			if (!strcmp(item, array[i]))
				return i;
		}

		return -1;
	}

	static std::string formatFixedFloatTrimmed(float value, int precision = 7)
	{
		auto length = std::snprintf(NULL, 0, "%.*f", precision, value);
		if (length < 0)
			return "NaN";
		std::string buf(length - 1, '\0');
		std::snprintf(buf.data(), length, "%.*f", precision, value);
		// Trim trailing zeros
		size_t end = buf.find_last_not_of('0');
		if (end != std::string::npos)
			buf.erase(buf[end] == '.' ? end : end + 1);
		return buf;
	}

	void drawShadedText(ImDrawList* drawList, ImVec2 textPos, float fontSize, ImU32 fontColor, const char* text);
	void drawTextureUnscaled(unsigned int texId, float texWidth, float texHeight, ImVec2 const& pos, ImVec2 const& size, ImU32 const& col);
}