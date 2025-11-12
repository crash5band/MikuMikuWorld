#include "Particle.h"
#include "DirectXMath.h"
#include "ResourceManager.h"
#include "Rendering/Camera.h"
#include <random>

namespace MikuMikuWorld::Effect
{
	constexpr float GRAVITY = 9.81f;
	constexpr DirectX::XMVECTOR billboardScale{ 0.7f, 0.7f, 0.7f, 1.f };

	static DirectX::XMVECTOR quaternionFromZYX(const DirectX::XMVECTOR& euler)
	{
		float xRad = DirectX::XMConvertToRadians(DirectX::XMVectorGetX(euler));
		float yRad = DirectX::XMConvertToRadians(DirectX::XMVectorGetY(euler));
		float zRad = DirectX::XMConvertToRadians(DirectX::XMVectorGetZ(euler));

		DirectX::XMVECTOR qX = DirectX::XMQuaternionRotationAxis({ 1, 0, 0, 0 }, xRad);
		DirectX::XMVECTOR qY = DirectX::XMQuaternionRotationAxis({ 0, 1, 0, 0 }, yRad);
		DirectX::XMVECTOR qZ = DirectX::XMQuaternionRotationAxis({ 0, 0, 1, 0 }, zRad);

		return DirectX::XMQuaternionMultiply(DirectX::XMQuaternionMultiply(qZ, qY), qX);
	}

	//static DirectX::XMMATRIX rotateToDirection(const ParticleInstance& p, const Particle& ref, DirectX::XMFLOAT3& velocity)
	//{
	//	DirectX::XMMATRIX direction = DirectX::XMMatrixIdentity();
	//	DirectX::XMVECTOR v = DirectX::XMLoadFloat3(&velocity);
	//	DirectX::XMVECTOR magnitude = DirectX::XMVector3Length(v);
	//	v = DirectX::XMVector3Normalize(v);

	//	DirectX::XMVECTOR up{ 0, -1, 0, 0 };
	//	float t{};
	//	DirectX::XMVECTOR axis = DirectX::XMVector3Cross(up, v);
	//	float angle = DirectX::XMVectorGetX(DirectX::XMVector3Length(axis));

	//	if (angle >= 0.000001f)
	//	{
	//		angle = asinf(std::min(angle, 1.0f));
	//	}
	//	else
	//	{
	//		angle = 0.0f;
	//		axis.x = up.z;
	//		axis.y = 0.0f;
	//		axis.z = up.x;
	//		t = axis.length();
	//		if (t < 0.000001f)
	//		{
	//			axis.x = -up.y;
	//			axis.y = up.x;
	//			axis.z = 0.0f;
	//		}
	//	}

	//	t = up.dotProduct(velocity);
	//	if (t < 0.0f)
	//		angle = PI - angle;

	//	float speedScale = magnitude * ref.speedScale;
	//	float lengthScale = ref.lengthScale;
	//	DirectX::XMVECTOR stretchScaling{ 1, speedScale + lengthScale, 1, 1 };

	//	direction *= DirectX::XMMatrixRotationZ(1.5708f);
	//	direction *= DirectX::XMMatrixScalingFromVector(stretchScaling);
	//	direction *= DirectX::XMMatrixRotationAxis(DirectX::XMVECTOR{ axis.x, axis.y, axis.z, 1.0f }, angle);
	//	//direction *= DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(180));
	//	return direction;
	//}

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

		DirectX::XMVECTOR qBase = quaternionFromZYX(baseTransform.rotation);

		DirectX::XMVECTOR baseAndRefRotation = DirectX::XMVectorAdd(baseTransform.rotation, ref.transform.rotation);
		DirectX::XMVECTOR qBaseRef = quaternionFromZYX(baseAndRefRotation);

		DirectX::XMVECTOR qAll = quaternionFromZYX(DirectX::XMVectorAdd(baseAndRefRotation, ref.emission.transform.rotation));

		DirectX::XMVECTOR transformPos = DirectX::XMVector3Rotate(DirectX::XMVectorMultiply(ref.transform.position, baseTransform.scale), qBase);
		DirectX::XMVECTOR basePos = DirectX::XMVector3Rotate(DirectX::XMVectorMultiply(baseTransform.position, baseTransform.scale), qBase);
		DirectX::XMVECTOR shapePos = DirectX::XMVector3Rotate(ref.emission.transform.position, qBaseRef);

		DirectX::XMVECTOR position = DirectX::XMVectorAdd(DirectX::XMVectorAdd(transformPos, basePos), shapePos);

		float length = ref.startSpeed.evaluate(random.get());

		DirectX::XMVECTOR emitPosition = DirectX::XMVectorSet(0, 0, 0, 1);
		DirectX::XMVECTOR direction = DirectX::XMVectorSet(0, 0, 0, 0);
		DirectX::XMVECTOR forward = DirectX::XMVectorSet(0, 0, 1, 0);
		if (ref.emission.shape == EmissionShape::Box)
		{
			DirectX::XMVECTOR halfScale = DirectX::XMVectorScale(ref.emission.transform.scale, .5f);
			DirectX::XMFLOAT3 halves{};
			DirectX::XMStoreFloat3(&halves, halfScale);
			emitPosition = DirectX::XMVectorSet(
				random.get(-halves.x, halves.x),
				random.get(-halves.y, halves.y),
				random.get(-halves.z, halves.z),
				1
			);

			emitPosition = DirectX::XMVectorAdd(DirectX::XMVector3Rotate(emitPosition, qAll), position);
			direction = DirectX::XMVector3Rotate(forward, qBase);
		}
		else if (ref.emission.shape == EmissionShape::Cone)
		{
			float arc = DirectX::XMConvertToRadians(random.get(0, ref.emission.arc));
			float angle = DirectX::XMConvertToRadians(ref.emission.angle);
			float radius = random.get(ref.emission.radius * (1 - ref.emission.radiusThickness), ref.emission.radius);
			float localRadius = tanf(arc) * length;

			float x = cosf(arc) * radius * DirectX::XMVectorGetX(ref.emission.transform.scale);
			float y = sinf(arc) * radius * DirectX::XMVectorGetY(ref.emission.transform.scale);
			float z = 0;
			switch (ref.emission.emitFrom)
			{
			case EmitFrom::Volume:
				z = random.get(0, length);
				break;
			}

			emitPosition = DirectX::XMVectorAdd(
				DirectX::XMVector3Rotate(DirectX::XMVectorSet(x, y, z, 1), qAll),
				position
			);

			DirectX::XMVECTOR positionNormalized = DirectX::XMVectorSetZ(DirectX::XMVector3Normalize(emitPosition), cosf(angle));
			DirectX::XMVECTOR angles = DirectX::XMVectorSet(sinf(angle), sinf(angle), 1.f, 1.f);
			direction = DirectX::XMVector3Rotate(DirectX::XMVectorMultiply(positionNormalized, angles), qAll);
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

			float x = cosf(angle) * radius * DirectX::XMVectorGetX(ref.emission.transform.scale);
			float y = sinf(angle) * radius * DirectX::XMVectorGetY(ref.emission.transform.scale);
			emitPosition = DirectX::XMVector3Rotate(DirectX::XMVectorSet(x, y, 0, 1), qAll);
			
			direction = emitPosition;
			emitPosition = DirectX::XMVectorAdd(emitPosition, position);
		}
		else if (ref.emission.shape == EmissionShape::Sphere)
		{
			float angle = random.get(0, DirectX::XMConvertToRadians(ref.emission.arc));
			float angle2 = random.get(0, DirectX::XMConvertToRadians(180));
			float radius = random.get(ref.emission.radius * (1 - ref.emission.radiusThickness), ref.emission.radius);

			float x = cosf(angle) * sinf(angle2) * radius * DirectX::XMVectorGetX(ref.emission.transform.scale);
			float y = sinf(angle) * radius * DirectX::XMVectorGetY(ref.emission.transform.scale);
			float z = cosf(angle) * cosf(angle2) * radius * DirectX::XMVectorGetZ(ref.emission.transform.scale);
			emitPosition = DirectX::XMVector3Rotate(DirectX::XMVectorSet(x, y, z, 1), qAll);

			direction = emitPosition;
			emitPosition = DirectX::XMVectorAdd(emitPosition, position);
		}
		else if (ref.emission.shape == EmissionShape::HemiShpere)
		{
			float angle = random.get(0, DirectX::XMConvertToRadians(ref.emission.arc));
			float angle2 = random.get(0, PI / 2.f);
			float radius = random.get(ref.emission.radius * (1 - ref.emission.radiusThickness), ref.emission.radius);

			float x = cosf(angle) * sinf(angle2) * radius * DirectX::XMVectorGetX(ref.emission.transform.scale);
			float y = sinf(angle) * sinf(angle2) * radius * DirectX::XMVectorGetY(ref.emission.transform.scale);
			float z = cosf(angle2) * radius * DirectX::XMVectorGetZ(ref.emission.transform.scale);
			emitPosition = DirectX::XMVector3Rotate(DirectX::XMVectorSet(x, y, z, 1), qAll);

			direction = emitPosition;
			emitPosition = DirectX::XMVectorAdd(emitPosition, position);
		}

		direction = DirectX::XMVector3Normalize(direction);

		DirectX::XMFLOAT3 startRotation = ref.startRotation.evaluate(0, random.get(), 0);
		DirectX::XMFLOAT3 startSize = ref.startSize.evaluate(0, random.get(), 1);

		ParticleInstance& instance = particles[instanceIndex];
		instance.alive = true;
		instance.transform.position = emitPosition;
		instance.transform.rotation = DirectX::XMLoadFloat3(&startRotation);
		instance.transform.scale = DirectX::XMVectorMultiply(ref.transform.scale, DirectX::XMLoadFloat3(&startSize));
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
			DirectX::XMVECTOR qShift = quaternionFromZYX(shift.rotation);
			instance.transform.position = DirectX::XMVector3Rotate(shift.position, qShift);
			instance.transform.rotation = DirectX::XMVectorAdd(instance.transform.rotation, shift.rotation);
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

	static DirectX::XMFLOAT3& limitVelocity(DirectX::XMFLOAT3& velocity, DirectX::XMFLOAT3& limitVelocity, float damp, float time)
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

		DirectX::XMVECTOR qLocal = quaternionFromZYX(baseTransform.rotation);
		DirectX::XMVECTOR qShift = quaternionFromZYX(shift.rotation);
		DirectX::XMVECTOR rotation = DirectX::XMVectorAdd(
			DirectX::XMVectorAdd(baseTransform.rotation, ref.transform.rotation),
			shift.rotation
		);

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

			DirectX::XMVECTOR currentRotation = DirectX::XMVectorAdd(rotation, particle.transform.rotation);
			if (ref.rotationOverLifetime.enabled)
			{
				DirectX::XMFLOAT3 temp = ref.rotationOverLifetime.integrate(0, normalizedTime, particle.duration, particle.rotationOverLifetimeLerpRatio);
				currentRotation = DirectX::XMVectorAdd(currentRotation, DirectX::XMLoadFloat3(&temp));
			}

			// Apparently, the transform scale affects velocity too
			DirectX::XMVECTOR currentScale = particle.transform.scale;
			DirectX::XMVECTOR velocityScale = ref.transform.scale;
			if (ref.scalingMode == ScalingMode::Hierarchy)
			{
				currentScale = DirectX::XMVectorMultiply(currentScale, baseTransform.scale);
				velocityScale = DirectX::XMVectorMultiply(velocityScale, baseTransform.scale);
			}

			if (ref.sizeOverLifetime.enabled)
			{
				DirectX::XMFLOAT3 sol = ref.sizeOverLifetime.evaluate(normalizedTime, particle.sizeOverLifetimeLerpRatio, 1.f);
				currentScale = DirectX::XMVectorMultiply(currentScale, DirectX::XMLoadFloat3(&sol));
			}

			DirectX::XMVECTOR currentVelocity{};
			if (ref.velocityOverLifetime.enabled)
			{
				DirectX::XMFLOAT3 vol = ref.velocityOverLifetime.evaluate(normalizedTime, particle.velocityOverLifetimeLerpRatio);
				DirectX::XMVECTOR vol1 = DirectX::XMLoadFloat3(&vol);
				if (ref.velocitySpace == TransformSpace::Local)
				{
					vol1 = DirectX::XMVector3Rotate(vol1, qLocal);
				}
				currentVelocity = DirectX::XMVectorAdd(currentVelocity, vol1);
			}

			if (ref.forceOverLifetime.enabled)
			{
				DirectX::XMFLOAT3 fol = ref.forceOverLifetime.integrate(0, normalizedTime, particle.duration, particle.forceOverTimeLerpRatio);
				DirectX::XMVECTOR fol1 = DirectX::XMLoadFloat3(&fol);
				if (ref.forceSpace == TransformSpace::Local)
				{
					fol1 = DirectX::XMVector3Rotate(fol1, qLocal);
				}
				currentVelocity = DirectX::XMVectorAdd(currentVelocity, fol1);
			}

			currentVelocity = DirectX::XMVectorMultiplyAdd(particle.direction, DirectX::XMVectorReplicate(particle.speed), currentVelocity);

			float speedModifier = ref.speedModifier.evaluate(normalizedTime, particle.speedModifierLerpRatio, 1.f);
			float gravity = GRAVITY * ref.gravityModifier.evaluate(normalizedTime, particle.gravityModifierLerpRatio) * particle.time;
			DirectX::XMVECTOR gravityVector = DirectX::XMVectorSet(0, -gravity, 0, 0);

			currentVelocity = DirectX::XMVectorMultiplyAdd(currentVelocity, DirectX::XMVectorReplicate(speedModifier), gravityVector);
			
			DirectX::XMFLOAT3 velocity{};
			DirectX::XMStoreFloat3(&velocity, currentVelocity);

			DirectX::XMFLOAT3 velocityLimit = ref.limitVelocityOverLifetime.evaluate(normalizedTime, particle.limitVelocityLerpRatio);
			velocity = limitVelocity(velocity, velocityLimit, ref.limitVelocityDampen, particle.time);
			currentVelocity = DirectX::XMVectorSet(velocity.x, velocity.y, velocity.z, 1);

			particle.transform.position = DirectX::XMVectorMultiplyAdd(
				DirectX::XMVectorMultiply(currentVelocity, DirectX::XMVectorReplicate(dt)),
				velocityScale,
				particle.transform.position
			);

			float rotationFactor = particle.flipRotation ? -1 : 1;

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
				//directionMatrix *= rotateToDirection(particle, ref, velocity);
				currentRotation = DirectX::XMVectorSet(0, 0, 0, 0);
				//currentScale.y *= (velocityMagnitude * ref.speedScale) * 10.5f;
				//currentScale.x *= currentScale.y * ref.lengthScale * 5.5f;
				break;
			case RenderMode::HorizontalBillboard:
				currentRotation = DirectX::XMVectorSetX(currentRotation, 90.f * rotationFactor);
				currentRotation = DirectX::XMVectorSetY(currentRotation, 0);
				currentScale = DirectX::XMVectorMultiply(currentScale, billboardScale);
				break;
			case RenderMode::VerticalBillboard:
				// TODO: figure out the correct matrix for this
				directionMatrix = inverseView;
				currentScale = DirectX::XMVectorMultiply(currentScale, billboardScale);
				break;
			}

			if (particle.flipRotation)
				currentRotation = DirectX::XMVectorNegate(currentRotation);

			DirectX::XMVECTOR translation = particle.transform.position;
			if (ref.simulationSpace == TransformSpace::Local)
			{
				translation = DirectX::XMVectorAdd(DirectX::XMVector3Rotate(translation, qShift), shift.position);
				float zRot = DirectX::XMVectorGetZ(shift.rotation);
				if (abs(zRot) > 0.f)
					currentRotation = DirectX::XMVectorSetZ(currentRotation, -zRot);
			}

			// I do not like this but I can't think of any other decent way for now...
			if (ref.name == "aura")
				currentScale = DirectX::XMVectorMultiply(currentScale, shift.scale);

			particle.matrix *= DirectX::XMMatrixTranslationFromVector({ref.pivot.x, ref.pivot.y, ref.pivot.z, 1});
			particle.matrix *= DirectX::XMMatrixScalingFromVector(currentScale);
			particle.matrix *= directionMatrix;
			particle.matrix *= DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(-DirectX::XMVectorGetZ(currentRotation)));
			particle.matrix *= DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(DirectX::XMVectorGetY(currentRotation)));
			particle.matrix *= DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(DirectX::XMVectorGetX(currentRotation)));
			particle.matrix *= DirectX::XMMatrixTranslationFromVector(translation);
		}

		// Do we need to pass a copy here?
		Transform chilldTransform = baseTransform;
		chilldTransform.rotation = DirectX::XMVectorAdd(baseTransform.rotation, ref.transform.rotation);
		chilldTransform.position = DirectX::XMVectorAdd(baseTransform.position, ref.transform.position);
		chilldTransform.scale = DirectX::XMVectorMultiply(baseTransform.scale, ref.transform.scale);
		for (auto& em : children)
		{
			em.update(t, shift, chilldTransform, camera);
		}
	}
}