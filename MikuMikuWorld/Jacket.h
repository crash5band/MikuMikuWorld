#pragma once
#include "Rendering/Texture.h"
#include <memory>
#include <string>

namespace MikuMikuWorld
{
	class Jacket
	{
	private:
		std::string filename;
		std::unique_ptr<Texture> texture;

	public:
		Jacket();

		void load(const std::string& filename);
		void draw();
		void clear();

		const std::string& getFilename() const;
		int getTexID() const;
	};
}