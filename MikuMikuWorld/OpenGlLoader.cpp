#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8

#include "Application.h"
#include "StringOperations.h"
#include "UI.h"
#include "Result.h"
#include "stb_image.h"
#include <string>
#include <tinyfiledialogs.h>

namespace MikuMikuWorld
{
	void frameBufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void windowSizeCallback(GLFWwindow* window, int width, int height)
	{
		Application* app = (Application*)glfwGetWindowUserPointer(window);
		if (!app)
			return;

		if (!app->windowState.maximized)
		{
			app->windowState.size.x = width;
			app->windowState.size.y = height;
		}
	}

	void windowPositionCallback(GLFWwindow* window, int x, int y)
	{
		Application* app = (Application*)glfwGetWindowUserPointer(window);
		if (!app)
			return;

		if (!app->windowState.maximized)
		{
			app->windowState.position.x = x;
			app->windowState.position.y = y;
		}
	}

	void dropCallback(GLFWwindow* window, int count, const char** paths)
	{
		Application* app = (Application*)glfwGetWindowUserPointer(window);
		if (!app)
			return;

		for (int i = 0; i < count; ++i)
			app->appendOpenFile(paths[i]);
	}

	void windowCloseCallback(GLFWwindow* window)
	{
		glfwSetWindowShouldClose(window, 0);

		Application* app = (Application*)glfwGetWindowUserPointer(window);
		if (app)
			app->windowState.closing = true;
	}

	void windowMaximizeCallback(GLFWwindow* window, int _maximized)
	{
		Application* app = (Application*)glfwGetWindowUserPointer(window);
		if (app)
			app->windowState.maximized = _maximized;
	}

	void windowKeyCallback(GLFWwindow* window, int key, int scan, int action, int mods)
	{
		Application* app = (Application*)glfwGetWindowUserPointer(window);
		if (action == GLFW_PRESS && app)
		{
			app->windowState.shouldTestKeyboardShortcuts = true;
		}
	}

	void loadIcon(std::string filepath, GLFWwindow* window)
	{
		if (!File::exists(filepath))
			return;

		GLFWimage images[1];
		images[0].pixels = stbi_load(filepath.c_str(), &images[0].width, &images[0].height, 0, 4); //rgba channels 
		glfwSetWindowIcon(window, 1, images);
		stbi_image_free(images[0].pixels);
	}

	void Application::installCallbacks()
	{
		glfwSetWindowPosCallback(window, windowPositionCallback);
		glfwSetWindowSizeCallback(window, windowSizeCallback);
		glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);
		glfwSetDropCallback(window, dropCallback);
		glfwSetWindowCloseCallback(window, windowCloseCallback);
		glfwSetWindowMaximizeCallback(window, windowMaximizeCallback);
		glfwSetKeyCallback(window, windowKeyCallback);
	}

	Result Application::initOpenGL()
	{
		// GLFW initializion
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		window = glfwCreateWindow(config.windowSize.x, config.windowSize.y, APP_NAME, NULL, NULL);
		if (window == NULL)
		{
			glfwTerminate();
			return Result(ResultStatus::Error, "Failed to create GLFW Window.");
		}

		glfwSetWindowPos(window, config.windowPos.x, config.windowPos.y);
		glfwMakeContextCurrent(window);
		glfwSetWindowTitle(window, "Untitled - " APP_NAME);

		// GLAD initializtion
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			glfwTerminate();
			return Result(ResultStatus::Error, "Failed to fetch OpenGL proc address.");
		}

		glfwSetWindowUserPointer(window, this);
		glfwSwapInterval(config.vsync);
		installCallbacks();
		loadIcon(appDir + "res/mmw_icon.png", window);

		if (config.maximized)
			glfwMaximizeWindow(window);

		glLineWidth(1.0f);
		glPointSize(1.0f);
		glEnablei(GL_BLEND, 0);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glViewport(0, 0, config.windowSize.x, config.windowSize.y);

		return Result::Ok();
	}
}
