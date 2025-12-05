#pragma once
#include "Rendering/Framebuffer.h"
#include "Rendering/Renderer.h"
#include "Rendering/Camera.h"
#include "Particle.h"
#include "EffectView.h"
#include <memory>

namespace MikuMikuWorld::Effect
{
	class DebugEffectView
	{
	public:
		void update(Renderer* renderer);
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
		Effect::EffectType selectedEffect{};
		std::unique_ptr<Framebuffer> previewBuffer;
		std::vector<EmitterInstance> effects;
		Texture* effectsTex{ nullptr };
		Camera camera;
		Transform debugTransform;

		void init();
		void drawTest(EmitterInstance& emitter, Renderer* renderer, float time);
		void resetCamera();
	};
}