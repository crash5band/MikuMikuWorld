#pragma once
#include <DirectXMath.h>

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

	struct Vector3
	{
		float x;
		float y;
		float z;

		Vector3(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f) :
			x{ _x }, y{ _y }, z{ _z }
		{

		}

		Vector3 operator+(const Vector3& v)
		{
			return Vector3(x + v.x, y + v.y, z + v.z);
		}

		Vector3 operator-(const Vector3& v)
		{
			return Vector3(x - v.x, y - v.y, z - v.z);
		}

		Vector3 operator*(const Vector3& v)
		{
			return Vector3(x * v.x, y * v.y, z * v.z);
		}

		Vector3 transform(const DirectX::XMMATRIX& m)
		{
			DirectX::XMVECTOR v{ x, y, z, 1.0f };
			v = DirectX::XMVector3Transform(v, m);
			return Vector3(v.m128_f32[0], v.m128_f32[1], v.m128_f32[2]);
		}
	};

	struct Color
	{
		float r, g, b, a;

		Color(float _r = 0.0f, float _g = 0.0f, float _b = 0.0f, float _a = 1.0f)
			: r{ _r }, g{ _g }, b{ _b }, a{ _a }
		{

		}
	};

	class Rectangle
	{
	private:
		float x1, x2, y1, y2;

	public:
		Rectangle(Vector2 start, Vector2 end);
		Rectangle(float x, float y, float width, float height);

		inline float getX1() const { return x1; }
		inline float getX2() const { return x2; }
		inline float getY1() const { return y1; }
		inline float getY2() const { return y2; }
		inline float getWidth() const;
		inline float getHeight() const;
		bool intersects(float x, float y) const;
		bool intersects(const Vector2& p) const;
	};

	float lerp(float start, float end, float percentage);
	float easeIn(float x);
	float easeOut(float x);

	bool isWithinRange(float x, float left, float right);
	float convertRange(float val, float originalStart, float originalEnd, float newStart, float newEnd);
}