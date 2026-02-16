#include "DebugEffectView.h"
#include "ImGuiManager.h"
#include "ResourceManager.h"
#include "Particle.h"

using namespace nlohmann;

namespace MikuMikuWorld::Effect
{
	DebugEffectView::DebugEffectView()
	{
		camera = std::make_unique<Camera>();
		resetCamera();
	}

	void DebugEffectView::resetCamera()
	{
		camera->setFov(50.f);
		camera->setRotation(-90.f, 27.1f);
		camera->setPosition({ 0, 5.32f, -5.86f, 0.f });
		camera->positionCamNormal();
	}

	static EmitterInstance createFromParticle(int particleId)
	{
		Effect::Particle& p = ResourceManager::getParticleEffect(particleId);

		EmitterInstance emitter;
		for (const int child : p.children)
		{
			emitter.children.emplace_back(createFromParticle(child));
		}

		return emitter;
	}

	void DebugEffectView::init()
	{
		previewBuffer = std::make_unique<Framebuffer>(1920, 1080);
		loadEffects();
	}

	void DebugEffectView::loadEffects()
	{
		int texId = ResourceManager::getTexture("tex_note_common_all_v2");
		effectsTex = &ResourceManager::textures[texId];

		int count = static_cast<int>(EffectType::fx_count);
		effects.clear();
		effects.reserve(count);

		const Transform transform{};
		for (int i = 0; i < count; i++)
		{
			int particleId = ResourceManager::getRootParticleIdByName(effectNames[i]);
			const Particle& ref = ResourceManager::getParticleEffect(particleId);
			effects.push_back(createFromParticle(particleId));

			effects[effects.size() - 1].init(ref, transform);
			effects[effects.size() - 1].start(0);
		}

		initialized = true;
	}

	void DebugEffectView::play()
	{
		playing ^= true;
	}

	void DebugEffectView::stop()
	{
		playing = false;
		time = 0;

		effects[static_cast<int>(selectedEffect)].stop(true);
		effects[static_cast<int>(selectedEffect)].start(time);
	}

	static void setEmitterView(EmitterInstance& em)
	{
		const Particle& ref = ResourceManager::getParticleEffect(em.getRefID());

		ImGui::PushID(ref.ID);
		ImGui::Checkbox(ref.name.c_str(), &em.visible);
		ImGui::PopID();

		for (auto& child : em.children)
			setEmitterView(child);
	}

	void DebugEffectView::update(Renderer* renderer)
	{
		if (!initialized)
			init();

		ImGui::Begin(IMGUI_TITLE("", "DBG_EFF"), NULL);
		//if (ImGui::Button("Load"))
		//	load();

		int selectedIndex = static_cast<int>(selectedEffect);
		int count = static_cast<int>(Effect::EffectType::fx_count);
		ImGui::SetNextItemWidth(300);
		if (ImGui::BeginCombo("Effect", effectNames[selectedIndex]))
		{
			for (int i = 0; i < count; ++i)
			{
				if (ImGui::Selectable(effectNames[i], selectedIndex == i))
					selectedEffect = static_cast<Effect::EffectType>(i);
			}

			ImGui::EndCombo();
		}

		ImGui::SameLine();
		if (ImGui::Button("Play/Pause"))
			play();

		ImGui::SameLine();
		if (ImGui::Button("Stop"))
			stop();

		ImGui::SameLine();
		if (ImGui::Button("CAM_RST"))
			resetCamera();

		ImGui::SameLine();
		if (ImGui::Button("TRS"))
			ImGui::OpenPopup("DBG_EFF_TRS");

		if (ImGui::BeginPopup("DBG_EFF_TRS"))
		{
			DirectX::XMFLOAT3 xmPos{}, xmRot{}, xmScale{};
			DirectX::XMStoreFloat3(&xmPos, debugTransform.position);
			DirectX::XMStoreFloat3(&xmRot, debugTransform.rotation);
			DirectX::XMStoreFloat3(&xmScale, debugTransform.scale);

			float position[] = { xmPos.x, xmPos.y, xmPos.z };
			float rotation[] = { xmRot.x, xmRot.y, xmRot.z };
			float scale[] = { xmScale.x, xmScale.y, xmScale.z };

			if (ImGui::DragFloat3("Position", position))
				debugTransform.position = { position[0], position[1], position[2] };

			if (ImGui::DragFloat3("Rotation", rotation))
				debugTransform.rotation = { rotation[0], rotation[1], rotation[2] };

			if (ImGui::DragFloat3("Scale", scale))
				debugTransform.scale = { scale[0], scale[1], scale[2] };

			ImGui::EndPopup();
		}

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

		ImVec2 position = ImGui::GetCursorScreenPos();
		ImVec2 size = ImGui::GetContentRegionAvail();
		ImRect boundaries{ position, position + size };

		if (previewBuffer->getWidth() != size.x || previewBuffer->getHeight() != size.y)
			previewBuffer->resize(size.x, size.y);

		previewBuffer->bind();
		previewBuffer->clear(0.1, 0.1, 0.1, 1);

		bool isWindowActive = !ImGui::IsWindowDocked() || ImGui::GetCurrentWindow()->TabId == ImGui::GetWindowDockNode()->SelectedTabId;
		if (isWindowActive && ImGui::IsMouseHoveringRect(boundaries.Min, boundaries.Max))
		{
			ImGuiIO& io = ImGui::GetIO();
			bool updateCamera = false;
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && io.KeyCtrl)
			{
				camera->rotate(io.MouseDelta[0], io.MouseDelta[1]);
				updateCamera = true;
			}

			if (abs(io.MouseWheel) > 0 && io.KeyCtrl)
			{
				camera->zoom(io.MouseWheel);
				updateCamera = true;
			}

			if (updateCamera)
				camera->positionCamNormal();

			time += ImGui::GetIO().MouseWheel * 0.1f * io.KeyAlt * !playing;
		}

		if (playing)
			time += ImGui::GetIO().DeltaTime * timeFactor;

		static Transform baseTransform;

		EmitterInstance& emitter = effects[static_cast<int>(selectedEffect)];
		emitter.update(time, debugTransform, *camera);
		renderer->beginBatch();

		Shader* shader = ResourceManager::shaders[ResourceManager::getShader("particles")];

		float aspectRatio = size.x / size.y;

		const auto pView = camera->getViewMatrix();
		auto pProjection = camera->getProjectionMatrix(aspectRatio, 0.3, 1000);
		float projectionScale = std::min(aspectRatio / (16.f / 9.f), 1.f);
		pProjection = DirectX::XMMatrixScaling(projectionScale, projectionScale, 1.f) * pProjection;

		shader->use();
		shader->setMatrix4("projection", pProjection);
		shader->setMatrix4("view", pView);

		drawTest(emitter, renderer, time);
		renderer->endBatchWithBlending(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		static ImVec2 uv0{ 0, 1 };
		static ImVec2 uv1{ 1, 0 };
		ImGui::GetWindowDrawList()->AddImage((ImTextureID)(size_t)previewBuffer->getTexture(), position, position + size, uv0, uv1);
		previewBuffer->unblind();
		ImGui::End();

		ImGui::Begin("DBG_EFF_VIEW");

		setEmitterView(emitter);

		ImGui::End();
	}

	void DebugEffectView::drawTest(EmitterInstance& emitter, Renderer* renderer, float time)
	{
		const Particle& ref = ResourceManager::getParticleEffect(emitter.getRefID());
		int flipUVs = ref.renderMode == RenderMode::StretchedBillboard ? 1 : 0;

		if (emitter.visible)
		{
			for (auto& particle : emitter.particles)
			{
				if (!particle.alive)
					continue;

				float normalizedTime = particle.time / particle.duration;
				float blend = ref.blend == BlendMode::Additive ? 1.f : 0.f;

				int frame = ref.textureSplitX * ref.textureSplitY * ref.startFrame.evaluate(particle.time, particle.spriteSheetLerpRatio);
				frame += ref.textureSplitX * ref.textureSplitY * ref.frameOverTime.evaluate(normalizedTime, particle.spriteSheetLerpRatio);

				Color color = particle.startColor * ref.colorOverLifetime.evaluate(normalizedTime, particle.colorLerpRatio);

				// TODO: We should provide a second Z-Index according to the notes order.
				// Maybe use the note's tick?
				renderer->drawQuadWithBlend(particle.matrix, *effectsTex, ref.textureSplitX, ref.textureSplitY, frame, color, ref.order, blend, flipUVs);
			}
		}

		for (auto& child : emitter.children)
			drawTest(child, renderer, time);
	}
}