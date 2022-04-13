#pragma once
#include <array>
#include "GLFW/glfw3.h"
#include "Math.h"

namespace MikuMikuWorld
{
	struct MouseState
	{
		Vector2 pos;
		std::array<bool, GLFW_MOUSE_BUTTON_LAST> buttons;
	};

	class InputListener
	{
	private:
		static std::array<bool, GLFW_KEY_LAST> prevKeyState;
		static std::array<bool, GLFW_KEY_LAST> keyState;
		static MouseState prevMouse;
		static MouseState mouse;

	public:
		static void update(GLFWwindow* window);
		static bool isTapped(int key);
		static bool isDown(int key);
		static bool isUp(int key);
		static bool isCtrlDown();
		static bool isShiftDown();
		static bool isAltDown();

		static bool isMouseDown();
		static bool isMouseUp();
		static bool wasMouseDown();
		static bool wasMouseUp();
	};
}