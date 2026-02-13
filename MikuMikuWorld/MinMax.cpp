#include "MinMax.h"
#include <algorithm>

namespace MikuMikuWorld::Effect
{
	// Referenced from https://discussions.unity.com/t/what-is-the-math-behind-animationcurve-evaluate/72058/7
	// Thanks a lot agate-pris!
	float hermite(const KeyFrame& k1, const KeyFrame& k2, float time)
	{
		float dt = k2.time - k1.time;

		const float x0 = k1.time;
		float y0 = k1.value;
		float x1 = k1.time + k1.outWeight * dt;
		float y1 = k1.value + k1.outTangent * k1.outWeight * dt;
		float x2 = k2.time - k1.inWeight * dt;
		float y2 = k2.value - k2.inTangent * k2.inWeight * dt;
		float x3 = k2.time;
		float y3 = k2.value;

		float a = x3 - 3 * x2 + 3 * x1 - x0;
		float b = 3 * x0 - 6 * x1 + 3 * x2;
		float c = 3 * x1 - 3 * x0;
		float d = x0 - time;

		float oneMinusT = 1 - time;
		float oneMinusT2 = oneMinusT * oneMinusT;
		float oneMinusT3 = oneMinusT2 * oneMinusT;
		float t2 = time * time;
		float t3 = t2 * time;

		return oneMinusT3 * y0 + 3 * oneMinusT2 * time * y1 + 3 * oneMinusT * t2 * y2 + t3 * y3;
	}

	float hermiteArea(const KeyFrame& k1, const KeyFrame& k2, float time)
	{
		float dt = k2.time - k1.time;

		float x0 = k1.time;
		float y0 = k1.value;
		float x1 = k1.time + k1.outWeight * dt;
		float y1 = k1.value + k1.outTangent * k1.outWeight * dt;
		float x2 = k2.time - k1.inWeight * dt;
		float y2 = k2.value - k2.inTangent * k2.inWeight * dt;
		float x3 = k2.time;
		float y3 = k2.value;

		float a = -y0 + 3 * y1 - 3 * y2 + y3;
		float b = 3 * y0 - 6 * y1 + 3 * y2;
		float c = -3 * y0 + 3 * y1;
		float d = y0;

		float t2 = time * time;
		float t3 = t2 * time;
		float t4 = t3 * time;

		return (a / 4.0f) * t4 + (b / 3.0f) * t3 + (c / 2.0f) * t2 + (d * time);
	}

	static int findKeyFrame(const std::vector<KeyFrame>& keyframes, float time)
	{
		int min = 0;
		int max = keyframes.size() - 1;

		while (min <= max)
		{
			int index = (min + max) / 2;

			if (time < keyframes[index].time)
				max = index - 1;
			else
				min = index + 1;
		}

		return min;
	}

	static float evaluateCurve(const std::vector<KeyFrame>& keyframes, float time, float fallback = 0.f)
	{
		if (keyframes.empty())
			return fallback;

		if (time <= keyframes.begin()->time)
			return keyframes.begin()->value;

		if (time >= keyframes.rbegin()->time)
			return keyframes.rbegin()->value;

		int index = findKeyFrame(keyframes, time);
		if (index == 0)
			return keyframes.begin()->value;

		const KeyFrame& k1 = keyframes[index - 1];
		const KeyFrame& k2 = keyframes[index];

		float ratio = k2.time - k1.time > 0 ? (time - k1.time) / (k2.time - k1.time) : 0;
		return hermite(k1, k2, ratio);
	}

	static float integrateCurve(const std::vector<KeyFrame>& keyframes, float from, float to, float scale)
	{
		if (keyframes.empty())
			return 0;

		if (keyframes.size() == 1)
			return keyframes.begin()->value;

		int index = findKeyFrame(keyframes, to);
		if (index == 0)
			return keyframes.begin()->value;

		float total = 0;
		for (int i = 0; i < index - 1; i++)
		{
			float timeFactor = keyframes[i + 1].time - keyframes[i].time;
			float value = hermiteArea(keyframes[i], keyframes[i + 1], 1) * (timeFactor * scale);
			total += value;
		}

		if (index <= keyframes.size() - 1)
		{
			const KeyFrame& k1 = keyframes[index - 1];
			const KeyFrame& k2 = keyframes[index];
			float ratio = k2.time - k1.time > 0 ? (to - k1.time) / (k2.time - k1.time) : 0;

			float value = hermiteArea(k1, k2, ratio) * ((k2.time - k1.time) * scale);
			total += value;
		}

		return total;
	}

	int MinMaxColor::findKeyFrame(const std::vector<ColorKeyFrame>& keyframes, float time) const
	{
		int min = 0;
		int max = keyframes.size()  - 1;

		while (min <= max)
		{
			int index = (min + max) / 2;

			if (time < keyframes[index].time)
				max = index - 1;
			else
				min = index + 1;
		}

		return min;
	}

	Color MinMaxColor::at(const std::vector<ColorKeyFrame>& keyframes, float time) const
	{
		if (keyframes.empty())
			return Color{ 1.0f, 1.0f, 1.0f, 1.0f };

		if (time >= keyframes.rbegin()->time)
			return keyframes.rbegin()->color;

		int index = findKeyFrame(keyframes, time);
		if (index == 0)
			return keyframes.begin()->color;

		const ColorKeyFrame& k1 = keyframes[index - 1];
		const ColorKeyFrame& k2 = keyframes[index];

		float ratio = k2.time - k1.time > 0 ? (time - k1.time) / (k2.time - k1.time) : 0;

		float r = lerp(k1.color.r, k2.color.r, ratio);
		float g = lerp(k1.color.g, k2.color.g, ratio);
		float b = lerp(k1.color.b, k2.color.b, ratio);
		float a = lerp(k1.color.a, k2.color.a, ratio);
		return Color(r, g, b, a);
	}

	Color MinMaxColor::evaluate(float time, float lerpRatio) const
	{
		switch (mode)
		{
		case MinMaxColorMode::TwoColors:
			return Color(
				lerp(min.r, max.r, lerpRatio),
				lerp(min.g, max.g, lerpRatio),
				lerp(min.b, max.b, lerpRatio),
				lerp(min.a, max.a, lerpRatio)
			);
		case MinMaxColorMode::Random:
			return gradientMin[std::clamp(static_cast<size_t>(gradientMin.size() * lerpRatio), 0ull, gradientMin.size() - 1)].color;
		case MinMaxColorMode::Gradient:
			return at(gradientMin, time);
		case MinMaxColorMode::TwoGradients:
		{
			const Color& c1 = at(gradientMin, time);
			const Color& c2 = at(gradientMax, time);
			return Color(
				lerp(c1.r, c2.r, lerpRatio),
				lerp(c1.g, c2.g, lerpRatio),
				lerp(c1.b, c2.b, lerpRatio),
				lerp(c1.a, c2.a, lerpRatio)
			);
		}
		default:
			return constant;
		}
	}

	void MinMaxColor::addKeyFrame(const ColorKeyFrame& k, MinMaxCurve curve)
	{
		switch (curve)
		{
		case MinMaxCurve::Min:
			gradientMin.emplace_back(k);
			break;
		default:
			gradientMax.emplace_back(k);
			break;
		}
	}

	void MinMaxColor::removeKeyFrame(size_t index, MinMaxCurve curve)
	{
		switch (curve)
		{
		case MinMaxCurve::Min:
			gradientMin.erase(gradientMin.begin() + index);
			break;
		default:
			gradientMax.erase(gradientMin.begin() + index);
			break;
		}
	}

	void MinMaxColor::sortKeyFrames()
	{
		auto keyframeSortFn = [](const ColorKeyFrame& k1, const ColorKeyFrame& k2) { return k1.time <= k2.time; };
		std::sort(gradientMin.begin(), gradientMin.end(), keyframeSortFn);
		std::sort(gradientMax.begin(), gradientMax.end(), keyframeSortFn);
	}

	float MinMax::evaluate(float time, float lerpRatio, float fallback) const
	{
		switch (mode)
		{
		case MinMaxMode::TwoConstants:
			return lerp(min, max, lerpRatio);
		case MinMaxMode::Curve:
			return evaluateCurve(curveMin, time, fallback);
		case MinMaxMode::TwoCurves:
			return lerp(evaluateCurve(curveMin, time), evaluateCurve(curveMax, time), lerpRatio);
		default:
			return constant;
		}
	}

	float MinMax::integrate(float from, float to, float scale, float lerpRatio, float fallback) const
	{
		switch (mode)
		{
		case MinMaxMode::TwoConstants:
			return lerp(min, max, lerpRatio) * ((to - from) * scale);
		case MinMaxMode::Curve:
			return integrateCurve(curveMin, from, to, scale);
		case MinMaxMode::TwoCurves:
			return lerp(integrateCurve(curveMin, from, to, scale), integrateCurve(curveMax, from, to, scale), lerpRatio);
		default:
			return constant * ((to - from) * scale);
		}
	}

	void MinMax::addKeyFrame(const KeyFrame& k, MinMaxCurve curve)
	{
		switch (curve)
		{
		case MinMaxCurve::Max:
			curveMax.emplace_back(k);
			break;
		default:
			curveMin.emplace_back(k);
			break;
		}
	}

	void MinMax::removeKeyFrame(size_t index, MinMaxCurve curve)
	{
		switch (curve)
		{
		case MinMaxCurve::Max:
			curveMax.erase(curveMax.begin() + index);
			break;
		default:
			curveMin.erase(curveMin.begin() + index);
			break;
		}
	}

	void MinMax::sortKeyFrames()
	{
		auto keyframeSortFn = [](const KeyFrame& k1, const KeyFrame& k2) { return k1.time <= k2.time; };
		std::sort(curveMin.begin(), curveMin.end(), keyframeSortFn);
		std::sort(curveMax.begin(), curveMax.end(), keyframeSortFn);
	}
}