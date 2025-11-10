#pragma once
#include "Rendering/Framebuffer.h"
#include "Rendering/Renderer.h"
#include "Rendering/Camera.h"
#include "Particle.h"
#include <memory>

namespace MikuMikuWorld::Effect
{
	class DebugEffectView
	{
	public:
		void update(Renderer* renderer);
		void load();
		void stop();
		void play();

		DebugEffectView();

	private:
		EmitterInstance testEmitter;

		bool initialized{ false };
		bool effectLoaded{ false };
		bool playing{ false };
		float time{};
		float timeFactor{ 1.f };
		std::unique_ptr<Framebuffer> previewBuffer;
		Texture* effectsTex{ nullptr };
		Camera camera;

		void init();
		void drawTest(EmitterInstance& emitter, Renderer* renderer);
	};
}