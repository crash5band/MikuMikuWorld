#pragma once
#include <GLES3/gl3.h>
#include <filesystem>
#include <string>
#include <map>
#include "DirectXMath.h"

namespace MikuMikuWorld
{
	class Shader
	{
	private:
		unsigned int ID;
		unsigned int uloc;
		std::string name;
		std::map<std::string, GLint> locMap;

		void compile(const std::string& source);
		GLint getUniformLoc(const std::string& name);

	public:	
		Shader(const std::string& name, const std::string &source);
		~Shader();

		std::string getName() const;
		void use();

		void setBool(const std::string& name, bool value);
		void setInt(const std::string& name, int value);
		void setFloat(const std::string& name, float value);
		void setVec2(const std::string& name, DirectX::XMVECTOR v);
		void setVec3(const std::string& name, DirectX::XMVECTOR v);
		void setVec4(const std::string& name, DirectX::XMVECTOR v);
		void setMatrix4(const std::string& name, DirectX::XMMATRIX m);
	};
}