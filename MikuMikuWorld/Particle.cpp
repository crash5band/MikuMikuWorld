#include "Particle.h"
#include "DirectXMath.h"
#include "ResourceManager.h"
#include "Rendering/Camera.h"
#include <random>

namespace MikuMikuWorld::Effect
{
	constexpr float GRAVITY = 9.81f;

	static DirectX::XMMATRIX rotateToDirection(const ParticleInstance& p, const Particle& ref, Vector3 velocity)
	{
		DirectX::XMMATRIX direction = DirectX::XMMatrixIdentity();
		float magnitude = velocity.normalize();

		Vector3 up{ 0, -1, 0 };
		float t;
		Vector3 axis = up.crossProduct(velocity);
		float angle = axis.length();

		if (angle >= 0.000001f)
		{
			angle = asinf(std::min(angle, 1.0f));
		}
		else
		{
			angle = 0.0f;
			axis.x = up.z;
			axis.y = 0.0f;
			axis.z = up.x;
			t = axis.length();
			if (t < 0.000001f)
			{
				axis.x = -up.y;
				axis.y = up.x;
				axis.z = 0.0f;
			}
		}

		t = up.dotProduct(velocity);
		if (t < 0.0f)
			angle = PI - angle;

		float speedScale = magnitude * ref.speedScale;
		float lengthScale = ref.lengthScale;
		DirectX::XMVECTOR stretchScaling{ 1, speedScale + lengthScale, 1, 1 };

		direction *= DirectX::XMMatrixRotationZ(1.5708f);
		direction *= DirectX::XMMatrixScalingFromVector(stretchScaling);
		direction *= DirectX::XMMatrixRotationAxis(DirectX::XMVECTOR{ axis.x, axis.y, axis.z, 1.0f }, angle);
		//direction *= DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(180));
		return direction;
	}

	// TODO: it would be much better if this was done in O(1)
	int EmitterInstance::findFirstDeadParticle(float time) const
	{
		for (int i = 0; i < particles.size(); i++)
		{
			if (!particles[i].alive)
				return i;
		}

		return -1;
	}

	// TODO: make sure this calculates the correct number!
	int EmitterInstance::getMaxParticleCount() const
	{
		const Particle& ref = ResourceManager::getParticleEffect(refID);

		int count = (rateOverTime * ref.duration) + 1;
		for (const auto& burst : ref.emission.bursts)
		{
			if (burst.cycles == 0)
			{
				float cycleCount = std::ceilf((ref.duration - burst.time) / burst.interval) + 1;
				count += cycleCount * burst.count;
			}
			else
			{
				count += burst.count * burst.cycles;
			}
		}

		return std::min(count, ref.maxParticles);
		//return ref.maxParticles;
	}

	void EmitterInstance::init(const Particle& ref)
	{
		refID = ref.ID;

		bursts.reserve(ref.emission.bursts.size());
		for (const auto& refBurst : ref.emission.bursts)
			bursts.push_back({});

		for (int i = 0; i < children.size(); i++)
		{
			const Particle& childRef = ResourceManager::getParticleEffect(ref.children[i]);
			children[i].init(childRef);
		}
	}

	void EmitterInstance::emit(const Transform& shift, const Transform& baseTransform, const Particle& ref, float time)
	{
		int instanceIndex = findFirstDeadParticle(time);
		if (instanceIndex == -1)
			return;

		Quaternion qBase{};
		qBase.fromEulerDegrees(baseTransform.rotation);

		Quaternion qBaseRef{};
		qBaseRef.fromEulerDegrees(baseTransform.rotation + ref.transform.rotation);

		Quaternion qAll{};
		qAll.fromEulerDegrees(baseTransform.rotation + ref.transform.rotation + ref.emission.transform.rotation);

		Vector3 transformPos = qBase * (ref.transform.position * baseTransform.scale);
		Vector3 basePos = qBase * (baseTransform.position * baseTransform.scale);
		Vector3 shapePos = qBaseRef * ref.emission.transform.position;

		Vector3 position = transformPos + basePos + shapePos;

		float length = ref.startSpeed.evaluate(random.get());

		Vector3 emitPosition{}, direction{}, forward{ 0, 0, 1 };
		if (ref.emission.shape == EmissionShape::Box)
		{
			Vector3 halfScale = ref.emission.transform.scale * .5f;
			emitPosition.x = random.get(-halfScale.x, halfScale.x);
			emitPosition.y = random.get(-halfScale.y, halfScale.y);
			emitPosition.z = random.get(-halfScale.z, halfScale.z);
			
			emitPosition = (qAll * emitPosition) + position;
			direction = qBase * forward;
		}
		else if (ref.emission.shape == EmissionShape::Cone)
		{
			float arc = DirectX::XMConvertToRadians(random.get(0, ref.emission.arc));
			float angle = DirectX::XMConvertToRadians(ref.emission.angle);
			float radius = random.get(ref.emission.radius * (1 - ref.emission.radiusThickness), ref.emission.radius);
			float localRadius = tanf(arc) * length;

			emitPosition.x = cosf(arc) * radius * ref.emission.transform.scale.x;
			emitPosition.y = sinf(arc) * radius * ref.emission.transform.scale.y;
			switch (ref.emission.emitFrom)
			{
			case EmitFrom::Base:
				emitPosition.z = 0;
				break;
			case EmitFrom::Volume:
				emitPosition.z = random.get(0, length);
				break;
			}

			emitPosition = (qAll * emitPosition) + position;
			Vector3 posNormal = emitPosition;
			posNormal.normalize();

			direction = Vector3(
				posNormal.x * sinf(angle),
				posNormal.y * sinf(angle),
				cosf(angle)
			);

			direction = qAll * direction;
		}
		else if (ref.emission.shape == EmissionShape::Circle)
		{
			float angle{};
			switch (ref.emission.arcMode)
			{
			case ArcMode::Loop:
				angle = emissionPosition;
				break;
			case ArcMode::BurstSpread:
				angle = emissionPosition;
				emissionPosition += emissionPositionInterval;
				break;
			default:
				angle = random.get(0, DirectX::XMConvertToRadians(ref.emission.arc));
			}

			float radius = random.get(ref.emission.radius * (1 - ref.emission.radiusThickness), ref.emission.radius);

			emitPosition.x = cosf(angle) * radius * ref.emission.transform.scale.x;
			emitPosition.y = sinf(angle) * radius * ref.emission.transform.scale.y;
			emitPosition = qAll * emitPosition;

			direction = emitPosition;
			emitPosition += position;
		}
		else if (ref.emission.shape == EmissionShape::Sphere)
		{
			float angle = random.get(0, DirectX::XMConvertToRadians(ref.emission.arc));
			float angle2 = random.get(0, DirectX::XMConvertToRadians(180));
			float radius = random.get(ref.emission.radius * (1 - ref.emission.radiusThickness), ref.emission.radius);

			emitPosition.x = cosf(angle) * sinf(angle2) * radius * ref.emission.transform.scale.x;
			emitPosition.y = sinf(angle) * radius * ref.emission.transform.scale.y;
			emitPosition.z = cosf(angle) * cosf(angle2) * radius * ref.emission.transform.scale.z;
			emitPosition = qAll * emitPosition;

			direction = emitPosition;
			emitPosition += position;
		}
		else if (ref.emission.shape == EmissionShape::HemiShpere)
		{
			float angle = random.get(0, DirectX::XMConvertToRadians(ref.emission.arc));
			float angle2 = random.get(0, PI / 2.f);
			float radius = random.get(ref.emission.radius * (1 - ref.emission.radiusThickness), ref.emission.radius);

			emitPosition.x = cosf(angle) * sinf(angle2) * radius * ref.emission.transform.scale.x;
			emitPosition.y = sinf(angle) * sinf(angle2) * radius * ref.emission.transform.scale.y;
			emitPosition.z = cosf(angle2) * radius * ref.emission.transform.scale.z;
			emitPosition = qAll * emitPosition;

			direction = emitPosition;
			emitPosition += position;
		}

		direction.normalize();

		ParticleInstance& instance = particles[instanceIndex];
		instance.alive = true;
		instance.transform.position = emitPosition;
		instance.transform.rotation = ref.startRotation.evaluate(0, random.get(), 0);
		instance.transform.scale = ref.transform.scale * ref.startSize.evaluate(0, random.get(), 1);
		instance.direction = direction;
		instance.startColor = ref.startColor.evaluate(random.get());
		instance.speed = length;
		instance.startTime = time;
		instance.duration = ref.startLifeTime.evaluate(random.get());
		instance.time = 0;
		
		instance.textureStartFrameLerpRatio = random.get();
		instance.textureFrameOverTimeLerpRatio = random.get();
		instance.velocityOverLifetimeLerpRatio = random.get();
		instance.colorOverLifeTimeLerpRatio = random.get();
		instance.sizeOverLifetimeLerpRatio = random.get();
		instance.gravityModifierLerpRatio = random.get();
		instance.speedModifierLerpRatio = random.get();
		instance.limitVelocityLerpRatio = random.get();
		instance.forceOverTimeLerpRatio = random.get();
		instance.rotationOverLifetimeLerpRatio = random.get();

		// shift will be included during emission for world space
		// for local space, the shift will be included during particle update
		if (ref.simulationSpace == TransformSpace::World)
		{
			Quaternion qShift{};
			qShift.fromEulerDegrees(shift.rotation);
			instance.transform.position += (qShift * shift.position);
			instance.transform.rotation += shift.rotation;
		}

		std::knuth_b rand_engine;
		std::bernoulli_distribution flipRotationRoll(ref.flipRotation);

		instance.flipRotation = flipRotationRoll(rand_engine);
	}

	void EmitterInstance::updateEmission(const Particle& ref, const Transform& shift, const Transform& baseTransform, float time)
	{
		// TODO: make these frame-rate independent!
		if (!ref.looping && time >= startTime + ref.duration)
			return;

		if (rateOverTime > 0.f)
		{
			float emissionInterval = emissionAccumulator == 0 ? 0 : 1.f / rateOverTime;
			float nextEmissionTime = startTime + lastEmissionTime + emissionInterval;
			if (time >= nextEmissionTime)
			{
				emissionPositionInterval = ref.emission.arc / rateOverTime;
				emissionPosition = 0;
				emissionAccumulator++;
				lastEmissionTime = time - startTime;
				emit(shift, baseTransform, ref, nextEmissionTime);
			}
		}

		for (int i = 0; i < ref.emission.bursts.size(); i++)
		{
			const EmissionBurst& refBurst = ref.emission.bursts.at(i);
			BurstInstance& burst = bursts.at(i);

			if (time >= burst.nextCyclesResetTime)
			{
				burst.cycleCount = 0;
				burst.nextCyclesResetTime += ref.duration;
			}

			if (burst.cycleCount < refBurst.cycles || refBurst.cycles == 0)
			{
				float interval = burst.cycleCount == 0 ? 0 : refBurst.interval;
				float nextBurstTime = burst.lastBurstTime + interval + refBurst.time + startTime;
				if (time >= nextBurstTime)
				{
 					emissionPosition = 0;
					emissionPositionInterval = ref.emission.arc / static_cast<float>(refBurst.count);
					burst.lastBurstTime = time - startTime;
					burst.cycleCount++;

					for (int i = 0; i < refBurst.count; i++)
						emit(shift, baseTransform, ref, time);
				}
			}
		}
	}

	void EmitterInstance::start(float time)
	{
		const Particle& ref = ResourceManager::getParticleEffect(refID);
		startTime = time + ref.startDelay.evaluate(0, random.get());
		rateOverTime = ref.emission.rateOverTime.evaluate(0, random.get());

		int maxParticleCount = getMaxParticleCount();
		particles.reserve(maxParticleCount);
		
		if (maxParticleCount > particles.size())
			particles.insert(particles.end(), maxParticleCount - particles.size(), {});

		for (auto& burst : bursts)
			burst.nextCyclesResetTime = startTime + ref.duration;

		for (auto& child : children)
			child.start(time);
	}

	void EmitterInstance::stop(bool allChildren)
	{
		lastEmissionTime = 0;
		emissionAccumulator = 0;
		emissionPosition = 0;
		emissionPositionInterval = 0;
		for (auto& burst : bursts)
		{
			burst.lastBurstTime = 0;
			burst.cycleCount = 0;
			burst.nextCyclesResetTime = 0;
		}

		for (auto& p : particles)
			p.alive = false;

		if (allChildren)
		{
			for (auto& child : children)
				child.stop(allChildren);
		}
	}

	static Vector3& limitVelocity(Vector3& velocity, const Vector3& limitVelocity, float damp, float time)
	{
		// Extracted from UnityPlayer.dll
		float k = powf(1 - damp, time * 30);

		float signX = velocity.x >= 0 ? 1 : -1;
		float absX = abs(velocity.x);
		if (absX > 0.0001 && absX > limitVelocity.x)
		{
			absX = limitVelocity.x + (absX - limitVelocity.x) * k;
			velocity.x = absX * signX;
		}

		float signY = velocity.y >= 0 ? 1 : -1;
		float absY = abs(velocity.y);
		if (absY > 0.0001 && absY > limitVelocity.y)
		{
			absY = limitVelocity.y + (absY - limitVelocity.y) * k;
			velocity.y = absY * signY;
		}

		float signZ = velocity.z >= 0 ? 1 : -1;
		float absZ = abs(velocity.z);
		if (absZ > 0.0001 && absZ > limitVelocity.z)
		{
			absZ = limitVelocity.z + (absZ - limitVelocity.z) * k;
			velocity.z = absZ * signZ;
		}

		return velocity;
	}

	void EmitterInstance::update(float t, const Transform& shift, Transform& baseTransform, const Camera& camera)
	{
		const Particle& ref = ResourceManager::getParticleEffect(refID);

		updateEmission(ref, shift, baseTransform, t);

		DirectX::XMMATRIX inverseView = DirectX::XMMatrixIdentity();
		inverseView.r[3] = DirectX::XMVECTOR{ 0, 0, 0, 1 };
		inverseView *= DirectX::XMMatrixInverse(nullptr, camera.getViewMatrix());
		inverseView.r[3] = DirectX::XMVECTOR{ 0, 0, 0, 1 };

		Vector3 simulationSpaceRotation = baseTransform.rotation;
		if (ref.simulationSpace == TransformSpace::Local)
		{
			//simulationSpaceRotation += shift.rotation;
		}

		Quaternion qLocal{};
		qLocal.fromEulerDegrees(simulationSpaceRotation);

		Quaternion qShift{};
		qShift.fromEulerDegrees(shift.rotation);

		for (auto& particle : particles)
		{
			float prevTime = particle.time;
			particle.time = std::max(0.f, t - particle.startTime);
			
			if (particle.time >= particle.duration)
				particle.alive = false;

			if (!particle.alive)
				continue;

			float normalizedTime = particle.time / particle.duration;
			float dt = particle.time - prevTime;

			Vector3 currentRotation = baseTransform.rotation + ref.transform.rotation + particle.transform.rotation + shift.rotation;
			if (ref.rotationOverLifetime.enabled)
			{
				currentRotation += ref.rotationOverLifetime.integrate(0, normalizedTime, particle.duration, particle.rotationOverLifetimeLerpRatio);
			}

			// Apparently, the transform scale affects velocity too
			Vector3 currentScale = particle.transform.scale;
			Vector3 velocityScale = ref.transform.scale;
			if (ref.scalingMode == ScalingMode::Hierarchy)
			{
				currentScale *= baseTransform.scale;
				velocityScale *= baseTransform.scale;
			}

			if (ref.sizeOverLifetime.enabled)
				currentScale *= ref.sizeOverLifetime.evaluate(normalizedTime, particle.sizeOverLifetimeLerpRatio, 1.f);

			Vector3 currentVelocity{};
			if (ref.velocityOverLifetime.enabled)
			{
				Vector3 vol = ref.velocityOverLifetime.evaluate(normalizedTime, particle.velocityOverLifetimeLerpRatio);
				if (ref.velocitySpace == TransformSpace::Local)
				{
					vol = qLocal * vol;
				}
				currentVelocity += vol;
			}

			if (ref.forceOverLifetime.enabled)
			{
				Vector3 fol1 = ref.forceOverLifetime.integrate(0, normalizedTime, particle.duration, particle.forceOverTimeLerpRatio);
				if (ref.forceSpace == TransformSpace::Local)
				{
					fol1 = qLocal * fol1;
				}
				currentVelocity += fol1;
			}

			currentVelocity += particle.direction * particle.speed;
			currentVelocity *= ref.speedModifier.evaluate(normalizedTime, particle.speedModifierLerpRatio, 1.f);
			currentVelocity.y -= GRAVITY * ref.gravityModifier.evaluate(normalizedTime, particle.gravityModifierLerpRatio) * particle.time;
			
			Vector3 velocityLimit = ref.limitVelocityOverLifetime.evaluate(normalizedTime, particle.limitVelocityLerpRatio);
			currentVelocity = limitVelocity(currentVelocity, velocityLimit, ref.limitVelocityDampen, particle.time);

			particle.transform.position += currentVelocity * dt * velocityScale;

			float rotationFactor = particle.flipRotation ? -1 : 1;
			float velocityMagnitude = currentVelocity.length();

			particle.matrix = DirectX::XMMatrixIdentity();
			DirectX::XMMATRIX directionMatrix = DirectX::XMMatrixIdentity();
			switch (ref.renderMode)
			{
			case RenderMode::Billboard:
				switch (ref.alignment)
				{
				case AlignmentMode::View:
					directionMatrix = inverseView;
					currentRotation = particle.transform.rotation;
					break;
				}
				break;
			case RenderMode::StretchedBillboard:
				directionMatrix *= inverseView;
				directionMatrix *= rotateToDirection(particle, ref, currentVelocity);
				currentRotation = { 0, 0, 0 };
				//currentScale.y *= (velocityMagnitude * ref.speedScale) * 10.5f;
				//currentScale.x *= currentScale.y * ref.lengthScale * 5.5f;
				break;
			case RenderMode::HorizontalBillboard:
				currentRotation.x = 90.f * rotationFactor;
				currentRotation.y = 0.f;
				currentScale *= 0.7f;
				break;
			case RenderMode::VerticalBillboard:
				// TODO: figure out the correct matrix for this
				directionMatrix = inverseView;
				currentScale *= 0.7f;
				break;
			}

			currentRotation *= rotationFactor;

			Vector3 translation = particle.transform.position;
			if (ref.simulationSpace == TransformSpace::Local)
			{
				translation = qShift * translation;
				translation += shift.position;

				if (abs(shift.rotation.z) > 0.f)
					currentRotation.z *= -1;
			}

			// I do not like this but I can't think of any other decent way for now...
			if (ref.name == "aura")
				currentScale *= shift.scale;

			particle.matrix *= DirectX::XMMatrixTranslation(ref.pivot.x, ref.pivot.y, ref.pivot.z);
			particle.matrix *= DirectX::XMMatrixScaling(currentScale.x, currentScale.y, currentScale.z);
			particle.matrix *= directionMatrix;
			particle.matrix *= DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(-currentRotation.z));
			particle.matrix *= DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(currentRotation.y));
			particle.matrix *= DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(currentRotation.x));
			particle.matrix *= DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z);
		}

		// Do we need to pass a copy here?
		Transform chilldTransform = baseTransform;
		chilldTransform.rotation = baseTransform.rotation + ref.transform.rotation;
		chilldTransform.position = baseTransform.position + ref.transform.position;
		chilldTransform.scale = baseTransform.scale * ref.transform.scale;
		for (auto& em : children)
		{
			em.update(t, shift, chilldTransform, camera);
		}
	}
}