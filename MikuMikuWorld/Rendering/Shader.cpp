#include "../IO.h"
#include "Shader.h"
#include <sstream>
#include <fstream>
#include <iostream>

using namespace IO;

namespace MikuMikuWorld
{
	Shader::Shader(const std::string& name, const std::string& source)
	{
		this->name = name;
		compile(source);
	}

	Shader::~Shader()
	{
		glDeleteProgram(ID);
	}

	std::string Shader::getName() const
	{
		return name;
	}

	void Shader::compile(const std::string& source)
	{
		std::wstring wSource = mbToWideStr(source);
		std::string vertexCode, fragmentCode;
		std::ifstream vertexFile, fragmentFile;
		vertexFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fragmentFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			vertexFile.open(wSource + L".vert");
			fragmentFile.open(wSource + L".frag");

			std::stringstream vertexStream, fragmentStream;

			vertexStream << vertexFile.rdbuf();
			fragmentStream << fragmentFile.rdbuf();

			vertexFile.close();
			fragmentFile.close();

			vertexCode = vertexStream.str();
			fragmentCode = fragmentStream.str();
		}
		catch (std::ifstream::failure e)
		{
			std::cout << "SHADER::COMPILE: failed to read shader source\n" << e.what() << '\n';
		}

		const char* vShaderCode = vertexCode.c_str();
		const char* fShaderCode = fragmentCode.c_str();

		unsigned int vertex, fragment;
		int success;
		char log[512];

		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);

		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vertex, 512, NULL, log);
			std::cout << "SHADER::VERTEX::COMPILATION_FAILED\n";
			std::cout << log;
		}

		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);

		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragment, 512, NULL, log);
			std::cout << "SHADER::FRAGMENT::COMPILATION_FAILED\n";
			std::cout << log;
		}

		ID = glCreateProgram();
		glAttachShader(ID, vertex);
		glAttachShader(ID, fragment);
		glLinkProgram(ID);

		glGetProgramiv(ID, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(ID, 512, NULL, log);
			std::cout << "SHADER::PROGRAM::LINKING_FAILED\n";
			std::cout << log;
		}

		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}

	GLint Shader::getUniformLoc(const std::string& name)
	{
		auto it = locMap.find(name);
		if (it == locMap.end())
		{
			GLint loc = glGetUniformLocation(ID, name.c_str());
			locMap.insert(std::pair<std::string, GLint>(name, loc));
			return loc;
		}
		else
		{
			return it->second;
		}
	}

	void Shader::use()
	{
		glUseProgram(ID);
	}

	void Shader::setBool(const std::string& name, bool value)
	{
		glUniform1i(getUniformLoc(name), (int)value);
	}

	void Shader::setInt(const std::string& name, int value)
	{
		glUniform1i(getUniformLoc(name), value);
	}

	void Shader::setFloat(const std::string& name, float value)
	{
		glUniform1f(getUniformLoc(name), value);
	}

	void Shader::setVec2(const std::string& name, DirectX::XMVECTOR value)
	{
		glUniform2fv(getUniformLoc(name), 1, (GLfloat*)&value);
	}

	void Shader::setVec3(const std::string& name, DirectX::XMVECTOR value)
	{
		glUniform3fv(getUniformLoc(name), 1, (GLfloat*)&value);
	}

	void Shader::setVec4(const std::string& name, DirectX::XMVECTOR value)
	{
		glUniform4fv(getUniformLoc(name), 1, (GLfloat*)&value);
	}

	void Shader::setMatrix4(const std::string& name, DirectX::XMMATRIX value)
	{
		glUniformMatrix4fv(getUniformLoc(name), 1, GL_FALSE, (GLfloat*)&value.r->m128_f32[0]);
	}

}
