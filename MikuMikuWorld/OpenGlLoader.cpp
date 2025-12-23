#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8

#include "Application.h"
#include "ApplicationConfiguration.h"
#include "UI.h"
#include "stb_image.h"
#include "dwmapi.h"


namespace MikuMikuWorld
{
	void frameBufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void windowSizeCallback(GLFWwindow* window, int width, int height)
	{
		if (!Application::windowState.maximized && !Application::windowState.fullScreen)
		{
			Application::windowState.size.x = width;
			Application::windowState.size.y = height;
		}
	}

	void windowPositionCallback(GLFWwindow* window, int x, int y)
	{
		if (!Application::windowState.maximized && !Application::windowState.fullScreen)
		{
			Application::windowState.position.x = x;
			Application::windowState.position.y = y;
		}
	}

	void windowCloseCallback(GLFWwindow* window)
	{
		glfwSetWindowShouldClose(window, 0);
		Application::windowState.closing = true;
	}

	void windowMaximizeCallback(GLFWwindow* window, int _maximized)
	{
		Application::windowState.maximized = _maximized;
	}

	Result Application::initOpenGL()
	{
		// GLFW initializion
		const char* glfwErrorDescription = NULL;
		int possibleError = GLFW_NO_ERROR;

		glfwInit();
		possibleError = glfwGetError(&glfwErrorDescription);
		if (possibleError != GLFW_NO_ERROR)
		{
			glfwTerminate();
			return Result(ResultStatus::Error, "Failed to initialize GLFW.\n" + std::string(glfwErrorDescription));
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

		int width{ static_cast<int>(config.windowSize.x) }, height{ static_cast<int>(config.windowSize.y) };

		if (config.fullScreen)
		{
			GLFWmonitor* mainMonitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(mainMonitor);

			width = mode->width, height = mode->height;
			window = glfwCreateWindow(width, height, APP_NAME, mainMonitor, NULL);
		}
		else
		{
			window = glfwCreateWindow(width, height, APP_NAME, NULL, NULL);
		}

		possibleError = glfwGetError(&glfwErrorDescription);
		if (possibleError != GLFW_NO_ERROR)
		{
			glfwTerminate();
			return Result(ResultStatus::Error, "Failed to create GLFW Window.\n" + std::string(glfwErrorDescription));
		}

		BOOL isDarkMode = UI::isSystemDarkMode();
		UI::setDarkMode(isDarkMode);
		glfwSetWindowPos(window, config.windowPos.x, config.windowPos.y);
		
		glfwMakeContextCurrent(window);
		glfwSetWindowTitle(window, APP_NAME " - Untitled");
		glfwSetWindowPosCallback(window, windowPositionCallback);
		glfwSetWindowSizeCallback(window, windowSizeCallback);
		glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);
		glfwSetWindowCloseCallback(window, windowCloseCallback);
		glfwSetWindowMaximizeCallback(window, windowMaximizeCallback);

		std::string iconFilename = appDir + "res\\mmw_icon.png";
		if (IO::File::exists(iconFilename))
		{
			GLFWimage images[1]{};
			images[0].pixels = stbi_load(iconFilename.c_str(), &images[0].width, &images[0].height, 0, 4); //rgba channels 
			glfwSetWindowIcon(window, 1, images);
			stbi_image_free(images[0].pixels);
		}

		// GLAD initializtion
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			glfwTerminate();
			return Result(ResultStatus::Error, "Failed to fetch OpenGL proc address.");
		}

		glfwSwapInterval(config.vsync);
		if (config.maximized && !config.fullScreen)
			glfwMaximizeWindow(window);

		glLineWidth(1.0f);
		glPointSize(1.0f);
		glEnablei(GL_BLEND, 0);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glViewport(0, 0, width, height);

		return Result::Ok();
	}
}
