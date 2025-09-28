#include "cmath"
#include "Math.h"

namespace MikuMikuWorld
{
	float lerp(float start, float end, float percentage)
	{
		return start + percentage * (end - start);
	}

	float unlerp(float start, float end, float value)
	{
		return (value - start) / (end - start);
	}

	double lerpD(double start, double end, double percentage)
	{
		return start + percentage * (end - start);
	}

	double unlerpD(double start, double end, double value)
	{
		return (value - start) / (end - start);
	}

	float easeIn(float start, float end, float ratio)
	{
		return lerp(start, end, ratio * ratio);
	}

	float easeOut(float start, float end, float ratio)
	{
		return lerp(start, end, 1 - (1 - ratio) * (1 - ratio));
	}

	float midpoint(float x1, float x2)
	{
		return (x1 + x2) * 0.5f;
	}

	bool isWithinRange(float x, float left, float right)
	{
		return x >= left && x <= right;
	}

	std::function<float(float, float, float)> getEaseFunction(EaseType ease)
	{
		switch (ease)
		{
		case EaseType::EaseIn:
			return easeIn;
		case EaseType::EaseOut:
			return easeOut;
		default:
			break;
		}

		return lerp;
	}

	uint32_t gcf(uint32_t a, uint32_t b)
	{
		for (;;)
		{
			if (b == 0)
			{
				break;
			}
			else
			{
				uint32_t t = a;
				a = b;
				b = t % a;
			}
		}

		return a;
	}

	namespace Engine
	{
		// General formular for deriving ease funcions
		// easeIn = some function that easeIn(0) = 0 and easeIn(1) = 1
		// easeOut = 1 - easeIn(1-x)
		// easeInOut = x < 0.5f ? easeIn(2x) / 2 : (easeOut(2x - 1) + 1) / 2
		// easeOutIn = x < 0.5f ? easeOut(2x) / 2 : (easeIn(2x - 1) + 1) / 2

		float easeLinear(float x) { return x; }
		float easeInSine(float x) { return 1 - std::cos(NUM_PI_2 * x); }
		float easeOutSine(float x) { return std::sin(NUM_PI_2 * x); }
		float easeInOutSine(float x) { return 0.5f - std::cos(NUM_PI * x) / 2; }
		float easeOutInSine(float x) { return x < 0.5f ? (std::sin(NUM_PI * x) / 2) : (1 - std::sin(NUM_PI * x) / 2); }
		float easeInQuad(float x) { return x*x; }
		float easeOutQuad(float x) { return 2*x - x*x; }
		float easeInOutQuad(float x) { return x < 0.5f ? 2*x*x : 4*x - 2*x*x - 1; }
		float easeOutInQuad(float x) { return x < 0.5f ? 2*x - 2*x*x : 2*x*x - 2*x + 1; }
		float easeInCubic(float x) { return x*x*x; }
		float easeOutCubic(float x) { return 3*x - 3*x*x + x*x*x; }
		float easeInOutCubic(float x) { return x < 0.5f ? 4*x*x*x : -3 + 12*x - 12*x*x + 4*x*x*x; }
		float easeOutInCubic(float x) { return 3*x - 6*x*x + 4*x*x*x; }
		float easeInQuart(float x) { return x*x*x*x; }
		float easeOutQuart(float x) { return 4*x - 6*x*x + 4*x*x*x - x*x*x*x; }
		float easeInOutQuart(float x) { return x < 0.5f ? 8*x*x*x*x : -7 + 32*x - 48*x*x + 32*x*x*x - 8*x*x*x*x; }
		float easeOutInQuart(float x) { return x < 0.5f ? 4*x - 12*x*x + 16*x*x*x - 8*x*x*x*x : 1 - 4*x + 12*x*x - 16*x*x*x + 8*x*x*x*x; }
		float easeInQuint(float x) { return x*x*x*x*x; }
		float easeOutQuint(float x) { return 5*x - 10*x*x + 10*x*x*x - 5*x*x*x*x + x*x*x*x*x; }
		float easeInOutQuint(float x) { return (x < 0.5f ? 0: -15 + 80*x - 160*x*x + 160*x*x*x - 80*x*x*x*x ) + 16*x*x*x*x*x; }
		float easeOutInQuint(float x) { return 5*x - 20*x*x + 40*x*x*x - 40*x*x*x*x + 16*x*x*x*x*x; }
		float easeInExpo(float x) { return x == 0 ? 0 : std::pow(1024.f, x-1); } // f(x) = 2^(10(x-1))
		float easeOutExpo(float x) { return x == 1 ? 1 : 1 - std::pow(0.0000765625f, x); }
		float easeInOutExpo(float x) { return x == 0 ? 0 : x == 1 ? 1 : (x < 0.5f ? std::pow(2.f, 20*x - 11) : 1 - std::pow(2.f, 9 - 20*x)); }
		float easeOutInExpo(float x) { return x == 0.5f ? 0.5f : (x < 0.5f ? (1 - std::pow(2.f, -20*x)) : std::pow(2.f, 20*x-20) + 1) / 2; }
		float easeInCirc(float x) { return 1 - std::sqrt(1 - x*x); }
		float easeOutCirc(float x) { return std::sqrt(1 - (x - 1)*(x - 1)); }
		float easeInOutCirc(float x) { return (x < 0.5f ? 1 - std::sqrt(1 - 4*x*x) : std::sqrt(-3 + 8*x - 4*x*x) + 1) / 2; }
		float easeOutInCirc(float x) { return x < 0.5f ? std::sqrt(x - x*x) : 1 - std::sqrt(x - x*x); }
		float easeInBack(float x) { return -1.70158*x*x + 2.70158*x*x*x; } // f(x) = (c+1)x^3-cx^2 | c = 1.70158
		float easeOutBack(float x) { x--; return 1 + 1.70158*x*x + 2.70158*x*x*x; } // c = 1.70158
		float easeInOutBack(float x) { return x < 0.5f ? (-5.189819f*x*x + 14.379638f*x*x*x) : (-8.189819f + 32.759276f*x - 37.949095f*x*x + 14.379638f*x*x*x); } // c = 1.70158 * 1.525
		float easeOutInBack(float x) { return x < 0.5f ? (5.5949095f*x - 16.379638f*x*x + 14.379638f*x*x*x) : (-2.5949095f + 15.9745475f*x - 26.759276f*x*x + 14.379638f*x*x*x); } // c = 1.70158 * 1.525
		float easeInElastic(float x) { return x == 0 ? 0 : -std::pow(1024.f, x - 1) * std::sin((x - 1) * (20 * NUM_PI / 3) - NUM_PI_2); } // f (x) = -2^(10(x-1)) * sin((x-1-c/4)*(2pi/c)) | c = 0.3
		float easeOutElastic(float x) { return x == 1 ? 1 : 1 + std::pow(0.0009765625f, x) * std::sin(-x * (20 * NUM_PI / 3) - NUM_PI_2); } // c = 0.3

		// *Modified* The original function just fades out instead of the correct behavior (flickering)
		float easeInOutElastic(float x) { return x == 0 ? 0 : x == 1 ? 1 : (x < 0.4f ? (-std::pow(2.f, 20*x-11) * std::sin((3*(x-1.2)) * (40 * NUM_PI / 9) - NUM_PI_2)) : 1 - std::pow(2.f, 9-20*x) * std::sin((3*((x*2)- 1.f)) * (40 * NUM_PI / 9) + NUM_PI_2)); } // c = 0.45
		float easeOutInElastic(float x) { return x == 0 ? 0 : x == 1 ? 1 : (x < 0.5f ? std::pow(2.f, -20*x) * std::sin(-x * (80 * NUM_PI / 9) - NUM_PI_2) : -std::pow(2.f, 20*(x-1)) * std::sin((x-1) * (80 * NUM_PI / 9) - NUM_PI_2)) / 2 + 0.5; } // c = 0.45

		EasingFunc getEaseFunc(Easing easing)
		{
			int baseType = static_cast<int>(Easing::TypeMask) & static_cast<int>(easing);
			int modifier = (static_cast<int>(Easing::TypeMask) << 4) & static_cast<int>(easing);
			switch ((Easing)baseType)
			{
				case Easing::Linear:
					return easeLinear;
				case Easing::Sine:
					switch ((Easing)modifier)
					{
						case Easing::In: return easeInSine;
						case Easing::Out: return easeOutSine;
						case Easing::InOut: return easeInOutSine;
						case Easing::OutIn: return easeOutInSine;
					}
					break;
				case Easing::Quad:
					switch ((Easing)modifier)
					{
						case Easing::In: return easeInQuad;
						case Easing::Out: return easeOutQuad;
						case Easing::InOut: return easeInOutQuad;
						case Easing::OutIn: return easeOutInQuad;
					}
					break;
				case Easing::Cubic:
					switch ((Easing)modifier)
					{
						case Easing::In: return easeInCubic;
						case Easing::Out: return easeOutCubic;
						case Easing::InOut: return easeInOutCubic;
						case Easing::OutIn: return easeOutInCubic;
					}
					break;
				case Easing::Quart:
					switch ((Easing)modifier)
					{
						case Easing::In: return easeInQuart;
						case Easing::Out: return easeOutQuart;
						case Easing::InOut: return easeInOutQuart;
						case Easing::OutIn: return easeOutInQuart;
					}
					break;
				case Easing::Quint:
					switch ((Easing)modifier)
					{
						case Easing::In: return easeInQuint;
						case Easing::Out: return easeOutQuint;
						case Easing::InOut: return easeInOutQuint;
						case Easing::OutIn: return easeOutInQuint;
					}
					break;
				case Easing::Expo:
					switch ((Easing)modifier)
					{
						case Easing::In: return easeInExpo;
						case Easing::Out: return easeOutExpo;
						case Easing::InOut: return easeInOutExpo;
						case Easing::OutIn: return easeOutInExpo;
					}
					break;
				case Easing::Circ:
					switch ((Easing)modifier)
					{
						case Easing::In: return easeInCirc;
						case Easing::Out: return easeOutCirc;
						case Easing::InOut: return easeInOutCirc;
						case Easing::OutIn: return easeOutInCirc;
					}
					break;
				case Easing::Back:
					switch ((Easing)modifier)
					{
						case Easing::In: return easeInBack;
						case Easing::Out: return easeOutBack;
						case Easing::InOut: return easeInOutBack;
						case Easing::OutIn: return easeOutInBack;
					}
					break; 
				case Easing::Elastic:
					switch ((Easing)modifier)
					{
						case Easing::In: return easeInElastic;
						case Easing::Out: return easeOutElastic;
						case Easing::InOut: return easeInOutElastic;
						case Easing::OutIn: return easeOutInElastic;
					}
					break;
			}
			assert(false && "Unsupported easing type");
			return easeLinear;
		}
	}
}