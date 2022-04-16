#pragma once
#include "Rendering/Camera.h"
#include "Rendering/Renderer.h"
#include "ImGui/imgui.h"
#include "Score.h"

namespace MikuMikuWorld
{
	class Framebuffer;

	class ScoreSimulator
	{
	private:
		const float previewWidth = 1920;
		const float previewHeight = 1080;
		float noteRotX = 35.0f;
		float noteScaleX = 0.10f;
		float noteSclaeY = 0.28f;
		float flickArrowRotX = -10.0f;
		float flickArrowBaseWidth = 0.080f;
		float flickArrowWidthFactor = 0.080f;
		float flickArrowBaseHeight = 0.200f;
		float flickArrowHeightFactor = 0.015f;
		float flickArrowYOffset = -0.15f;
		float flickArrowYOffsetFactor = -0.01f;
		float holdXOffset = -0.075f;
		float holdXOffsetFactor = -0.05f;
		float holdSideScale = 0.1f;
		const Vector2 judgeLinePos{ 0, -275 };

		DirectX::XMMATRIX transformMatrix;

		Camera camera;
		Framebuffer* framebuffer;

		float laneTransparency;
		float backgorundBrightness;
		float noteSpeed;
		float debugLaneFov = 45;
		float debugNoteXPos = 0;
		float transformAngle = 56.0f;
		float timeAdjustment = 1.50f;

		ImVec2 getPreviewDimensions(const ImVec2& windowSize);

		float getNoteOnScreenTime();
		float laneToPosition(float lane);
		float tickToPosition(int ticks, float time, const std::vector<Tempo>& tempos);

		void drawBackground(Renderer* renderer);
		void drawJacket(Renderer* renderer);
		void drawLanes(Renderer* renderer);
		void drawNotes(float time, const Score& score, Renderer* renderer);
		void drawNote(const Note& note, float time, const std::vector<Tempo>& tempos, Renderer* renderer);
		void drawHoldNote(const HoldNote& hold, const Score& score, float time, Renderer* renderer);
		void drawHoldCurve(const Note& n1, const Note& n2, EaseType ease, float time, const std::vector<Tempo>& tempos, Renderer* renderer);
		void updateSettings();
		void debug();

	public:
		ScoreSimulator();

		void update(float time, const Score& score, Renderer* renderer);
	};
}