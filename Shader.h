#ifndef SHADER_H
#define SHADER_H

// Clase Shader (la misma idea que en las prácticas):
// lee el shader de vértices y el de fragmentos desde archivo,
// los compila, los enlaza en un programa y revisa errores.

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <GL/glew.h>

class Shader
{
public:
	GLuint Program;

	Shader(const GLchar *vertexPath, const GLchar *fragmentPath)
	{
		std::string vertexCode, fragmentCode;
		std::ifstream vShaderFile, fShaderFile;
		vShaderFile.exceptions(std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::badbit);
		try
		{
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);
			std::stringstream vShaderStream, fShaderStream;
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			vShaderFile.close();
			fShaderFile.close();
			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();
		}
		catch (std::ifstream::failure &e)
		{
			std::cout << "ERROR::SHADER::NO_SE_PUDO_LEER_EL_ARCHIVO" << std::endl;
		}
		const GLchar *vShaderCode = vertexCode.c_str();
		const GLchar *fShaderCode = fragmentCode.c_str();

		GLint success;
		GLchar infoLog[512];

		GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);
		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vertex, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::VERTEX::COMPILACION\n" << infoLog << std::endl;
		}

		GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);
		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragment, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::FRAGMENT::COMPILACION\n" << infoLog << std::endl;
		}

		this->Program = glCreateProgram();
		glAttachShader(this->Program, vertex);
		glAttachShader(this->Program, fragment);
		glLinkProgram(this->Program);
		glGetProgramiv(this->Program, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(this->Program, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::PROGRAMA::ENLAZADO\n" << infoLog << std::endl;
		}
		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}

	void Use()
	{
		glUseProgram(this->Program);
	}
};

#endif
