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

	static DirectX::XMMATRIX rotateToDirection(const Particle& ref, const DirectX::XMVECTOR& velocity, const DirectX::XMVECTOR& scale)
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
			angle = asinf(angle);
		}
		else
		{
			angle = 0.f;
			axis = DirectX::XMVectorSet(0, 1, 0, 0);
			DirectX::XMStoreFloat(&t, DirectX::XMVector3Length(axis));

			if (t < 0.000001f)
				axis = DirectX::XMVectorSet(0, -1.f, 0, 0);
		}

		DirectX::XMStoreFloat(&t, DirectX::XMVector3Dot(DirectX::XMVectorNegate(normalizedVelocity), up));
		if (t < 0.0f)
			angle = NUM_PI - angle;

		DirectX::XMVECTOR lengthSquaredVector = DirectX::XMVector3LengthSq(velocity);

		float lengthSquared = DirectX::XMVectorGetX(lengthSquaredVector);
		float length = lengthSquared > 0.000001f ? sqrtf(lengthSquared) : 0.0f;
		float yScale = (ref.speedScale * length) + (ref.lengthScale * DirectX::XMVectorGetX(scale));
		DirectX::XMVECTOR stretchDirection = DirectX::XMVectorScale(normalizedVelocity, yScale * 0.5f);

		DirectX::XMMATRIX result = DirectX::XMMatrixIdentity();
		result *= DirectX::XMMatrixScalingFromVector(DirectX::XMVectorSet(1.f, yScale, 1.f, 1.f));
		result *= DirectX::XMMatrixRotationAxis(axis, angle);
		result *= DirectX::XMMatrixTranslationFromVector(stretchDirection);
		return result;
	}

	int EmitterInstance::findFirstDeadParticle(float time) const
	{
		return aliveCount;
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

	void EmitterInstance::emit(const Transform& worldTransform, const Particle& ref, float time, int count)
	{
		if (aliveCount >= particles.size())
			return;

		DirectX::XMVECTOR qBase = quaternionFromZYX(baseTransform.rotation);

		DirectX::XMVECTOR baseAndRefRotation = DirectX::XMVectorAdd(baseTransform.rotation, ref.transform.rotation);
		DirectX::XMVECTOR qBaseRef = quaternionFromZYX(baseAndRefRotation);

		DirectX::XMVECTOR qAll = quaternionFromZYX(DirectX::XMVectorAdd(baseAndRefRotation, ref.emission.transform.rotation));

		DirectX::XMVECTOR transformPos = DirectX::XMVector3Rotate(DirectX::XMVectorMultiply(ref.transform.position, baseTransform.scale), qBase);
		DirectX::XMVECTOR basePos = DirectX::XMVector3Rotate(DirectX::XMVectorMultiply(baseTransform.position, baseTransform.scale), qBase);
		DirectX::XMVECTOR shapePos = DirectX::XMVector3Rotate(ref.emission.transform.position, qBaseRef);

		DirectX::XMVECTOR position = DirectX::XMVectorAdd(DirectX::XMVectorAdd(transformPos, basePos), shapePos);

		std::array<float, 4> shapeRandomSetX{};
		std::array<float, 4> shapeRandomSetY{};
		std::array<float, 4> shapeRandomSetZ{};

		std::array<float, 4> velocityRandomSet{};
		std::array<float, 4> sizeRandomSet{};
		
		std::array<float, 4> initialA{};
		std::array<float, 4> initialB{};
		std::array<float, 4> initialC{};
		std::array<float, 4> initialD{};
		std::array<float, 4> initialE{};
		std::array<float, 4> initialCy{};
		std::array<float, 4> initialEy{};
		std::array<float, 4> initialEz{};

		for (int i = 0; i < count; i++)
		{
			int instanceIndex = findFirstDeadParticle(time);
			if (!isArrayIndexInBounds(instanceIndex, particles))
				return;

			if (i % 4 == 0)
			{
				initialA = initialRandom.nextFloat();
				initialB = initialRandom.nextFloat();

				initialC = initialRandom.nextFloat();
				if (ref.startSize.is3D)
				{
					initialCy = initialRandom.nextFloat();
				}

				initialD = initialRandom.nextFloat();
				
				initialE = initialRandom.nextFloat();
				if (ref.startRotation.is3D)
				{
					initialEy = initialRandom.nextFloat();
					initialEz = initialRandom.nextFloat();
				}

				velocityRandomSet = velocityRandom.nextFloat();
				sizeRandomSet = sizeRandom.nextFloat();
			}

			int randomSetIndex = i % 4;

			float a = initialA[randomSetIndex];
			float b = initialB[randomSetIndex];
			float c = initialC[randomSetIndex];
			float cy = initialCy[randomSetIndex];
			float d = initialD[randomSetIndex];
			float e = initialE[randomSetIndex];
			float ey = initialEy[randomSetIndex];
			float ez = initialEz[randomSetIndex];

			float length = ref.startSpeed.evaluate(b);
			DirectX::XMFLOAT3 startRotation = ref.startRotation.is3D ?
				ref.startRotation.evaluate(0, { e, ey, ez }, 0) : ref.startRotation.evaluate(0, e, 0);

			DirectX::XMVECTOR emitPosition = DirectX::XMVectorSet(0, 0, 0, 1);
			DirectX::XMVECTOR direction = DirectX::XMVectorSet(0, 0, 0, 0);

			if (ref.emission.shape == EmissionShape::Box)
			{
				if (randomSetIndex == 0)
				{
					shapeRandomSetX = shapeRandom4.nextFloat();
					shapeRandomSetY = shapeRandom4.nextFloat();
					shapeRandomSetZ = shapeRandom4.nextFloat();
				}

				DirectX::XMVECTOR halfScale = DirectX::XMVectorScale(ref.emission.transform.scale, .5f);
				DirectX::XMFLOAT3 halves{};
				DirectX::XMStoreFloat3(&halves, halfScale);

				float x = lerp(-halves.x, halves.x, shapeRandomSetX[randomSetIndex]);
				float y = lerp(-halves.y, halves.y, shapeRandomSetY[randomSetIndex]);
				float z = lerp(-halves.z, halves.z, shapeRandomSetZ[randomSetIndex]);

				emitPosition = DirectX::XMVectorSet(x, y, z, 1);
				emitPosition = DirectX::XMVectorAdd(DirectX::XMVector3Rotate(emitPosition, qAll), position);
				direction = DirectX::XMVector3Rotate({ 0, 0, 1, 0 }, qBaseRef);
			}
			else if (ref.emission.shape == EmissionShape::Cone)
			{
				if (randomSetIndex == 0)
				{
					if (ref.emission.arcMode != ArcMode::BurstSpread)
						shapeRandomSetX = shapeRandom4.nextFloat();

					shapeRandomSetY = shapeRandom4.nextFloat();
					shapeRandomSetZ = shapeRandom4.nextFloat();
				}

				float arc{};
				switch (ref.emission.arcMode)
				{
				case ArcMode::Loop:
					arc = DirectX::XMConvertToRadians(emissionPosition);
					emissionPosition += ref.emission.arcSpeed.evaluate(std::max(time - startTime, 0.f), shapeRandomSetX[randomSetIndex]) * (time - startTime) * 360.f;
					emissionPosition = fmodf(emissionPosition, ref.emission.arc);
					break;
				case ArcMode::BurstSpread:
					arc = DirectX::XMConvertToRadians(emissionPosition);
					emissionPosition += emissionPositionInterval;
					break;
				default:
					arc = lerp(0, DirectX::XMConvertToRadians(ref.emission.arc), shapeRandomSetX[randomSetIndex]);
				}

				float angle = DirectX::XMConvertToRadians(ref.emission.angle);
				float radius = lerp(ref.emission.radius * (1 - ref.emission.radiusThickness), ref.emission.radius, shapeRandomSetY[randomSetIndex]);
				float localRadius = tanf(angle) * length;

				float x = cosf(arc) * radius * DirectX::XMVectorGetX(ref.emission.transform.scale);
				float y = sinf(arc) * radius * DirectX::XMVectorGetY(ref.emission.transform.scale);
				float z = ref.emission.emitFrom == EmitFrom::Volume ? lerp(0, length, shapeRandomSetZ[randomSetIndex]) : 0;

				float xRandom = lerp(-ref.emission.randomizeDirection, ref.emission.randomizeDirection, globalRandom.get());
				float yRandom = lerp(-ref.emission.randomizeDirection, ref.emission.randomizeDirection, globalRandom.get());
				float zRandom = lerp(-ref.emission.randomizeDirection, ref.emission.randomizeDirection, globalRandom.get());

				emitPosition = DirectX::XMVectorAdd(DirectX::XMVectorSet(x, y, z, 0), DirectX::XMVectorSet(xRandom, yRandom, zRandom, 0));
				DirectX::XMVECTOR positionNormalized = DirectX::XMVectorSetZ(DirectX::XMVector3Normalize(emitPosition), cosf(angle));
				DirectX::XMVECTOR angles = DirectX::XMVectorSet(sinf(angle), sinf(angle), 1.f, 1.f);
				direction = DirectX::XMVector3Rotate(DirectX::XMVectorMultiply(positionNormalized, angles), qAll);

				emitPosition = DirectX::XMVectorAdd(DirectX::XMVector3Rotate(emitPosition, qAll), position);
			}
			else if (ref.emission.shape == EmissionShape::Circle)
			{
				if (randomSetIndex == 0)
				{
					if (ref.emission.arcMode != ArcMode::BurstSpread)
						shapeRandomSetX = shapeRandom4.nextFloat();

					shapeRandomSetY = shapeRandom4.nextFloat();
				}

				float angle{};
				switch (ref.emission.arcMode)
				{
				case ArcMode::Loop:
					angle = DirectX::XMConvertToRadians(emissionPosition);
					emissionPosition += fmodf(ref.emission.arcSpeed.evaluate(std::max(time - startTime, 0.f), shapeRandomSetX[randomSetIndex]) * (time - startTime) * 360.f, ref.emission.arc);
					break;
				case ArcMode::BurstSpread:
					angle = DirectX::XMConvertToRadians(emissionPosition);
					emissionPosition += emissionPositionInterval;
					break;
				default:
					angle = lerp(0, DirectX::XMConvertToRadians(ref.emission.arc), shapeRandomSetX[randomSetIndex]);
				}

				float radius = lerp(ref.emission.radius * (1 - ref.emission.radiusThickness), ref.emission.radius, shapeRandomSetY[randomSetIndex]);

				float x = cosf(angle) * radius * DirectX::XMVectorGetX(ref.emission.transform.scale);
				float y = sinf(angle) * radius * DirectX::XMVectorGetY(ref.emission.transform.scale);
				emitPosition = DirectX::XMVector3Rotate(DirectX::XMVectorSet(x, y, 0, 1), qAll);

				direction = emitPosition;
				emitPosition = DirectX::XMVectorAdd(emitPosition, position);
			}
			else if (ref.emission.shape == EmissionShape::Sphere)
			{
				if (randomSetIndex == 0)
				{
					shapeRandomSetX = shapeRandom4.nextFloat();
					shapeRandomSetY = shapeRandom4.nextFloat();
					shapeRandomSetZ = shapeRandom4.nextFloat();
				}

				float angle = lerp(0, DirectX::XMConvertToRadians(ref.emission.arc), shapeRandomSetX[randomSetIndex]);
				float angle2 = lerp(0, DirectX::XMConvertToRadians(180), shapeRandomSetY[randomSetIndex]);
				float radius = lerp(ref.emission.radius * (1 - ref.emission.radiusThickness), ref.emission.radius, shapeRandomSetZ[randomSetIndex]);

				float x = cosf(angle) * sinf(angle2) * radius * DirectX::XMVectorGetX(ref.emission.transform.scale);
				float y = sinf(angle) * radius * DirectX::XMVectorGetY(ref.emission.transform.scale);
				float z = cosf(angle) * cosf(angle2) * radius * DirectX::XMVectorGetZ(ref.emission.transform.scale);
				emitPosition = DirectX::XMVector3Rotate(DirectX::XMVectorSet(x, y, z, 1), qAll);

				direction = emitPosition;
				emitPosition = DirectX::XMVectorAdd(emitPosition, position);
			}
			else if (ref.emission.shape == EmissionShape::HemiShpere)
			{
				if (randomSetIndex == 0)
				{
					shapeRandomSetX = shapeRandom4.nextFloat();
					shapeRandomSetY = shapeRandom4.nextFloat();
					shapeRandomSetZ = shapeRandom4.nextFloat();
				}

				float angle = lerp(0, DirectX::XMConvertToRadians(ref.emission.arc), shapeRandomSetX[randomSetIndex]);
				float angle2 = lerp(0, NUM_PI_2, shapeRandomSetY[randomSetIndex]);
				float radius = lerp(ref.emission.radius * (1 - ref.emission.radiusThickness), ref.emission.radius, shapeRandomSetZ[randomSetIndex]);

				float x = cosf(angle) * sinf(angle2) * radius * DirectX::XMVectorGetX(ref.emission.transform.scale);
				float y = sinf(angle) * sinf(angle2) * radius * DirectX::XMVectorGetY(ref.emission.transform.scale);
				float z = cosf(angle2) * radius * DirectX::XMVectorGetZ(ref.emission.transform.scale);
				emitPosition = DirectX::XMVector3Rotate(DirectX::XMVectorSet(x, y, z, 1), qAll);

				direction = emitPosition;
				emitPosition = DirectX::XMVectorAdd(emitPosition, position);
			}

			direction = DirectX::XMVector3Normalize(direction);

			ParticleInstance& instance = particles[instanceIndex];
			instance.alive = true;
			instance.transform.position = emitPosition;
			if (ref.renderMode == RenderMode::Billboard && ref.alignment == AlignmentMode::View)
				instance.transform.rotation = DirectX::XMLoadFloat3(&startRotation);
			else
				instance.transform.rotation = DirectX::XMVectorNegate(DirectX::XMLoadFloat3(&startRotation));

			instance.duration = ref.startLifeTime.evaluate(a);
			instance.spriteSheetLerpRatio = a;
			instance.gravityLerpRatio = d;

			instance.startColor = ref.startColor.evaluate(d);
			instance.colorLerpRatio = d;

			instance.direction = direction;
			instance.speed = length;
			instance.startTime = time;
			instance.time = 0;
			instance.accumulateVelocityLimit = DirectX::XMVectorSet(0, 0, 0, 0);

			float velocityR = velocityRandomSet[randomSetIndex];
			instance.velocityLerpRatio = velocityR;
			instance.limitVelocityLerpRatio = velocityR;
			instance.forceLerpRatio = velocityR;

			instance.rotationLerpRatio = e;
			instance.sizeLerpRatio = sizeRandomSet[randomSetIndex];

			DirectX::XMFLOAT3 startSize = ref.startSize.is3D ?
				ref.startSize.evaluate(0, { c, cy, 1 }, 1) : ref.startSize.evaluate(0, c, 1);
			instance.transform.scale = DirectX::XMVectorMultiply(ref.transform.scale, DirectX::XMLoadFloat3(&startSize));

			// world transform will be included during emission for world space
			// for local space, the world transform will be included during particle update 
			if (ref.simulationSpace == TransformSpace::World)
			{
				DirectX::XMVECTOR qShift = quaternionFromZYX(worldTransform.rotation);
				DirectX::XMMATRIX worldOffset = DirectX::XMMatrixIdentity();
				worldOffset *= DirectX::XMMatrixRotationQuaternion(qShift);
				worldOffset *= DirectX::XMMatrixTranslationFromVector(worldTransform.position);

				instance.transform.position = DirectX::XMVector3Transform(instance.transform.position, worldOffset);
				instance.transform.rotation = DirectX::XMVectorAdd(instance.transform.rotation, worldTransform.rotation);
				instance.direction = DirectX::XMVector3Rotate(instance.direction, qShift);
			}

			std::bernoulli_distribution flipRotationRoll(ref.flipRotation);
			instance.flipRotation = flipRotationRoll(rand_engine);
			aliveCount++;
		}
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
				emit(shift, ref, nextEmissionTime, 1);
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

					emit(shift, ref, time, refBurst.count);
				}
			}
		}
	}

	void EmitterInstance::start(float time)
	{
		const Particle& ref = ResourceManager::getParticleEffect(refID);

		uint32_t seed = ref.randomSeed;
		if (ref.useAutoRandomSeed)
			seed = globalRandom.get() * std::numeric_limits<uint32_t>::max();

		initialRandom.setSeed(seed);
		shapeRandom4.setSeed(seed);
		velocityRandom.setSeed(seed);
		sizeRandom.setSeed(seed);

		float emissionRandom = globalRandom.get();
		float arcSpeedRandom = globalRandom.get();

		startTime = time + ref.startDelay.evaluate(0, emissionRandom);
		rateOverTime = ref.emission.rateOverTime.evaluate(0, emissionRandom);
		arcSpeed = arcSpeedRandom;

		int maxParticleCount = getMaxParticleCount();
		particles.reserve(maxParticleCount);
		
		if (maxParticleCount > particles.size())
			particles.insert(particles.end(), maxParticleCount - particles.size(), {});

		for (auto& burst : bursts)
			burst.nextCyclesResetTime = startTime + ref.duration;

		maxDuration = startTime + ref.duration + ref.startLifeTime.evaluate(1);

		for (auto& child : children)
		{
			child.start(time);
			maxDuration = std::max(maxDuration, child.maxDuration);
		}
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

	static DirectX::XMVECTOR limitVelocity(const DirectX::XMVECTOR& velocity, const DirectX::XMFLOAT3& limit, DirectX::XMVECTOR& accumulator, float damp, float dt)
	{
		// Extracted from UnityPlayer.dll
		float k = 1 - powf(1 - damp, dt * 30);

		DirectX::XMVECTOR velocitySignVector = DirectX::XMVectorSet(
			DirectX::XMVectorGetX(velocity) >= 0 ? 1 : -1,
			DirectX::XMVectorGetY(velocity) >= 0 ? 1 : -1,
			DirectX::XMVectorGetZ(velocity) >= 0 ? 1 : -1,
			1
		);

		DirectX::XMVECTOR absoluteVelocity = DirectX::XMVectorAbs(velocity);
		DirectX::XMVECTOR currentExceedingVelocity = DirectX::XMVectorSubtract(absoluteVelocity, accumulator);
		DirectX::XMFLOAT3 amountToLimit{};

		float absX = abs(DirectX::XMVectorGetX(currentExceedingVelocity));
		if (absX > 0.0001f && absX > limit.x)
		{
			amountToLimit.x = abs((absX - limit.x) * k);
		}

		float absY = abs(DirectX::XMVectorGetY(currentExceedingVelocity));
		if (absY > 0.0001f && absY > limit.y)
		{
			amountToLimit.y = abs((absY - limit.y) * k);
		}

		float absZ = abs(DirectX::XMVectorGetZ(currentExceedingVelocity));
		if (absZ > 0.0001f && absZ > limit.z)
		{
			amountToLimit.z = abs((absZ - limit.z) * k);
		}

		DirectX::XMVECTOR limitVector = DirectX::XMLoadFloat3(&limit);
		DirectX::XMVECTOR exceedingVelocity = DirectX::XMVectorSubtract(absoluteVelocity, limitVector);

		accumulator = DirectX::XMVectorAdd(accumulator, DirectX::XMLoadFloat3(&amountToLimit));
		return DirectX::XMVectorMultiply(DirectX::XMVectorAdd(
			DirectX::XMLoadFloat3(&limit),
			DirectX::XMVectorSubtract(exceedingVelocity, accumulator)
		), velocitySignVector);
	}

	void EmitterInstance::updateParticles(const Particle& ref, float t, const Transform& worldTransform, const Camera& camera)
	{
		const DirectX::XMMATRIX& inverseView = camera.getInverseViewMatrix();
		DirectX::XMVECTOR qLocal = quaternionFromZYX(baseTransform.rotation);
		DirectX::XMVECTOR qShift = quaternionFromZYX(worldTransform.rotation);
		DirectX::XMVECTOR rotation = DirectX::XMVectorAdd(baseTransform.rotation, ref.transform.rotation);
		DirectX::XMVECTOR pivot = DirectX::XMVectorSet(ref.pivot.x, ref.pivot.y, ref.pivot.z, 1.f);

		DirectX::XMMATRIX worldOffset = DirectX::XMMatrixIdentity();
		if (ref.name == "aura")
			worldOffset *= DirectX::XMMatrixScalingFromVector(worldTransform.scale);
		worldOffset *= DirectX::XMMatrixRotationQuaternion(qShift);
		worldOffset *= DirectX::XMMatrixTranslationFromVector(worldTransform.position);

		const bool isViewAligned = ref.renderMode == RenderMode::Billboard && ref.alignment == AlignmentMode::View;
		const bool isWorldAligned = ref.renderMode == RenderMode::Billboard && ref.alignment == AlignmentMode::World;


		for (int i = 0; i < aliveCount; i++)
		{
			float dt = t - particles[i].startTime - particles[i].time;
			particles[i].time = t - particles[i].startTime;
			float normalizedTime = particles[i].time / particles[i].duration;

			if (normalizedTime < 0)
				continue;

			if (particles[i].time >= particles[i].duration)
			{
				particles[i].alive = false;
				std::swap(particles[i], particles[aliveCount - 1]);
				aliveCount--;

				i--;
				continue;
			}

			auto& p = particles[i];

			DirectX::XMVECTOR currentRotation = isViewAligned || isWorldAligned ? p.transform.rotation : DirectX::XMVectorAdd(rotation, p.transform.rotation);
			if (ref.rotationOverLifetime.enabled)
			{
				DirectX::XMFLOAT3 temp = ref.rotationOverLifetime.integrate(0, normalizedTime, p.duration, p.rotationLerpRatio);
				currentRotation = DirectX::XMVectorSubtract(currentRotation, DirectX::XMLoadFloat3(&temp));
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
				DirectX::XMFLOAT3 sol = ref.sizeOverLifetime.evaluate(normalizedTime, p.sizeLerpRatio, 1.f);
				currentScale = DirectX::XMVectorMultiply(currentScale, DirectX::XMLoadFloat3(&sol));
			}

			DirectX::XMVECTOR currentVelocity{};
			if (ref.velocityOverLifetime.enabled)
			{
				DirectX::XMFLOAT3 vol = ref.velocityOverLifetime.evaluate(normalizedTime, p.velocityLerpRatio);
				DirectX::XMVECTOR vol1 = DirectX::XMLoadFloat3(&vol);
				if (ref.velocitySpace == TransformSpace::Local)
				{
					vol1 = DirectX::XMVector3Rotate(vol1, qLocal);
				}
				currentVelocity = DirectX::XMVectorAdd(currentVelocity, vol1);
			}

			if (ref.forceOverLifetime.enabled)
			{
				DirectX::XMFLOAT3 fol = ref.forceOverLifetime.integrate(0, normalizedTime, p.duration, p.forceLerpRatio);
				DirectX::XMVECTOR fol1 = DirectX::XMLoadFloat3(&fol);
				if (ref.forceSpace == TransformSpace::Local)
				{
					fol1 = DirectX::XMVector3Rotate(fol1, qLocal);
				}
				currentVelocity = DirectX::XMVectorAdd(currentVelocity, fol1);
			}

			float speedModifier = ref.speedModifier.evaluate(normalizedTime, p.velocityLerpRatio, 1.f);
			currentVelocity = DirectX::XMVectorMultiplyAdd(p.direction, DirectX::XMVectorReplicate(p.speed), currentVelocity);
			currentVelocity = DirectX::XMVectorMultiply(currentVelocity, DirectX::XMVectorReplicate(speedModifier));

			float gravity = GRAVITY * ref.gravityModifier.evaluate(normalizedTime, p.gravityLerpRatio) * p.time;
			DirectX::XMVECTOR gravityVector = DirectX::XMVectorSet(0, -gravity, 0, 0);
			currentVelocity = DirectX::XMVectorMultiplyAdd(currentVelocity, velocityScale, gravityVector);
			
			if (ref.limitVelocityOverLifetime.enabled)
			{
				DirectX::XMFLOAT3 velocityLimit = ref.limitVelocityOverLifetime.evaluate(normalizedTime, p.limitVelocityLerpRatio);
				currentVelocity = limitVelocity(currentVelocity, velocityLimit, p.accumulateVelocityLimit, ref.limitVelocityDampen, dt);
			}

			p.transform.position = DirectX::XMVectorAdd(
				DirectX::XMVectorMultiply(currentVelocity, DirectX::XMVectorReplicate(dt)),
				p.transform.position
			);

			float rotationFactor = p.flipRotation ? -1 : 1;

			p.matrix = DirectX::XMMatrixIdentity();
			DirectX::XMMATRIX directionMatrix = DirectX::XMMatrixIdentity();
			DirectX::XMVECTOR pivotScale = currentScale;
			switch (ref.renderMode)
			{
			case RenderMode::Billboard:
				switch (ref.alignment)
				{
				case AlignmentMode::View:
					directionMatrix = inverseView;
					break;
				}
				break;
			case RenderMode::StretchedBillboard:
				directionMatrix = rotateToDirection(ref, currentVelocity, currentScale);
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

			p.matrix *= DirectX::XMMatrixScalingFromVector(currentScale);
			p.matrix *= DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorMultiply(pivot, pivotScale));
			DirectX::XMMATRIX m4Rotation = DirectX::XMMatrixRotationQuaternion(quaternionFromZYX(currentRotation));

			if (ref.renderMode == RenderMode::StretchedBillboard)
				directionMatrix *= DirectX::XMMatrixInverse(nullptr, m4Rotation);

			p.matrix *= directionMatrix;
			p.matrix *= m4Rotation;
			p.matrix *= DirectX::XMMatrixTranslationFromVector(p.transform.position);

			if (ref.simulationSpace == TransformSpace::Local)
				p.matrix *= worldOffset;
		}
	}

	void EmitterInstance::update(float t, const Transform& worldTransform, const Camera& camera)
	{
		const Particle& ref = ResourceManager::getParticleEffect(refID);
		updateEmission(ref, worldTransform, t);

		if (aliveCount > 0)
			updateParticles(ref, t, worldTransform, camera);

		for (auto& em : children)
			em.update(t, worldTransform, camera);
	}
}
