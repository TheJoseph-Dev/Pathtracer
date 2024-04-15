#ifndef SHADER_H
#define SHADER_H

//#include <stdio.h>
#include <iostream>

#include <glad/glad.h>
#include <glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

enum class ShaderType {
	VERTEX = 0,
	FRAGMENT = 1
};

struct ShadersData {
	std::string vertexSource;
	std::string fragmentSource;
};

class Shader {

private:
	unsigned int rendererID;

public:
	Shader() : rendererID(0){};
	Shader(const char* filepath);
	~Shader();

private:
	ShadersData Parse(const char* filepath);
	void Compile(ShadersData sources, unsigned int* shaders);
	void Create(ShadersData sources);


	inline unsigned int GetUniformLocation(std::string name) const { return glGetUniformLocation(this->rendererID, name.c_str()); }
public:
	void Bind() const;
	//void Unbind();

	void SetUniformMat4(std::string name, const glm::mat4& matrix) const {
		unsigned int location = GetUniformLocation(name);
		glUniformMatrix4fv(location, 1, GL_FALSE, &matrix[0][0]);
	}

	void SetUniform3f(std::string name, float f1, float f2, float f3) const {
		unsigned int location = GetUniformLocation(name);
		glUniform3f(location, f1, f2, f3);
	}

	void SetUniform2f(std::string name, float f1, float f2) const {
		unsigned int location = GetUniformLocation(name);
		glUniform2f(location, f1, f2);
	}

	void SetUniformFloat(std::string name, float value) const {
		unsigned int location = GetUniformLocation(name);
		glUniform1f(location, value);
	}

	void SetUniformInt(std::string name, int value) const {
		unsigned int location = GetUniformLocation(name);
		glUniform1i(location, value);
	}

	void SetUniformUInt(std::string name, unsigned int value) const {
		unsigned int location = GetUniformLocation(name);
		glUniform1ui(location, value);
	}
};


#endif // SHADER_H