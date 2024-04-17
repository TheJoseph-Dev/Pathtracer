
#include "Shader.h"

#include <fstream> // file stream
#include <sstream> // string stream
#include <ostream>

#include "Source/Utils.h"

Shader::Shader(const char* filepath) : rendererID(0) {
	ShadersData sources = Parse(filepath);
	Create(sources);
}
Shader::~Shader() {}

ShadersData Shader::Parse(const char* filepath) {
	std::ifstream stream = std::ifstream(filepath);
	print(filepath << "\n\n");

	if (!stream) { print("ERROR: Shader got a NULL directory"); return {}; }

	std::string line;
	std::stringstream shaderStream[2];

	const std::string vertexKeyword = "#VERTEX_SHADER";
	const std::string fragKeyword = "#FRAGMENT_SHADER";
	ShaderType type = ShaderType::VERTEX;
	bool foundShader = false;

	while (getline(stream, line)) {
		if (line.find(vertexKeyword) != std::string::npos) { foundShader = true; continue; } // Found
		if (line.find(fragKeyword) != std::string::npos) { foundShader = true; type = ShaderType::FRAGMENT; continue; }
		
		if (foundShader == false) { continue; }

		shaderStream[(int)type] << line << "\n";
		//print(line);
	}


	return { shaderStream[0].str(), shaderStream[1].str() };
}

void StateHandle(unsigned int shaderID, ShaderType type) {
	int  success;
	char infoLog[512];
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
	std::string errorLocation = type == ShaderType::VERTEX ? "VERTEX" : "FRAGMENT";
	if (!success)
	{
		glGetShaderInfoLog(shaderID, 512, NULL, infoLog);
		print("ERROR::SHADER::" << errorLocation << "::COMPILATION_FAILED\n" << infoLog << std::endl);
	}
}

void Shader::Compile(ShadersData sources, unsigned int* shaders) {
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char* vsSource = sources.vertexSource.c_str();
	const char* fsSource = sources.fragmentSource.c_str();
	glShaderSource(vertexShader, 1, &vsSource, NULL);
	glShaderSource(fragmentShader, 1, &fsSource, NULL);
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);

	StateHandle(vertexShader, ShaderType::VERTEX);
	StateHandle(fragmentShader, ShaderType::FRAGMENT);

	glAttachShader(rendererID, vertexShader);
	glAttachShader(rendererID, fragmentShader);

	*shaders = vertexShader;
	*(shaders+1) = fragmentShader;
}

void Shader::Create(ShadersData sources) {
	this->rendererID = glCreateProgram();
	
	unsigned int shaders[2];
	Compile(sources, shaders);

	unsigned int vs = *(shaders);
	unsigned int fs = *(shaders+1);
	glLinkProgram(rendererID);
	glValidateProgram(rendererID);
	glDeleteShader(vs);
	glDeleteShader(fs);
}

void Shader::Bind() const {
	glUseProgram(this->rendererID);
}


// SSBO

SSBO::SSBO() {
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
};

SSBO::~SSBO() {};

void SSBO::Bind(unsigned int bind) { glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bind, buffer); }
void SSBO::SendData(uint32_t size, void* data) { glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_STATIC_DRAW); }
void SSBO::Unbind() { glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); }