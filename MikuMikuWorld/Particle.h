#pragma once
#include <string>
#include "Math.h"
#include "MinMax.h"
#include "Utilities.h"
#include "DirectXMath.h"

namespace MikuMikuWorld
{
	class Camera;
}

namespace MikuMikuWorld::Effect
{
	enum class EmissionShape : uint8_t
	{
		Sphere,
		HemiShpere = 2,
		Cone = 4,
		Box,
		Circle = 10
	};

	enum class BlendMode : uint8_t
	{
		/// <summary>
		/// Normal alpha blending
		/// </summary>
		Typical,

		/// <summary>
		/// Additive alpha blending
		/// </summary>
		Additive
	};

	enum class EmitFrom : uint8_t
	{
		Base,
		Volume,
	};

	enum class TransformSpace : uint8_t
	{
		/// <summary>
		/// Transform along the particle's local axes
		/// </summary>
		Local,

		/// <summary>
		/// Transform along the world's axes
		/// </summary>
		World
	};

	enum class ScalingMode : uint8_t
	{
		/// <summary>
		/// Particle is scaled by its parent's scale
		/// </summary>
		Hierarchy,

		/// <summary>
		/// Particle does not inherit its parent's scale
		/// </summary>
		Local,

		/// <summary>
		/// Particle's scale is affected by the emission shape's scale
		/// </summary>
		Shape
	};

	enum class RenderMode : uint8_t
	{
		/// <summary>
		/// Normal billboard
		/// </summary>
		Billboard,

		/// <summary>
		/// ???
		/// </summary>
		StretchedBillboard,

		/// <summary>
		/// Particle lays flat on the ground
		/// </summary>
		HorizontalBillboard,

		/// <summary>
		/// Particle lays flat on a wall
		/// </summary>
		VerticalBillboard,

		/// <summary>
		/// Particle uses a mesh to render
		/// </summary>
		Mesh,

		/// <summary>
		/// Particle is not rendered
		/// </summary>
		None,
	};

	enum class AlignmentMode : uint8_t
	{
		/// <summary>
		/// The particle always faces the view (Normal billboarding)
		/// </summary>
		View,

		/// <summary>
		/// The particle is aligned to the world transform
		/// </summary>
		World,

		/// <summary>
		/// The particle is aligned to its local transform
		/// </summary>
		Local,

		/// <summary>
		/// The particle faces the position of the camera
		/// </summary>
		Facing,

		/// <summary>
		/// The particle faces the direction of its velocity
		/// </summary>
		Velocity
	};

	struct EmissionBurst
	{
		/// <summary>
		/// The start time of the burst
		/// </summary>
		float time{};

		/// <summary>
		/// How many particles are emitted per burst
		/// </summary>
		int count{};

		/// <summary>
		/// The numner of times the burst occurs. Infinite if 0;
		/// </summary>
		int cycles{};

		/// <summary>
		/// The time between each burst
		/// </summary>
		float interval{};

		/// <summary>
		/// The probability of a burst emitting particles [0 - 1]
		/// </summary>
		float probability{1.f};
	};

	enum class ArcMode : uint8_t
	{
		/// <summary>
		/// The emission position is random
		/// </summary>
		Random,

		/// <summary>
		/// The emission angle is incremented with each emission
		/// </summary>
		Loop,

		/// <summary>
		/// Same as loop but goes back and forth instead of restarting from the beginning of the arc
		/// </summary>
		PingPong,

		/// <summary>
		/// The particles from 1 burst are spread along the arc
		/// </summary>
		BurstSpread
	};

	struct Transform
	{
		DirectX::XMVECTOR position{0, 0, 0, 1};
		DirectX::XMVECTOR rotation{0, 0, 0, 0};
		DirectX::XMVECTOR scale{1, 1, 1, 1};
	};

	struct Emission
	{
		EmissionShape shape{};
		MinMax rateOverTime;
		MinMax rateOverDistance;
		std::vector<EmissionBurst> bursts;

		float angle{};
		float radius{0.0001f};
		float radiusThickness{0.0001f};
		float arc{360.f};
		MinMax arcSpeed{};
		float randomizeDirection{};
		float randomizePosition{};
		float spherizeDirection{};
		ArcMode arcMode;
		EmitFrom emitFrom;

		Transform transform;
	};

	struct Particle
	{
		int ID{};

		std::string name{};
		Transform transform{};

		float duration{};
		int maxParticles{};
		float flipRotation{};
		bool looping{};
		
		uint32_t randomSeed{};
		bool useAutoRandomSeed{ true };
		
		MinMax startDelay{};
		MinMax startLifeTime{};
		MinMax startSpeed{};
		MinMax gravityModifier{};
		MinMax speedModifier{};
		MinMax3 startSize{};
		MinMax3 startRotation{};
		MinMaxColor startColor{};

		Emission emission{};

		MinMax3 limitVelocitySpeed{};
		float limitVelocityDrag{};
		float limitVelocityDampen{};

		int order{};
		BlendMode blend{};
		RenderMode renderMode{};
		AlignmentMode alignment{};
		Vector3 pivot{};
		float speedScale{ 1 };
		float lengthScale{ 1 };

		int textureSplitX{};
		int textureSplitY{};
		MinMax startFrame{};
		MinMax frameOverTime{};

		ScalingMode scalingMode{};
		TransformSpace velocitySpace{};
		TransformSpace simulationSpace{};
		TransformSpace forceSpace{};

		MinMaxColor colorOverLifetime{};
		MinMax3 velocityOverLifetime{};
		MinMax3 limitVelocityOverLifetime{};
		MinMax3 forceOverLifetime{};
		MinMax3 sizeOverLifetime{};
		MinMax3 rotationOverLifetime{};

		std::vector<int> children;
	};

	struct ParticleInstance
	{
		DirectX::XMMATRIX matrix{};
		Transform transform;
		DirectX::XMVECTOR direction{};
		float startTime{};
		float time{};
		float duration{};

		float gravityLerpRatio{};
		float spriteSheetLerpRatio{};
		float velocityLerpRatio{};
		float sizeLerpRatio{};
		float colorLerpRatio{};
		float limitVelocityLerpRatio{};
		float forceLerpRatio{};
		float rotationLerpRatio{};

		float speed{};
		Color startColor{ 1.f, 1.f, 1.f, 1.f };

		bool flipRotation{};
		bool alive{ false };
	};

	struct BurstInstance
	{
		/// <summary>
		/// The time of the last emission from this burst
		/// </summary>
		float lastBurstTime{};

		/// <summary>
		/// The number of cycles a burst has gone through
		/// </summary>
		int cycleCount{};

		/// <summary>
		/// The time at which the cycleCount will reset
		/// </summary>
		float nextCyclesResetTime{};
	};

	class EmitterInstance
	{
	public:
		Transform baseTransform{};

		/// <summary>
		/// The start time of the emitter
		/// </summary>
		float startTime{};

		/// <summary>
		/// The last time a particle was emitted (rate over time only)
		/// </summary>
		float lastEmissionTime{};

		/// <summary>
		/// The number of particles to be emitted
		/// </summary>
		float emissionAccumulator{};

		/// <summary>
		/// Accumulator to shift the emission position
		/// </summary>
		float emissionPosition{};

		/// <summary>
		/// Increment applied to emissionPosition
		/// </summary>
		float emissionPositionInterval{};

		/// <summary>
		/// Whether the particle system is drawn or not
		/// </summary>
		bool visible{ true };

		/// <summary>
		/// The speed at which the emit position moves through the arc
		/// </summary>
		float arcSpeed{};

		/// <summary>
		/// The amount of particles emitted in one second
		/// </summary>
		float rateOverTime{ 1 };

		/// <summary>
		/// The number of alive particles
		/// </summary>
		int aliveCount{};

		RandN initialRandom;
		RandN shapeRandom;
		RandN sizeRandom;
		RandN velocityRandom;

		std::vector<BurstInstance> bursts;
		std::vector<ParticleInstance> particles;
		std::vector<EmitterInstance> children;

		void updateEmission(const Particle& ref, const Transform& worldTransform, float time);
		void emit(const Transform& worldTransform, const Particle& ref, float time);
		void update(float time, const Transform& worldTransform, const Camera& camera);
		void start(float time);
		void stop(bool allChildren);
		void init(const Particle& ref, const Transform& transform);

		inline int getRefID() const { return refID; }

	private:
		int refID{};

		int findFirstDeadParticle(float time) const;
		int getMaxParticleCount() const;
	};
}