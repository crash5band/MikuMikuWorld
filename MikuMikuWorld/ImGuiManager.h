#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include <string>

struct GLFWwindow;

namespace MikuMikuWorld
{
	class ImGuiManager
	{
	private:
		ImGuiIO* io;
		std::string configFilename;
		ImGuiID dockspaceID;

	public:
		ImGuiManager();

		bool initialize(GLFWwindow* window);
		void initializeLayout();
		void shutdown();
		void setStyle();
		void begin();
		void loadFont(const std::string& filename, float size);
		void loadIconFont(const std::string& filename, int start, int end, float size);
		void draw(GLFWwindow* window);
	};
}