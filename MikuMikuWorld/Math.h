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

		inline bool operator==(const Color& c) const { return r == c.r && g == c.g && b == c.b && a == c.a; }
		inline bool operator!=(const Color& c) const { return !(*this == c); }

		static inline int rgbaToInt(int r, int g, int b, int a) { return r << 24 | g << 16 | b << 8 | a; }
		static inline int abgrToInt(int a, int b, int g, int r) { return a << 24 | b << 16 | g << 8 | r; }

		inline ImVec4 toImVec4() const { return ImVec4{ r, g, b, a }; }
		static inline Color fromImVec4(const ImVec4& col) { return Color{ col.x, col.y, col.z, col.w }; }
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
	float easeIn(float start, float end, float ratio);
	float easeOut(float start, float end, float ratio);
	float midpoint(float x1, float x2);
	bool isWithinRange(float x, float left, float right);

	std::function<float(float, float, float)> getEaseFunction(EaseType ease);

	uint32_t gcf(uint32_t a, uint32_t b);
}