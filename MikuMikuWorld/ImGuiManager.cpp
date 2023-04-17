#include "Application.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "../Depends/glad/include/glad/glad.h"
#include "../Depends/GLFW/include/GLFW/glfw3.h"
#include "File.h"
#include "Constants.h"
#include "UI.h"
#include "Result.h"
#include "IconsFontAwesome5.h"
#include "Colors.h"

namespace MikuMikuWorld
{
	ImGuiManager::ImGuiManager() :
		io{ nullptr }, configFilename{ "" }
	{

	}

	Result ImGuiManager::initialize(GLFWwindow* window)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		configFilename = Application::getAppDir() + IMGUI_CONFIG_FILENAME;

		io = &ImGui::GetIO();
		io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io->ConfigWindowsMoveFromTitleBarOnly = true;
		io->ConfigViewportsNoDefaultParent = false;
		io->ConfigViewportsNoAutoMerge = true;
		io->ConfigViewportsNoDecoration = false;
		io->KeyRepeatRate = 0.15f;
		io->IniFilename = configFilename.c_str();

		if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
			return Result(ResultStatus::Error, "Failed to initialize ImGui GLFW implementation.");

		if (!ImGui_ImplOpenGL3_Init("#version 150"))
			return Result(ResultStatus::Error, "Failed to initialize ImGui OpenGL implementation.");

		std::string baseDir = Application::getAppDir();
		loadFont(baseDir + "res/fonts/NotoSansCJK-Regular.ttc", 18.0f);
		loadIconFont(baseDir + "res/fonts/fa-solid-900.ttf", ICON_MIN_FA, ICON_MAX_FA, 14.0f);

		setBaseTheme(BaseTheme::DARK);

		return Result::Ok();
	}

	void ImGuiManager::setBaseTheme(BaseTheme theme)
	{
		ImGuiStyle* style = &ImGui::GetStyle();
		style->FramePadding.x = 4;
		style->FramePadding.y = 2;
		style->ItemSpacing.x = 3;
		style->ItemSpacing.y = 4;
		style->WindowPadding.x = 8;
		style->WindowRounding = 4;
		style->WindowBorderSize = 1;
		style->FrameBorderSize = 0;
		style->FrameRounding = 0.5f;
		style->ScrollbarRounding = 2;
		style->ChildRounding = 1;
		style->PopupRounding = 2;
		style->GrabRounding = 0.5f;
		style->TabRounding = 1;
		style->ScrollbarSize = 14;

		style->AntiAliasedLines = true;
		style->AntiAliasedFill = true;

		ImVec4* colors = style->Colors;

		if (theme == BaseTheme::LIGHT) {
			ImGui::StyleColorsLight();
			colors[ImGuiCol_Text] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
			colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);
			colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.80f, 0.85f);
			colors[ImGuiCol_BorderShadow] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
			colors[ImGuiCol_FrameBg] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
			colors[ImGuiCol_FrameBgHovered] = ImVec4(0.85f, 0.85f, 0.85f, 0.78f);
			colors[ImGuiCol_FrameBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 0.67f);
			colors[ImGuiCol_TitleBgActive] = ImVec4(0.9f, 0.9f, 0.9f, 1.00f);
			colors[ImGuiCol_MenuBarBg] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
			colors[ImGuiCol_CheckMark] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
			colors[ImGuiCol_Button] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
			colors[ImGuiCol_ButtonHovered] = ImVec4(0.7f, 0.7f, 0.7f, 1.00f);
			colors[ImGuiCol_ButtonActive] = ImVec4(0.66f, 0.84f, 0.95f, 1.00f);
			colors[ImGuiCol_Header] = ImVec4(0.97f, 0.97f, 0.97f, 1.00f);
			colors[ImGuiCol_HeaderHovered] = ImVec4(0.97f, 0.97f, 0.97f, 1.00f);
			colors[ImGuiCol_HeaderActive] = ImVec4(0.97f, 0.97f, 0.97f, 1.00f);
			colors[ImGuiCol_Separator] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
			colors[ImGuiCol_SeparatorActive] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
			colors[ImGuiCol_Tab] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
			colors[ImGuiCol_TabHovered] = ImVec4(0.99f, 0.48f, 0.88f, 0.80f);
			colors[ImGuiCol_TabActive] = ImVec4(0.16f, 0.44f, 0.75f, 1.00f);
			colors[ImGuiCol_TabUnfocused] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
			colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
			colors[ImGuiCol_ChildBg] = ImVec4(0.9f, 0.9f, 0.9f, 1.00f);
			colors[ImGuiCol_PopupBg] = ImVec4(0.9f, 0.9f, 0.9f, 1.00f);
			colors[ImGuiCol_TitleBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
			colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
			colors[ImGuiCol_ScrollbarBg] = ImVec4(0.9f, 0.9f, 0.9f, 1.00f);
			colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
			colors[ImGuiCol_TableHeaderBg] = ImVec4(0.9f, 0.9f, 0.9f, 1.00f);
			colors[ImGuiCol_TableRowBg] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
			colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.91f, 0.91f, 0.91f, 1.00f);
		}
		else {
			ImGui::StyleColorsDark();
			colors[ImGuiCol_WindowBg] = ImVec4(0.196f, 0.196f, 0.196f, 1.00f);
			colors[ImGuiCol_Border] = ImVec4(0.10f, 0.10f, 0.10f, 0.85f);
			colors[ImGuiCol_BorderShadow] = ImVec4(0.10f, 0.10f, 0.10f, 0.35f);
			colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
			colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.15f, 0.15f, 0.78f);
			colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 0.67f);
			colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
			colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
			colors[ImGuiCol_CheckMark] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
			colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
			colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
			colors[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.44f, 0.75f, 1.00f);
			colors[ImGuiCol_Header] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
			colors[ImGuiCol_HeaderHovered] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
			colors[ImGuiCol_HeaderActive] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
			colors[ImGuiCol_Separator] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
			colors[ImGuiCol_SeparatorActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
			colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
			colors[ImGuiCol_TabHovered] = ImVec4(0.19f, 0.48f, 0.88f, 0.80f);
			colors[ImGuiCol_TabActive] = ImVec4(0.16f, 0.44f, 0.75f, 1.00f);
			colors[ImGuiCol_TabUnfocused] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
			colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
			colors[ImGuiCol_ChildBg] = ImVec4(0.196f, 0.196f, 0.196f, 1.00f);
			colors[ImGuiCol_PopupBg] = ImVec4(0.196f, 0.196f, 0.196f, 1.00f);
			colors[ImGuiCol_TitleBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
			colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
			colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
			colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
			colors[ImGuiCol_TableHeaderBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
			colors[ImGuiCol_TableRowBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.00f);
			colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
		}

		baseTheme = theme;
	}

	BaseTheme ImGuiManager::getBaseTheme() const
	{
		return baseTheme;
	}

	void ImGuiManager::shutdown()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		io = nullptr;
	}

	void ImGuiManager::begin()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiManager::draw(GLFWwindow* window)
	{
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(window);
		}
	}

	void ImGuiManager::loadFont(const std::string& filename, float size)
	{
		if (File::exists(filename))
			io->Fonts->AddFontFromFileTTF(filename.c_str(), size, NULL, io->Fonts->GetGlyphRangesChineseFull());
	}

	void ImGuiManager::loadIconFont(const std::string& filename, int start, int end, float size)
	{
		if (File::exists(filename))
		{
			ImFontConfig fontConfig;
			fontConfig.MergeMode = true;
			fontConfig.GlyphMinAdvanceX = 13.0f;
			static const ImWchar iconRanges[] = { start, end, 0 };

			io->Fonts->AddFontFromFileTTF(filename.c_str(), size, &fontConfig, iconRanges);
		}
	}

	void ImGuiManager::initializeLayout()
	{
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoBackground;

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 viewportOffset{ 0, UI::toolbarBtnSize.y + ImGui::GetStyle().WindowPadding.y + 5 };
		ImGui::SetNextWindowPos(viewport->WorkPos + viewportOffset);
		ImGui::SetNextWindowSize(viewport->WorkSize - viewportOffset);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("InvisibleWindow", nullptr, windowFlags); // This is basically the background window that contains all the dockable windows
		ImGui::PopStyleVar(3);

		std::string dockStrId{ "InvisibleWindowDockSpace-" };
		dockStrId.append(Localization::currentLanguage->getCode());

		ImGuiID dockSpaceId = ImGui::GetID(dockStrId.c_str());
		if (!ImGui::DockBuilderGetNode(dockSpaceId))
		{
			ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockSpaceId, viewport->WorkSize - viewportOffset);

			ImGuiID dockMainId = dockSpaceId;
			ImGuiID midRightId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Right, 0.25f, nullptr, &dockMainId);
			ImGuiID topRightId = ImGui::DockBuilderSplitNode(midRightId, ImGuiDir_Up, 0.15f, nullptr, &midRightId);
			ImGuiID bottomRightId = ImGui::DockBuilderSplitNode(midRightId, ImGuiDir_Down, 0.3f, nullptr, &midRightId);

			ImGui::DockBuilderDockWindow(IMGUI_TITLE(ICON_FA_MUSIC, "notes_timeline"), dockMainId);
			ImGui::DockBuilderDockWindow(IMGUI_TITLE(ICON_FA_ALIGN_LEFT, "chart_properties"), midRightId);
			ImGui::DockBuilderDockWindow(IMGUI_TITLE(ICON_FA_WRENCH, "settings"), topRightId);
			ImGui::DockBuilderDockWindow(IMGUI_TITLE(ICON_FA_DRAFTING_COMPASS, "presets"), bottomRightId);

			ImGui::DockBuilderFinish(dockMainId);
		}

		ImGui::DockSpace(dockSpaceId, ImVec2(), ImGuiDockNodeFlags_PassthruCentralNode);
		ImGui::End();
	}

	void ImGuiManager::applyAccentColor(int colIndex)
	{
		ImVec4* colors = ImGui::GetStyle().Colors;
		const ImVec4 color = UI::accentColors[colIndex].color;
		const ImVec4 darkColor = generateDarkColor(color);
		const ImVec4 lightColor = generateHighlightColor(color);

		colors[ImGuiCol_SliderGrab] = color;
		colors[ImGuiCol_SliderGrabActive] = darkColor;
		colors[ImGuiCol_ButtonActive] = darkColor;
		colors[ImGuiCol_SeparatorHovered] = lightColor;
		colors[ImGuiCol_TabHovered] = lightColor;
		colors[ImGuiCol_TabActive] = color;
		colors[ImGuiCol_CheckMark] = color;

		accentColor = colIndex;
	}

	int ImGuiManager::getAccentColor() const
	{
		return accentColor;
	}
}