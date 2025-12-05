#include "Particle.h"
#include "DirectXMath.h"
#include "ResourceManager.h"
#include "Rendering/Camera.h"

namespace MikuMikuWorld::Effect
{
	constexpr float GRAVITY = 9.81f; // Based on default Physics3D
	constexpr float BILLBOARD_SCALE = 0.71f; // Estimated

	static std::knuth_b rand_engine;

	static DirectX::XMVECTOR quaternionFromZYX(const DirectX::XMVECTOR& euler)
	{
		float xRad = DirectX::XMConvertToRadians(DirectX::XMVectorGetX(euler));
		float yRad = DirectX::XMConvertToRadians(DirectX::XMVectorGetY(euler));
		float zRad = DirectX::XMConvertToRadians(DirectX::XMVectorGetZ(euler));

		DirectX::XMVECTOR qX = DirectX::XMQuaternionRotationNormal({ 1, 0, 0, 0 }, xRad);
		DirectX::XMVECTOR qY = DirectX::XMQuaternionRotationNormal({ 0, 1, 0, 0 }, yRad);
		DirectX::XMVECTOR qZ = DirectX::XMQuaternionRotationNormal({ 0, 0, 1, 0 }, zRad);

		return DirectX::XMQuaternionMultiply(DirectX::XMQuaternionMultiply(qZ, qY), qX);
	}

	static DirectX::XMMATRIX rotateToDirection(const ParticleInstance& p, const Particle& ref, const DirectX::XMVECTOR& velocity, const DirectX::XMVECTOR& scale)
	{
		static DirectX::XMVECTOR up = DirectX::XMVectorSet(0, 1, 0, 0);

		DirectX::XMVECTOR normalizedVelocity = DirectX::XMVector3Normalize(DirectX::XMVectorNegate(velocity));
		DirectX::XMVECTOR axis = DirectX::XMVector3Cross(normalizedVelocity, up);
		DirectX::XMVECTOR lengthVector = DirectX::XMVector3Length(axis);

		float angle{};
		DirectX::XMStoreFloat(&angle, lengthVector);

		float t{};
		if (angle >= 0.000001f)
		{
			angle = asinf(std::min(angle, 1.f));
		}
		else
		{
			angle = 0.f;
			axis = DirectX::XMVectorSet(1, 0, 0, 0);
			DirectX::XMStoreFloat(&t, DirectX::XMVector3Length(axis));

			if (t < 0.000001f)
				axis = DirectX::XMVectorSet(-1.f, 0, 0, 0);
		}

		DirectX::XMStoreFloat(&t, DirectX::XMVector3Dot(DirectX::XMVectorNegate(normalizedVelocity), up));
		if (t < 0.0f)
			angle = PI - angle;

		DirectX::XMVECTOR lengthSquaredVector = DirectX::XMVector3LengthSq(velocity);

		float lengthSquared = DirectX::XMVectorGetX(lengthSquaredVector);
		float length = lengthSquared > 0.000001 ? sqrtf(lengthSquared) : 0.0f;
		float yScale = (ref.speedScale * length) + (ref.lengthScale * DirectX::XMVectorGetX(scale));
		DirectX::XMVECTOR stretchDirection = DirectX::XMVectorScale(normalizedVelocity, yScale * 0.5f);

		DirectX::XMMATRIX result = DirectX::XMMatrixIdentity();
		result *= DirectX::XMMatrixScalingFromVector(DirectX::XMVectorSet(1.f, yScale, 1.f, 1.f));
		result *= DirectX::XMMatrixRotationAxis(axis, angle);
		result *= DirectX::XMMatrixTranslationFromVector(stretchDirection);
		return result;
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

		// NOTE: std::min(count, ref.maxParticles) is enough
		// + 1 to fix a stupid emission bug
		return std::min(count, ref.maxParticles) + 1;
	}

	void EmitterInstance::init(const Particle& ref, const Transform& transform)
	{
		refID = ref.ID;
		baseTransform = transform;

		bursts.reserve(ref.emission.bursts.size());
		for (const auto& refBurst : ref.emission.bursts)
			bursts.push_back({});

		Transform childTransform;
		childTransform.position = DirectX::XMVectorAdd(transform.position, ref.transform.position);
		childTransform.rotation = DirectX::XMVectorAdd(transform.rotation, ref.transform.rotation);
		childTransform.scale = DirectX::XMVectorMultiply(transform.scale, ref.transform.scale);

		for (int i = 0; i < children.size(); i++)
		{
			const Particle& childRef = ResourceManager::getParticleEffect(ref.children[i]);
			children[i].init(childRef, childTransform);
		}
	}

	void EmitterInstance::emit(const Transform& shift, const Particle& ref, float time)
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

		ParticleInstance& instance = particles[instanceIndex];

		// TODO: figure out the correct order for these
		float initialR = initialRandom.nextFloat();
		float shapeRandomX = shapeRandom.nextFloat();
		float shapeRandomY = shapeRandom.nextFloat();
		float shapeRandomZ = shapeRandom.nextFloat();
		float shapeRandomW = shapeRandom.nextFloat();

		float sizeR = sizeRandom.nextFloat();
		float rotationRandom = initialRandom.nextFloat();
		float colorRandom = initialRandom.nextFloat();
		float velocityR = velocityRandom.nextFloat();

		float length = ref.startSpeed.evaluate(initialR);
		DirectX::XMFLOAT3 startRotation = ref.startRotation.evaluate(0, initialR, 0);
		
		instance.duration = ref.startLifeTime.evaluate(initialR);
		instance.textureStartFrameLerpRatio = initialR;
		instance.gravityModifierLerpRatio = initialR;
		instance.textureFrameOverTimeLerpRatio = initialR;

		instance.startColor = ref.startColor.evaluate(colorRandom);
		instance.colorOverLifeTimeLerpRatio = colorRandom;

		DirectX::XMVECTOR emitPosition = DirectX::XMVectorSet(0, 0, 0, 1);
		DirectX::XMVECTOR direction = DirectX::XMVectorSet(0, 0, 0, 0);

		if (ref.emission.shape == EmissionShape::Box)
		{
			DirectX::XMVECTOR halfScale = DirectX::XMVectorScale(ref.emission.transform.scale, .5f);
			DirectX::XMFLOAT3 halves{};
			DirectX::XMStoreFloat3(&halves, halfScale);
			float x = lerp(-halves.x, halves.x, shapeRandomX);
			float y = lerp(-halves.y, halves.y, shapeRandomY);
			float z = lerp(-halves.z, halves.z, shapeRandomZ);
			emitPosition = DirectX::XMVectorSet(x, y, z, 1);

			emitPosition = DirectX::XMVectorAdd(DirectX::XMVector3Rotate(emitPosition, qAll), position);
			direction = DirectX::XMVector3Rotate({ 0, 0, 1, 0 }, qBase);
		}
		else if (ref.emission.shape == EmissionShape::Cone)
		{
			float arc{};
			switch (ref.emission.arcMode)
			{
			case ArcMode::Loop:
				arc = DirectX::XMConvertToRadians(emissionPosition);
				emissionPosition += fmodf(ref.emission.arcSpeed.evaluate(std::max(time - startTime, 0.f), 0) * (time - startTime) * 360.f, ref.emission.arc);
				break;
			case ArcMode::BurstSpread:
				arc = DirectX::XMConvertToRadians(emissionPosition);
				emissionPosition += emissionPositionInterval;
				break;
			default:
				arc = lerp(0, DirectX::XMConvertToRadians(ref.emission.arc), shapeRandomX);
			}

			float angle = DirectX::XMConvertToRadians(ref.emission.angle);
			float radius =lerp(ref.emission.radius * (1 - ref.emission.radiusThickness), ref.emission.radius, shapeRandomY);
			float localRadius = tanf(angle) * length;

			float x = cosf(arc) * radius * DirectX::XMVectorGetX(ref.emission.transform.scale);
			float y = sinf(arc) * radius * DirectX::XMVectorGetY(ref.emission.transform.scale);
			float z = 0;
			switch (ref.emission.emitFrom)
			{
			case EmitFrom::Volume:
				z = lerp(0, length, shapeRandomZ);
				break;
			}

			emitPosition = DirectX::XMVectorSet(x, y, z, 1);
			DirectX::XMVECTOR positionNormalized = DirectX::XMVectorSetZ(DirectX::XMVector3Normalize(emitPosition), cosf(angle));
			DirectX::XMVECTOR angles = DirectX::XMVectorSet(sinf(angle), sinf(angle), 1.f, 1.f);
			direction = DirectX::XMVector3Rotate(DirectX::XMVectorMultiply(positionNormalized, angles), qAll);

			emitPosition = DirectX::XMVectorAdd(
				DirectX::XMVector3Rotate(emitPosition, qAll),
				position
			);
		}
		else if (ref.emission.shape == EmissionShape::Circle)
		{
			float angle{};
			switch (ref.emission.arcMode)
			{
			case ArcMode::Loop:
				angle = DirectX::XMConvertToRadians(emissionPosition);
				emissionPosition += fmodf(ref.emission.arcSpeed.evaluate(std::max(time - startTime, 0.f), 0) * (time - startTime) * 360.f, ref.emission.arc);
				break;
			case ArcMode::BurstSpread:
				angle = DirectX::XMConvertToRadians(emissionPosition);
				emissionPosition += emissionPositionInterval;
				break;
			default:
				angle = lerp(0, DirectX::XMConvertToRadians(ref.emission.arc), shapeRandomX);
			}

			float radius = lerp(ref.emission.radius * (1 - ref.emission.radiusThickness), ref.emission.radius, shapeRandomY);

			float x = cosf(angle) * radius * DirectX::XMVectorGetX(ref.emission.transform.scale);
			float y = sinf(angle) * radius * DirectX::XMVectorGetY(ref.emission.transform.scale);
			emitPosition = DirectX::XMVector3Rotate(DirectX::XMVectorSet(x, y, 0, 1), qAll);
			
			direction = emitPosition;
			emitPosition = DirectX::XMVectorAdd(emitPosition, position);
		}
		else if (ref.emission.shape == EmissionShape::Sphere)
		{
			float angle = lerp(0, DirectX::XMConvertToRadians(ref.emission.arc), shapeRandomX);
			float angle2 = lerp(0, DirectX::XMConvertToRadians(180), shapeRandomY);
			float radius = lerp(ref.emission.radius * (1 - ref.emission.radiusThickness), ref.emission.radius, shapeRandomZ);

			float x = cosf(angle) * sinf(angle2) * radius * DirectX::XMVectorGetX(ref.emission.transform.scale);
			float y = sinf(angle) * radius * DirectX::XMVectorGetY(ref.emission.transform.scale);
			float z = cosf(angle) * cosf(angle2) * radius * DirectX::XMVectorGetZ(ref.emission.transform.scale);
			emitPosition = DirectX::XMVector3Rotate(DirectX::XMVectorSet(x, y, z, 1), qAll);

			direction = emitPosition;
			emitPosition = DirectX::XMVectorAdd(emitPosition, position);
		}
		else if (ref.emission.shape == EmissionShape::HemiShpere)
		{
			float angle = lerp(0, DirectX::XMConvertToRadians(ref.emission.arc), shapeRandomX);
			float angle2 = lerp(0, PI / 2.f, shapeRandomY);
			float radius = lerp(ref.emission.radius * (1 - ref.emission.radiusThickness), ref.emission.radius, shapeRandomZ);

			float x = cosf(angle) * sinf(angle2) * radius * DirectX::XMVectorGetX(ref.emission.transform.scale);
			float y = sinf(angle) * sinf(angle2) * radius * DirectX::XMVectorGetY(ref.emission.transform.scale);
			float z = cosf(angle2) * radius * DirectX::XMVectorGetZ(ref.emission.transform.scale);
			emitPosition = DirectX::XMVector3Rotate(DirectX::XMVectorSet(x, y, z, 1), qAll);

			direction = emitPosition;
			emitPosition = DirectX::XMVectorAdd(emitPosition, position);
		}

		direction = DirectX::XMVector3Normalize(direction);

		instance.alive = true;
		instance.transform.position = emitPosition;

		instance.transform.rotation = DirectX::XMLoadFloat3(&startRotation);
		instance.direction = direction;
		instance.speed = length;
		instance.startTime = time;
		instance.time = 0;
		
		instance.velocityOverLifetimeLerpRatio = velocityR;
		instance.speedModifierLerpRatio = velocityR;
		instance.limitVelocityLerpRatio = velocityR;
		instance.forceOverTimeLerpRatio = velocityR;
		instance.rotationOverLifetimeLerpRatio = rotationRandom;

		DirectX::XMFLOAT3 startSize = ref.startSize.evaluate(0, sizeR, 1);
		instance.transform.scale = DirectX::XMVectorMultiply(ref.transform.scale, DirectX::XMLoadFloat3(&startSize));
		instance.sizeOverLifetimeLerpRatio = sizeR;

		// shift will be included during emission for world space
		// for local space, the shift will be included during particle update 
		if (ref.simulationSpace == TransformSpace::World)
		{
			DirectX::XMVECTOR qShift = quaternionFromZYX(shift.rotation);
			instance.transform.position = DirectX::XMVectorAdd(instance.transform.position, DirectX::XMVector3Rotate(shift.position, qShift));
			instance.transform.rotation = DirectX::XMVectorAdd(instance.transform.rotation, shift.rotation);
			instance.direction = DirectX::XMVector3Rotate(instance.direction, qShift);
		}

		std::bernoulli_distribution flipRotationRoll(ref.flipRotation);
		instance.flipRotation = flipRotationRoll(rand_engine);
		aliveCount++;
	}

	void EmitterInstance::updateEmission(const Particle& ref, const Transform& shift, float time)
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
				emissionAccumulator++;
				lastEmissionTime = time - startTime;
				emit(shift, ref, nextEmissionTime);
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
						emit(shift, ref, time);
				}
			}
		}
	}

	void EmitterInstance::start(float time)
	{
		const Particle& ref = ResourceManager::getParticleEffect(refID);
		seed = ref.useAutoRandomSeed ? globalRandom.get() * UINT_FAST32_MAX : ref.randomSeed;
		initialRandom.setSeed(seed);
		shapeRandom.setSeed(seed);
		velocityRandom.setSeed(seed);
		sizeRandom.setSeed(seed);

		float emissionRandom = initialRandom.nextFloat();
		float arcSpeedRandom = shapeRandom.nextFloat();
		startTime = time + ref.startDelay.evaluate(0, emissionRandom);
		rateOverTime = ref.emission.rateOverTime.evaluate(0, emissionRandom);
		arcSpeed = arcSpeedRandom;

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
		aliveCount = 0;
		
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

	void EmitterInstance::update(float t, const Transform& shift, const Camera& camera)
	{
		const Particle& ref = ResourceManager::getParticleEffect(refID);

		const DirectX::XMMATRIX& inverseView = camera.getInverseViewMatrix();
		DirectX::XMVECTOR qLocal = quaternionFromZYX(baseTransform.rotation);
		DirectX::XMVECTOR qShift = quaternionFromZYX(shift.rotation);
		DirectX::XMVECTOR rotation = DirectX::XMVectorAdd(baseTransform.rotation, ref.transform.rotation);

		DirectX::XMMATRIX worldOffset = DirectX::XMMatrixIdentity();
		worldOffset *= DirectX::XMMatrixRotationQuaternion(qShift);
		worldOffset *= DirectX::XMMatrixTranslationFromVector(shift.position);

		updateEmission(ref, shift, t);

		for (auto& p : particles)
		{
			if (!p.alive)
				continue;

			float dt = t - p.startTime - p.time;
			p.time = t - p.startTime;

			if (p.time >= p.duration)
			{
				p.alive = false;
				aliveCount = std::max(aliveCount - 1, 0);
				continue;
			}

			float normalizedTime = p.time / p.duration;

			DirectX::XMVECTOR currentRotation = DirectX::XMVectorAdd(rotation, p.transform.rotation);
			if (ref.rotationOverLifetime.enabled)
			{
				DirectX::XMFLOAT3 temp = ref.rotationOverLifetime.integrate(0, normalizedTime, p.duration, p.rotationOverLifetimeLerpRatio);
				DirectX::XMVECTOR tempV = DirectX::XMLoadFloat3(&temp);
				currentRotation = DirectX::XMVectorAdd(currentRotation, tempV);
			}

			// Apparently, the transform scale affects velocity too
			// Need to make sure hierarchy scale affects velocity too
			DirectX::XMVECTOR currentScale = p.transform.scale;
			DirectX::XMVECTOR velocityScale = ref.transform.scale;
			if (ref.scalingMode == ScalingMode::Hierarchy)
			{
				currentScale = DirectX::XMVectorMultiply(currentScale, baseTransform.scale);
				velocityScale = DirectX::XMVectorMultiply(velocityScale, baseTransform.scale);
			}

			if (ref.sizeOverLifetime.enabled)
			{
				DirectX::XMFLOAT3 sol = ref.sizeOverLifetime.evaluate(normalizedTime, p.sizeOverLifetimeLerpRatio, 1.f);
				currentScale = DirectX::XMVectorMultiply(currentScale, DirectX::XMLoadFloat3(&sol));
			}

			DirectX::XMVECTOR currentVelocity{};
			if (ref.velocityOverLifetime.enabled)
			{
				DirectX::XMFLOAT3 vol = ref.velocityOverLifetime.evaluate(normalizedTime, p.velocityOverLifetimeLerpRatio);
				DirectX::XMVECTOR vol1 = DirectX::XMLoadFloat3(&vol);
				if (ref.velocitySpace == TransformSpace::Local)
				{
					vol1 = DirectX::XMVector3Rotate(vol1, qLocal);
				}
				currentVelocity = DirectX::XMVectorAdd(currentVelocity, vol1);
			}

			if (ref.forceOverLifetime.enabled)
			{
				DirectX::XMFLOAT3 fol = ref.forceOverLifetime.integrate(0, normalizedTime, p.duration, p.forceOverTimeLerpRatio);
				DirectX::XMVECTOR fol1 = DirectX::XMLoadFloat3(&fol);
				if (ref.forceSpace == TransformSpace::Local)
				{
					fol1 = DirectX::XMVector3Rotate(fol1, qLocal);
				}
				currentVelocity = DirectX::XMVectorAdd(currentVelocity, fol1);
			}

			currentVelocity = DirectX::XMVectorMultiplyAdd(p.direction, DirectX::XMVectorReplicate(p.speed), currentVelocity);

			float speedModifier = ref.speedModifier.evaluate(normalizedTime, p.speedModifierLerpRatio, 1.f);
			float gravity = GRAVITY * ref.gravityModifier.evaluate(normalizedTime, p.gravityModifierLerpRatio) * p.time;
			DirectX::XMVECTOR gravityVector = DirectX::XMVectorSet(0, -gravity, 0, 0);
			
			currentVelocity = DirectX::XMVectorMultiplyAdd(currentVelocity, DirectX::XMVectorReplicate(speedModifier), gravityVector);
			DirectX::XMFLOAT3 velocity{};
			DirectX::XMStoreFloat3(&velocity, currentVelocity);

			DirectX::XMFLOAT3 velocityLimit = ref.limitVelocityOverLifetime.evaluate(normalizedTime, p.limitVelocityLerpRatio);
			velocity = limitVelocity(velocity, velocityLimit, ref.limitVelocityDampen, p.time);
			currentVelocity = DirectX::XMLoadFloat3(&velocity);

			p.transform.position = DirectX::XMVectorMultiplyAdd(
				DirectX::XMVectorMultiply(currentVelocity, DirectX::XMVectorReplicate(dt)),
				velocityScale,
				p.transform.position
			);

			float rotationFactor = p.flipRotation ? -1 : 1;

			p.matrix = DirectX::XMMatrixIdentity();
			DirectX::XMMATRIX directionMatrix = DirectX::XMMatrixIdentity();
			switch (ref.renderMode)
			{
			case RenderMode::Billboard:
				switch (ref.alignment)
				{
				case AlignmentMode::View:
					directionMatrix = inverseView;
					currentRotation = p.transform.rotation;
					break;
				}
				break;
			case RenderMode::StretchedBillboard:
				directionMatrix = rotateToDirection(p, ref, currentVelocity, currentScale);
				currentRotation = DirectX::XMVectorSubtract(currentRotation, p.transform.rotation);
				currentScale = DirectX::XMVectorSetY(currentScale, 1.f);
				break;
			case RenderMode::HorizontalBillboard:
				currentRotation = DirectX::XMVectorSetX(currentRotation, 90.f * rotationFactor);
				currentRotation = DirectX::XMVectorSetY(currentRotation, 0);
				currentScale = DirectX::XMVectorScale(currentScale, BILLBOARD_SCALE);
				break;
			case RenderMode::VerticalBillboard:
				// TODO: figure out the correct matrix for this
				directionMatrix = inverseView;
				currentScale = DirectX::XMVectorScale(currentScale, BILLBOARD_SCALE);
				break;
			}

			if (p.flipRotation)
				currentRotation = DirectX::XMVectorNegate(currentRotation);

			// I do not like this but I can't think of any other decent way for now...
			if (ref.name == "aura")
				currentScale = DirectX::XMVectorMultiply(currentScale, shift.scale);

			p.matrix *= DirectX::XMMatrixTranslation(ref.pivot.x, ref.pivot.y, ref.pivot.z);
			p.matrix *= DirectX::XMMatrixScalingFromVector(currentScale);
			
			DirectX::XMVECTOR eulerFinal = DirectX::XMVectorSetZ(currentRotation, -DirectX::XMVectorGetZ(currentRotation));
			DirectX::XMMATRIX m4Rotation = DirectX::XMMatrixRotationQuaternion(quaternionFromZYX(eulerFinal));
			
			if (ref.renderMode == RenderMode::StretchedBillboard)
			{
				directionMatrix *= DirectX::XMMatrixInverse(nullptr, m4Rotation);
			}

			p.matrix *= directionMatrix;
			p.matrix *= m4Rotation;
			p.matrix *= DirectX::XMMatrixTranslationFromVector(p.transform.position);
			
			if (ref.simulationSpace == TransformSpace::Local)
			{
				p.matrix *= worldOffset;
			}
		}

		for (auto& em : children)
		{
			em.update(t, shift, camera);
		}
	}
}