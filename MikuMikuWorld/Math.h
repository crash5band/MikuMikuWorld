#pragma once
#include "ImGui/imgui.h"
#include <functional>
#include "NoteTypes.h"

namespace MikuMikuWorld
{
	struct Vector2
	{
		float x;
		float y;

		Vector2(float _x, float _y)
			: x{ _x }, y{ _y }
		{

		}

		Vector2() : x{ 0 }, y{ 0 }
		{

		}

		Vector2 operator+(const Vector2& v)
		{
			return Vector2(x + v.x, y + v.y);
		}

		Vector2 operator-(const Vector2& v)
		{
			return Vector2(x - v.x, y - v.y);
		}

		Vector2 operator*(const Vector2& v)
		{
			return Vector2(x * v.x, y * v.y);
		}
	};

	struct Color
	{
	public:
		float r, g, b, a;

		explicit Color(float _r = 0.0f, float _g = 0.0f, float _b = 0.0f, float _a = 1.0f)
			: r{ _r }, g{ _g }, b{ _b }, a{ _a }
		{

		}
		explicit Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
			: r{ r / 255.f }, g{ g / 255.f}, b{ b / 255.f}, a{ a / 255.f }
		{

		}

		inline bool operator==(const Color& c) const { return r == c.r && g == c.g && b == c.b && a == c.a; }
		inline bool operator!=(const Color& c) const { return !(*this == c); }

		static inline int rgbaToInt(int r, int g, int b, int a) { return r << 24 | g << 16 | b << 8 | a; }
		static inline int abgrToInt(int a, int b, int g, int r) { return a << 24 | b << 16 | g << 8 | r; }

		inline ImVec4 toImVec4() const { return ImVec4{ r, g, b, a }; }
		static inline Color fromImVec4(const ImVec4& col) { return Color{ col.x, col.y, col.z, col.w }; }

		inline Color scaleAlpha(float scalar) { return Color{ r, g, b, a * scalar}; }
	};

	constexpr uint32_t roundUpToPowerOfTwo(uint32_t v)
	{
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v++;
		return v;
	}

	float lerp(float start, float end, float ratio);
	float unlerp(float start, float end, float value);
	double lerpD(double start, double end, double ratio);
	double unlerpD(double start, double end, double value);
	float easeIn(float start, float end, float ratio);
	float easeOut(float start, float end, float ratio);
	float midpoint(float x1, float x2);
	bool isWithinRange(float x, float left, float right);
	template <typename T, typename cmp_t = std::less<T>>
	inline const T& clamp(const T& v, const T& a, const T& b)
	{
		cmp_t cmp;
		if (cmp(v, a))
			return a;
		if (cmp(b, v))
			return b;
		return v;
	}

	std::function<float(float, float, float)> getEaseFunction(EaseType ease);

	uint32_t gcf(uint32_t a, uint32_t b);

	static constexpr double NUM_PI = 3.14159265358979323846;
	static constexpr double NUM_PI_2 = 3.14159265358979323846 / 2;

	namespace Engine
	{
		struct Range
		{
			float min;
			float max;
		};

		enum class Easing : uint8_t
		{
			Linear,
			Sine,
			Quad,
			Cubic,
			Quart,
			Quint,
			Expo,
			Circ,
			Back,
			Elastic,
			EasingCount,
			TypeMask = 0b1111,
			In    = 1 << 4,
			Out   = 1 << 5,
			InOut = In | Out,
			OutIn = 1 << 6
		};

		using EasingFunc = float(*)(float x);

		float easeLinear(float x);
		float easeInSine(float x);
		float easeOutSine(float x);
		float easeInOutSine(float x);
		float easeOutInSine(float x);
		float easeInQuad(float x);
		float easeOutQuad(float x);
		float easeInOutQuad(float x);
		float easeOutInQuad(float x);
		float easeInCubic(float x);
		float easeOutCubic(float x);
		float easeInOutCubic(float x);
		float easeOutInCubic(float x);
		float easeInQuart(float x);
		float easeOutQuart(float x);
		float easeInOutQuart(float x);
		float easeOutInQuart(float x);
		float easeInQuint(float x);
		float easeOutQuint(float x);
		float easeInOutQuint(float x);
		float easeOutInQuint(float x);
		float easeInExpo(float x);
		float easeOutExpo(float x);
		float easeInOutExpo(float x);
		float easeOutInExpo(float x);
		float easeInCirc(float x);
		float easeOutCirc(float x);
		float easeInOutCirc(float x);
		float easeOutInCirc(float x);
		float easeInBack(float x);
		float easeOutBack(float x);
		float easeInOutBack(float x);
		float easeOutInBack(float x);
		float easeInElastic(float x);
		float easeOutElastic(float x);
		float easeInOutElastic(float x);
		float easeOutInElastic(float x);

		EasingFunc getEaseFunc(Easing easing);
	}
}