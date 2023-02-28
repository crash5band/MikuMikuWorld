#pragma once

namespace MikuMikuWorld
{
	extern int nextSkillID;

	struct SkillTrigger
	{
		int ID;
		int tick;
	};

	struct Fever
	{
		int startTick;
		int endTick;
	};

	struct HiSpeedChange
	{
		int tick;
		float speed;
	};
}