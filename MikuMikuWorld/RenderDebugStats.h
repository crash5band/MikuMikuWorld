#pragma once
#include "Rendering/Renderer.h"

namespace mmw = MikuMikuWorld;

namespace Debug
{
	class DebugRenderStats
	{
	public:
		float renderCpuTime{};

		void addStats(mmw::Renderer* renderer)
		{
			renderQuadsThisFrame += renderer->getNumQuads();
			renderVerticiesThisFrame += renderer->getNumVertices();
		}

		void clear()
		{
			renderQuadsThisFrame = renderVerticiesThisFrame = 0;
		}

		inline int getQuads() const { return renderQuadsThisFrame; }
		inline int getVerticies() const { return renderVerticiesThisFrame; }
		inline float getRenderCpuTime() const { return renderCpuTime; }

	private:
		int renderQuadsThisFrame;
		int renderVerticiesThisFrame;
	};

}