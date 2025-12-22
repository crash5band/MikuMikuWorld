#pragma once
#include "Math.h"
#include "Utilities.h"
#include <vector>
#include <DirectXMath.h>

namespace MikuMikuWorld::Effect
{
	struct ColorKeyFrame
	{
		float time{};
		Color color{};
	};

	struct KeyFrame
	{
		float time{};
		float value{};
		float inTangent{};
		float outTangent{};
		float inWeight{};
		float outWeight{};
	};

	float hermite(const KeyFrame& k1, const KeyFrame& k2, float ratio);
	float hermiteArea(const KeyFrame& k1, const KeyFrame& k2, float ratio);

	enum class MinMaxMode
	{
		/// <summary>
		/// Constant value
		/// </summary>
		Constant,

		/// <summary>
		/// Uses a single curve
		/// </summary>
		Curve,

		/// <summary>
		/// Uses a random value between 2 curves
		/// </summary>
		TwoCurves,

		/// <summary>
		/// Uses a random value between two constants
		/// </summary>
		TwoConstants
	};

	enum class MinMaxCurve
	{
		Min, Max
	};

	enum class MinMaxColorMode
	{
		/// <summary>
		/// Constant color
		/// </summary>
		Constant,

		/// <summary>
		/// Uses a single color gradient
		/// </summary>
		Gradient,

		/// <summary>
		/// Uses a random value between two colors
		/// </summary>
		TwoColors,

		/// <summary>
		/// Usees a random color between two color gradients
		/// </summary>
		TwoGradients,

		/// <summary>
		/// Chooses a random color from a list
		/// </summary>
		Random
	};

	class MinMax
	{
	public:
		MinMaxMode mode{};
		float constant{};
		float min{};
		float max{};

		inline float evaluate() const { return evaluate(0, 0); }
		inline float evaluate(float lerpRatio) const { return evaluate(0, lerpRatio); }
		float evaluate(float time, float lerpRatio, float fallback = 0.f) const;
		float integrate(float from, float to, float scale, float lerpRatio, float fallback = 0.f) const;

		void addKeyFrame(const KeyFrame& k, MinMaxCurve curve = MinMaxCurve::Min);
		void removeKeyFrame(size_t index, MinMaxCurve curve = MinMaxCurve::Min);

		void sortKeyFrames();
	private:
		std::vector<KeyFrame> curveMin;
		std::vector<KeyFrame> curveMax;
	};

	class MinMax3
	{
	public:
		bool enabled{};
		bool is3D{};
		MinMax x{}, y{}, z{};

		inline DirectX::XMFLOAT3 evaluate(float time, float lerpRatio, float fallback = 0.f) const
		{
			return DirectX::XMFLOAT3(
				x.evaluate(time, lerpRatio, fallback),
				y.evaluate(time, lerpRatio, fallback),
				z.evaluate(time, lerpRatio, fallback)
			);
		}

		inline DirectX::XMFLOAT3 evaluate(float time, DirectX::XMFLOAT3 lerpRatio, float fallback = 0.f) const
		{
			return DirectX::XMFLOAT3(
				x.evaluate(time, lerpRatio.x, fallback),
				y.evaluate(time, lerpRatio.y, fallback),
				z.evaluate(time, lerpRatio.z, fallback)
			);
		}

		inline DirectX::XMFLOAT3 integrate(float from, float to, float scale, float lerpRatio) const
		{
			return DirectX::XMFLOAT3(
				x.integrate(from, to, scale, lerpRatio),
				y.integrate(from, to, scale, lerpRatio),
				z.integrate(from, to, scale, lerpRatio)
			);
		}

		inline DirectX::XMFLOAT3 integrate(float from, float to, float scale, DirectX::XMFLOAT3 lerpRatio) const
		{
			return DirectX::XMFLOAT3(
				x.integrate(from, to, scale, lerpRatio.x),
				y.integrate(from, to, scale, lerpRatio.y),
				z.integrate(from, to, scale, lerpRatio.z)
			);
		}
	};

	class MinMaxColor
	{
	public:
		MinMaxColorMode mode{};
		Color constant{};
		Color min{};
		Color max{};

		inline Color evaluate() const { return evaluate(0, 0); }
		inline Color evaluate(float lerpRatio) const { return evaluate(0, lerpRatio); }
		Color evaluate(float time, float lerpRatio) const;

		void addKeyFrame(const ColorKeyFrame& k, MinMaxCurve curve);
		void removeKeyFrame(size_t index, MinMaxCurve curve);
		
		void sortKeyFrames();
	private:
		Color at(const std::vector<ColorKeyFrame>& keyframes, float time) const;
		int findKeyFrame(const std::vector<ColorKeyFrame>& keyframes, float time) const;

		std::vector<ColorKeyFrame> gradientMin;
		std::vector<ColorKeyFrame> gradientMax;
	};
}