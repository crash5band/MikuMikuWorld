#pragma once
#include "ImGui/imgui_internal.h"
#include <algorithm>
#include <string>
#include <type_traits>
#include <vector>

// Macro to allow usage of flags operators with types enums
#define DECLARE_ENUM_FLAG_OPERATORS(EnumType)                                                                          \
	inline constexpr EnumType operator|(EnumType lhs, EnumType rhs)                                                    \
	{                                                                                                                  \
		return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) |                              \
		                             static_cast<std::underlying_type_t<EnumType>>(rhs));                              \
	}                                                                                                                  \
	inline constexpr EnumType operator&(EnumType lhs, EnumType rhs)                                                    \
	{                                                                                                                  \
		return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) &                              \
		                             static_cast<std::underlying_type_t<EnumType>>(rhs));                              \
	}                                                                                                                  \
	inline constexpr EnumType operator^(EnumType lhs, EnumType rhs)                                                    \
	{                                                                                                                  \
		return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) ^                              \
		                             static_cast<std::underlying_type_t<EnumType>>(rhs));                              \
	}                                                                                                                  \
	inline constexpr EnumType operator~(EnumType value)                                                                \
	{                                                                                                                  \
		return static_cast<EnumType>(~static_cast<std::underlying_type_t<EnumType>>(value));                           \
	}                                                                                                                  \
	inline constexpr EnumType& operator|=(EnumType& lhs, EnumType rhs)                                                 \
	{                                                                                                                  \
		lhs = lhs | rhs;                                                                                               \
		return lhs;                                                                                                    \
	}                                                                                                                  \
	inline constexpr EnumType& operator&=(EnumType& lhs, EnumType rhs)                                                 \
	{                                                                                                                  \
		lhs = lhs & rhs;                                                                                               \
		return lhs;                                                                                                    \
	}                                                                                                                  \
	inline constexpr EnumType& operator^=(EnumType& lhs, EnumType rhs)                                                 \
	{                                                                                                                  \
		lhs = lhs ^ rhs;                                                                                               \
		return lhs;                                                                                                    \
	}

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

		template <typename T> static void sort(std::vector<T>& list, bool (*func)(const T& a, const T& b))
		{
			std::sort(list.begin(), list.end(), func);
		}
	};

	enum class ResultStatus
	{
		Success,
		Warning,
		Error
	};

	class Result
	{
	  private:
		ResultStatus status;
		std::string message;

	  public:
		Result(ResultStatus _status, std::string _msg = "") : status{ _status }, message{ _msg } {}

		ResultStatus getStatus() const { return status; }
		std::string getMessage() const { return message; }
		bool isOk() const { return status == ResultStatus::Success; }

		static Result Ok() { return Result(ResultStatus::Success); }
	};

	constexpr static const char* boolToString(bool value) { return value ? "true" : "false"; }

	template <typename ArrayType> static size_t arrayLength(const ArrayType& arr)
	{
		static_assert(std::is_array_v<ArrayType>);
		return (sizeof(arr) / sizeof(arr[0]));
	}

	template <typename Array> static inline bool isArrayIndexInBounds(size_t index, const Array& arr)
	{
		return index >= 0 && index < arrayLength(arr);
	}

	template <typename T> static inline bool isArrayIndexInBounds(size_t index, const std::vector<T>& arr)
	{
		return index >= 0 && index < arr.size();
	}

	template <typename Type> static size_t findArrayItem(Type item, const Type array[], size_t length)
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

	void drawShadedText(ImDrawList* drawList, ImVec2 textPos, float fontSize, ImU32 fontColor, const char* text);
}