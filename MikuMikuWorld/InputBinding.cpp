#include "InputBinding.h"
#include "ImGui/imgui.h"
#include "IO.h"

using IO::trim, IO::split;

struct ImGuiKeyInfo
{
	ImGuiKey key;
	const char* name;
	const char* displayName;
};

static char keyInfoNameBuffer[128]{};

static constexpr ImGuiKeyInfo imguiKeysTable[] =
{
	{ ImGuiKey_Tab,				"Tab", "Tab" },
	{ ImGuiKey_LeftArrow,		"LeftArrow", "Left" },
	{ ImGuiKey_RightArrow,		"RightArrow", "Right" },
	{ ImGuiKey_UpArrow,			"UpArrow", "Up" },
	{ ImGuiKey_DownArrow,		"DownArrow", "Down" },
	{ ImGuiKey_PageUp,			"PageUp", "Page Up" },
	{ ImGuiKey_PageDown,		"PageDown", "Page Down" },
	{ ImGuiKey_Home,			"Home", "Home" },
	{ ImGuiKey_End,				"End", "End" },
	{ ImGuiKey_Insert,			"Insert", "Insert" },
	{ ImGuiKey_Delete,			"Delete", "Delete" },
	{ ImGuiKey_Backspace,		"Backspace", "Backspace" },
	{ ImGuiKey_Space,			"Space", "Space" },
	{ ImGuiKey_Enter,			"Enter", "Enter" },
	{ ImGuiKey_Escape,			"Escape", "Escape" },
	{ ImGuiKey_LeftCtrl,		"LeftCtrl", "Left Ctrl" },
	{ ImGuiKey_LeftShift,		"LeftShift", "Left Shift" },
	{ ImGuiKey_LeftAlt, 		"LeftAlt", "Left Alt" },
	{ ImGuiKey_LeftSuper,		"LeftSuper", "Left Super" },
	{ ImGuiKey_RightCtrl, 		"RightCtrl", "Right Ctrl" },
	{ ImGuiKey_RightShift, 		"RightShift", "Right Shift" },
	{ ImGuiKey_RightAlt, 		"RightAlt", "Right Alt" },
	{ ImGuiKey_RightSuper,		"RightSuper", "Right Super" },
	{ ImGuiKey_Menu,			"Menu", "Menu" },
	{ ImGuiKey_0, 				"0", "0" },
	{ ImGuiKey_1, 				"1", "1" },
	{ ImGuiKey_2, 				"2", "2" },
	{ ImGuiKey_3, 				"3", "3" },
	{ ImGuiKey_4, 				"4", "4" },
	{ ImGuiKey_5, 				"5", "5" },
	{ ImGuiKey_6, 				"6", "6" },
	{ ImGuiKey_7, 				"7", "7" },
	{ ImGuiKey_8, 				"8", "8" },
	{ ImGuiKey_9,				"9", "9" },
	{ ImGuiKey_A, 				"A", "A" },
	{ ImGuiKey_B, 				"B", "B" },
	{ ImGuiKey_C, 				"C", "C" },
	{ ImGuiKey_D, 				"D", "D" },
	{ ImGuiKey_E, 				"E", "E" },
	{ ImGuiKey_F, 				"F", "F" },
	{ ImGuiKey_G, 				"G", "G" },
	{ ImGuiKey_H, 				"H", "H" },
	{ ImGuiKey_I, 				"I", "I" },
	{ ImGuiKey_J,				"J", "J" },
	{ ImGuiKey_K, 				"K", "K" },
	{ ImGuiKey_L, 				"L", "L" },
	{ ImGuiKey_M, 				"M", "M" },
	{ ImGuiKey_N, 				"N", "N" },
	{ ImGuiKey_O, 				"O", "O" },
	{ ImGuiKey_P, 				"P", "P" },
	{ ImGuiKey_Q, 				"Q", "Q" },
	{ ImGuiKey_R, 				"R", "R" },
	{ ImGuiKey_S, 				"S", "S" },
	{ ImGuiKey_T,				"T", "T" },
	{ ImGuiKey_U, 				"U", "U" },
	{ ImGuiKey_V, 				"V", "V" },
	{ ImGuiKey_W, 				"W", "W" },
	{ ImGuiKey_X, 				"X", "X" },
	{ ImGuiKey_Y, 				"Y", "Y" },
	{ ImGuiKey_Z,				"Z", "Z" },
	{ ImGuiKey_F1, 				"F1", "F1" },
	{ ImGuiKey_F2, 				"F2", "F2" },
	{ ImGuiKey_F3, 				"F3", "F3" },
	{ ImGuiKey_F4, 				"F4", "F4" },
	{ ImGuiKey_F5, 				"F5", "F5" },
	{ ImGuiKey_F6,				"F6", "F6" },
	{ ImGuiKey_F7, 				"F7", "F7" },
	{ ImGuiKey_F8, 				"F8", "F8" },
	{ ImGuiKey_F9, 				"F9", "F9" },
	{ ImGuiKey_F10, 			"F10", "F10" },
	{ ImGuiKey_F11, 			"F11", "F11" },
	{ ImGuiKey_F12,				"F12", "F12" },
	{ ImGuiKey_Apostrophe,		"Apostrophe", "'"},       // '
	{ ImGuiKey_Comma,           "Comma", "," },           // ,
	{ ImGuiKey_Minus,           "Minus", "-" },           // -
	{ ImGuiKey_Period,          "Period", "." },          // .
	{ ImGuiKey_Slash,           "Slash", "/" },           // /
	{ ImGuiKey_Semicolon,       "Semicolon", ";" },       // ;
	{ ImGuiKey_Equal,           "Equal", "=" },           // =
	{ ImGuiKey_LeftBracket,     "LeftBracket", "[" },     // [
	{ ImGuiKey_Backslash,       "Backslash", "\\" },      // \ (this text inhibits multiline comment caused by backslash)
	{ ImGuiKey_RightBracket,    "RightBracket", "]" },    // ]
	{ ImGuiKey_GraveAccent,     "GraveAccent", "`" },     // `
	{ ImGuiKey_CapsLock,		"CapsLock", "Caps Lock" },
	{ ImGuiKey_ScrollLock,		"ScrollLock", "Scroll Lock" },
	{ ImGuiKey_NumLock,			"NumLock", "Num Lock" },
	{ ImGuiKey_PrintScreen,		"PrintScreen", "Print Screen" },
	{ ImGuiKey_Pause,			"Pause", "Pause" },
	{ ImGuiKey_Keypad0, 		"Keypad0", "Keypad 0" },
	{ ImGuiKey_Keypad1, 		"Keypad1", "Keypad 1" },
	{ ImGuiKey_Keypad2, 		"Keypad2", "Keypad 2" },
	{ ImGuiKey_Keypad3, 		"Keypad3", "Keypad 3" },
	{ ImGuiKey_Keypad4,			"Keypad4", "Keypad 4" },
	{ ImGuiKey_Keypad5, 		"Keypad5", "Keypad 5" },
	{ ImGuiKey_Keypad6, 		"Keypad6", "Keypad 6" },
	{ ImGuiKey_Keypad7, 		"Keypad7", "Keypad 7" },
	{ ImGuiKey_Keypad8, 		"Keypad8", "Keypad 8" },
	{ ImGuiKey_Keypad9,			"Keypad9", "Keypad 9" },
	{ ImGuiKey_KeypadDecimal,	"KeypadDecimal", "Keypad Decimal" },
	{ ImGuiKey_KeypadDivide,	"KeypadDivide", "Keypad Divide" },
	{ ImGuiKey_KeypadMultiply,	"KeypadMultiply", "Keypad Multiply" },
	{ ImGuiKey_KeypadSubtract,	"KeypadSubtract", "Keypad Subtract" },
	{ ImGuiKey_KeypadAdd,		"KeypadAdd", "Keypad Add" },
	{ ImGuiKey_KeypadEnter,		"KeypadEnter", "Keypad Enter" },
	{ ImGuiKey_KeypadEqual,		"KeypadEqual", "Keypad Equal" },

	{ ImGuiKey_ModCtrl,			"Ctrl", "Ctrl" },
	{ ImGuiKey_ModShift,		"Shift", "Shift" },
	{ ImGuiKey_ModAlt, 			"Alt", "Alt" },
	{ ImGuiKey_ModSuper,		"Super", "Super" }
};

static constexpr ImGuiKeyInfo GetImGuiKeyInfo(ImGuiKey key)
{
	if (key == ImGuiKey_None)
		return { key, "None", "" };
	else if (key >= ImGuiKey_ModCtrl && key <= ImGuiKey_ModSuper)
		return imguiKeysTable[105 + key - ImGuiKey_ModCtrl]; // not very elegant but will do for now
	else if (key < ImGuiKey_NamedKey_BEGIN || key > ImGuiKey_ModSuper || (key > ImGuiKey_KeypadEqual && key < ImGuiKey_ModCtrl))
		return { key, "Unknown", "Unknown" };
	else
		return imguiKeysTable[key - ImGuiKey_NamedKey_BEGIN];
}

const char* ToShortcutString(const MultiInputBinding& binding)
{
	if (binding.count < 1)
	{
		return GetImGuiKeyInfo(ImGuiKey_None).displayName;
	}
	else
	{
		return ToShortcutString(binding.bindings[0]);
	}
}

const char* ToShortcutString(const InputBinding& binding)
{
	return ToShortcutString(binding.keyCode, binding.keyModifiers);
}

const char* ToShortcutString(ImGuiKey key, ImGuiKeyModFlags mods)
{
	strcpy(keyInfoNameBuffer, "\0"); // start from the beginning of the buffer
	if (mods & ImGuiKeyModFlags_Ctrl)
	{
		strcat(keyInfoNameBuffer, GetImGuiKeyInfo(ImGuiKey_ModCtrl).displayName);
		strcat(keyInfoNameBuffer, " + ");
	}
	if (mods & ImGuiKeyModFlags_Shift)
	{
		strcat(keyInfoNameBuffer, GetImGuiKeyInfo(ImGuiKey_ModShift).displayName);
		strcat(keyInfoNameBuffer, " + ");
	}
	if (mods & ImGuiKeyModFlags_Alt)
	{
		strcat(keyInfoNameBuffer, GetImGuiKeyInfo(ImGuiKey_ModAlt).displayName);
		strcat(keyInfoNameBuffer, " + ");
	};

	strcat(keyInfoNameBuffer, GetImGuiKeyInfo(key).displayName);
	return keyInfoNameBuffer;
}

const char* ToShortcutString(ImGuiKey key)
{
	return GetImGuiKeyInfo(key).displayName;
}

std::string ToSerializedString(const InputBinding& binding)
{
	std::string string{};
	if (binding.keyModifiers & ImGuiKeyModFlags_Ctrl) string.append(GetImGuiKeyInfo(ImGuiKey_ModCtrl).name).append(" + ");
	if (binding.keyModifiers & ImGuiKeyModFlags_Shift) string.append(GetImGuiKeyInfo(ImGuiKey_ModShift).name).append(" + ");
	if (binding.keyModifiers & ImGuiKeyModFlags_Alt) string.append(GetImGuiKeyInfo(ImGuiKey_ModAlt).name).append(" + ");

	return string.append(GetImGuiKeyInfo(binding.keyCode).name);
}

std::string ToFullShortcutsString(const MultiInputBinding& binding)
{
	if (!binding.count)
		return "None";

	std::string shortcuts{};
	for (int i = 0; i < binding.count; ++i)
	{
		const ImGuiKeyInfo keyInfo = GetImGuiKeyInfo(binding.bindings.at(i).keyCode);
		if (strcmp(keyInfo.name, "None"))
			shortcuts.append(ToShortcutString(binding.bindings[i]));

		if (i < binding.count - 1)
			shortcuts.append(", ");
	}

	return shortcuts;
}

InputBinding FromSerializedString(std::string string)
{
	string = trim(string);
	std::vector<std::string> keys = split(string, "+");

	InputBinding binding{ ImGuiKey_None, ImGuiKeyModFlags_None };

	// case insensitive comparison
	auto stringCompare = [](char a, char b) { return std::tolower(a) == std::tolower(b); };
	for (auto& key : keys)
	{
		// get rid of possible space before/after key name
		key = trim(key);

		// look for a matching key name
		ImGuiKey imKey = ImGuiKey_NamedKey_BEGIN;
		for (imKey = ImGuiKey_NamedKey_BEGIN; imKey <= ImGuiKey_NamedKey_END; ++imKey)
		{
			ImGuiKeyInfo infoKey = GetImGuiKeyInfo(imKey);
			if (infoKey.name == nullptr)
				continue;

			std::string infoKeyName{ infoKey.name };
			if (std::equal(key.begin(), key.end(), infoKeyName.begin(), infoKeyName.end(), stringCompare))
				break;
		}

		switch (imKey)
		{
		case ImGuiKey_ModCtrl: binding.keyModifiers |= ImGuiKeyModFlags_Ctrl; break;
		case ImGuiKey_ModShift: binding.keyModifiers |= ImGuiKeyModFlags_Shift; break;
		case ImGuiKey_ModAlt: binding.keyModifiers |= ImGuiKeyModFlags_Alt; break;
		default: binding.keyCode = imKey; break;
		}
	}

	return binding;
}

namespace ImGui
{
	bool TestModifiers(ImGuiModFlags mods)
	{
		return ImGui::GetIO().KeyMods == mods;
	}

	bool IsDown(const InputBinding& binding)
	{
		return ImGui::TestModifiers(binding.keyModifiers) && ImGui::IsKeyDown(binding.keyCode);
	}

	bool IsPressed(const InputBinding& binding, bool repeat)
	{
		return ImGui::TestModifiers(binding.keyModifiers) && ImGui::IsKeyPressed(binding.keyCode, repeat);
	}

	bool IsAnyDown(const MultiInputBinding& bindings)
	{
		for (int i = 0; i < bindings.count; ++i)
			if (IsDown(bindings.bindings[i]))
				return true;
		
		return false;
	}

	bool IsAnyPressed(const MultiInputBinding& bindings, bool repeat)
	{
		for (int i = 0; i < bindings.count; ++i)
			if (IsPressed(bindings.bindings[i], repeat))
				return true;

		return false;
	}
}
