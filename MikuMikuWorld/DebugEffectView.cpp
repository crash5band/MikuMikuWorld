#include "DebugEffectView.h"
#include "ImGuiManager.h"
#include "ResourceManager.h"
#include "Application.h"
#include "Particle.h"
#include <map>

using namespace nlohmann;

namespace MikuMikuWorld::Effect
{
	DebugEffectView::DebugEffectView()
	{
		camera.fov = 50.f;
		camera.pitch = 27.1f;
		camera.yaw = -90.f;
		camera.position.m128_f32[0] = 0;
		camera.position.m128_f32[1] = 5.32f;
		camera.position.m128_f32[2] = -5.86f;
		camera.positionCamNormal();

		std::srand(static_cast<unsigned int>(std::time(nullptr)));
	}

	void DebugEffectView::init()
	{
		previewBuffer = std::make_unique<Framebuffer>(1920, 1080);
		int texId = ResourceManager::getTexture("tex_note_common_all_v2");
		effectsTex = &ResourceManager::textures[texId];
		initialized = true;
	}

	static EmitterInstance createFromParticle(Transform& transform, int particleId)
	{
		Effect::Particle& p = ResourceManager::getParticleEffect(particleId);;

		EmitterInstance emitter;
		emitter.refID = particleId;
		emitter.startTime = p.startDelay.getValue();
		emitter.transform = p.transform;
		for (const auto& refBurst : p.emission.bursts)
			emitter.bursts.push_back({ 0, 0 });

		for (const auto& child : p.children)
			emitter.children.push_back(createFromParticle(p.transform, child));

		return emitter;
	}

	void DebugEffectView::play()
	{
		playing ^= true;
	}

	void DebugEffectView::stop()
	{
		playing = false;
		time = 0;

		testEmitter.stop(true);
	}

	void DebugEffectView::load()
	{
		IO::FileDialog fileDialog{};
		fileDialog.parentWindowHandle = Application::windowState.windowHandle;
		fileDialog.title = "Load Particle Effect";

		if (fileDialog.openFile() != IO::FileDialogResult::OK)
			return;

		ResourceManager::removeAllParticleEffects();

		int rootId = ResourceManager::loadParticleEffect(fileDialog.outputFilename);
		Transform baseTransform;
		testEmitter = createFromParticle(baseTransform, rootId);

		effectLoaded = true;
	}

	void DebugEffectView::update(Renderer* renderer)
	{
		if (!initialized)
			init();

		if (ImGui::Button("Load"))
			load();

		ImGui::SameLine();
		if (ImGui::Button("Play/Pause"))
			play();

		ImGui::SameLine();
		if (ImGui::Button("Stop"))
			stop();

		ImGui::SameLine();
		if (ImGui::Button("x0.1"))
			timeFactor = 0.1f;

		ImGui::SameLine();
		if (ImGui::Button("x0.25"))
			timeFactor = 0.25f;

		ImGui::SameLine();
		if (ImGui::Button("x0.5"))
			timeFactor = 0.5f;

		ImGui::SameLine();
		if (ImGui::Button("x1"))
			timeFactor = 1.f;

		ImGui::SameLine();
		ImGui::Text("Time: %.3f", time);
		ImGui::SameLine();
		ImGui::Text("Speed: %.2f", timeFactor);

		if (!playing)
		{
			time += ImGui::GetIO().MouseWheel * 0.1f;
		}

		ImVec2 position = ImGui::GetCursorScreenPos();
		ImVec2 size = ImGui::GetContentRegionAvail();

		if (previewBuffer->getWidth() != size.x || previewBuffer->getHeight() != size.y)
			previewBuffer->resize(size.x, size.y);

		previewBuffer->bind();
		previewBuffer->clear(0.1, 0.1, 0.1, 1);

		if (effectLoaded)
		{
			if (playing)
			{
				time += ImGui::GetIO().DeltaTime * timeFactor;
			}

			testEmitter.update(time, Transform());
			renderer->beginBatch();

			Shader* shader = ResourceManager::shaders[ResourceManager::getShader("particles")];
			Texture texture = ResourceManager::textures[ResourceManager::getTexture("particles")];

			float aspectRatio = size.x / size.y;

			const auto pView = camera.getViewMatrix();
			auto pProjection = camera.getProjectionMatrix(aspectRatio, 0.3, 1000);
			float projectionScale = std::min(aspectRatio / (16.f / 9.f), 1.f);
			pProjection = DirectX::XMMatrixScaling(projectionScale, projectionScale, 1.f) * pProjection;

			shader->use();
			shader->setMatrix4("projection", pProjection);
			shader->setMatrix4("view", pView);

			drawTest(testEmitter, renderer);
			renderer->endBatchWithBlending(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}

		ImGui::GetWindowDrawList()->AddImage((ImTextureID)(size_t)previewBuffer->getTexture(), position, position + size);
		previewBuffer->unblind();
	}

	void DebugEffectView::drawTest(EmitterInstance& emitter, Renderer* renderer)
	{
		const Particle& ref = ResourceManager::getParticleEffect(emitter.refID);
		for (auto& particle : emitter.particles)
		{
			float xAngle = particle.transform.rotation.x;;
			if (ref.renderMode == RenderMode::HorizontalBillboard)
				xAngle = 90.f;

			DirectX::XMMATRIX m = DirectX::XMMatrixIdentity();
			m *= DirectX::XMMatrixTranslation(ref.pivot.x, ref.pivot.y, ref.pivot.z);
			m *= DirectX::XMMatrixScaling(particle.transform.scale.x / 1.5f, particle.transform.scale.y / 1.5f, particle.transform.scale.z / 1.5f);
			m *= DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(particle.transform.rotation.z));
			m *= DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(xAngle));
			m *= DirectX::XMMatrixTranslation(particle.transform.position.x, particle.transform.position.y, particle.transform.position.z);

			float blend = ref.blend == BlendMode::Additive ? 1.f : 0.f;
			int frame = ref.textureSplitX * ref.textureSplitY * ref.startFrame.constant;
			renderer->drawQuadWithBlend(m, *effectsTex, ref.textureSplitX, ref.textureSplitY, frame, particle.color, ref.order, blend);
		}

		for (auto& child : emitter.children)
			drawTest(child, renderer);
	}
}