#pragma once
#include <stdint.h>
#include <array>
#include <string>
#include "ImGui/imgui.h"

struct InputBinding
{
	uint16_t keyCode{};
	uint8_t keyModifiers{};

	constexpr bool operator==(const InputBinding& other) const
	{
		return keyModifiers == other.keyModifiers && keyCode == other.keyCode;
	}

	constexpr bool operator!=(const InputBinding& other) const
	{
		return !(*this == other);
	}
};

struct MultiInputBinding
{
	int count = 0;
	const char* name;
	std::array<InputBinding, 4> bindings;

	MultiInputBinding(const char* name) { this->name = name; }

	MultiInputBinding(const char* name, InputBinding binding)
	{
		this->name = name;
		bindings[count++] = binding;
	}

	void addBinding(InputBinding binding)
	{
		if (count == bindings.size())
			return;

		bindings[count++] = binding;
	}

	void removeAt(int index)
	{
		if (index < 0 || index >= bindings.size())
			return;

		// shift elements to the left
		for (int i = index; i < count - 1; ++i)
			bindings[i] = bindings[i + 1];

		--count;
	}
};

const char* ToShortcutString(const MultiInputBinding& binding);
const char* ToShortcutString(const InputBinding& binding);
const char* ToShortcutString(ImGuiKey key, ImGuiKeyModFlags mods);
const char* ToShortcutString(ImGuiKey key);

std::string ToFullShortcutsString(const MultiInputBinding& binding);
std::string ToSerializedString(const InputBinding& binding);
InputBinding FromSerializedString(std::string string);

namespace ImGui
{
	bool TestModifiers(ImGuiModFlags mods);

	bool IsDown(const InputBinding& binding);
	bool IsPressed(const InputBinding& binding, bool repeat = false);

	bool IsAnyDown(const MultiInputBinding& binding);
	bool IsAnyPressed(const MultiInputBinding& binding, bool repeat = false);
}