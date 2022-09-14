#include "InputListener.h"
#include <stdio.h>

namespace MikuMikuWorld
{
	std::array<bool, GLFW_KEY_LAST> InputListener::prevKeyState;
	std::array<bool, GLFW_KEY_LAST> InputListener::keyState;
	MouseState InputListener::prevMouse;
	MouseState InputListener::mouse;

	void InputListener::update(GLFWwindow* window)
	{
		prevKeyState = keyState;
		for (int i = GLFW_KEY_SPACE; i < GLFW_KEY_LAST; ++i)
			keyState[i] = glfwGetKey(window, i);

		prevMouse = mouse;
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		mouse.pos.x = x;
		mouse.pos.y = y;

		for (int i = 0; i < GLFW_MOUSE_BUTTON_LAST; ++i)
			mouse.buttons[i] = glfwGetMouseButton(window, i);
	}

	bool InputListener::isTapped(int key)
	{
		return !prevKeyState[key] && keyState[key];
	}

	bool InputListener::isDown(int key)
	{
		return keyState[key];
	}

	bool InputListener::isUp(int key)
	{
		return !keyState[key];
	}

	bool InputListener::isCtrlDown()
	{
		return isDown(GLFW_KEY_LEFT_CONTROL) || isDown(GLFW_KEY_RIGHT_CONTROL);
	}

	bool InputListener::isShiftDown()
	{
		return isDown(GLFW_KEY_LEFT_SHIFT) || isDown(GLFW_KEY_RIGHT_SHIFT);
	}

	bool InputListener::isAltDown()
	{
		return isDown(GLFW_KEY_LEFT_ALT) || isDown(GLFW_KEY_RIGHT_ALT);
	}

	bool InputListener::isMouseDown()
	{
		return mouse.buttons[GLFW_MOUSE_BUTTON_LEFT];
	}

	bool InputListener::isMouseUp()
	{
		return !isMouseDown();
	}

	bool InputListener::wasMouseDown()
	{
		return prevMouse.buttons[GLFW_MOUSE_BUTTON_LEFT] && isMouseUp();
	}

	bool InputListener::wasMouseUp()
	{
		return !wasMouseDown();
	}
}