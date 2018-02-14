
#include "ShaderUtils.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <algorithm>

#ifdef _MSC_VER
#	include <Windows.h>
#endif

CShaderUtils::CShaderUtils()
{
}

CShaderUtils::~CShaderUtils()
{
}

GLuint CShaderUtils::FindAndCompileShader(GLenum type, const wchar_t* filename)
{
#ifndef _MSC_VER
	// TODO: platform specific code forLinux/macOS
	assert(false);
#endif

	// NOTE: defined in property sheet
	std::wstring projectdir(MY_PROJECT_DIR);
	std::wstring sourcefile(MY_PROJECT_DIR);
	FILE* infile = nullptr;

	sourcefile += filename;
	_wfopen_s(&infile, sourcefile.c_str(), L"rb");

	if (infile == nullptr) {
		printf("[ShaderUtils] Could not open file '%S'\n", sourcefile.c_str());
		return 0;
	}

	fseek(infile, 0, SEEK_END);
	long length = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	char* data = new char[length + 1];
	fread(data, 1, length, infile);
	data[length] = 0;

	GLuint shader = glCreateShader(type);
	GLint success = GL_FALSE;

	glShaderSource(shader, 1, &data, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (GL_TRUE != success) {
		char infolog[512];

		glGetShaderInfoLog(shader, 512, NULL, infolog);
		glDeleteShader(shader);

		printf("[ShaderUtils] Shader compilation failed:\n%s\n", infolog);
		return 0;
	}

	delete[] data;
	fclose(infile);

	return shader;
}

bool CShaderUtils::ValidateShaderProgram(GLuint program)
{
	GLint success = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (GL_TRUE != success) {
		char infolog[512];
		glGetProgramInfoLog(program, 512, NULL, infolog);

		printf("[ShaderUtils] Shader linkage failed:\n%s\n", infolog);
		return false;
	}

	return true;
}

void CShaderUtils::QueryUniformLocations(UniformTable& outmap, GLuint program)
{
	// TODO: uniform blocks
	assert(program != 0);

	GLint		count;
	GLenum		type;
	GLsizei		length;
	GLint		size, loc;
	GLchar		uniname[32];

	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);

	for (GLint i = 0; i < count; ++i) {
		memset(uniname, 0, sizeof(uniname));

		glGetActiveUniform(program, i, 32, &length, &size, &type, uniname);
		loc = glGetUniformLocation(program, uniname);

		// array uniforms sometimes contain []
		for (int j = 0; j < length; ++j) {
			if (uniname[j] == '[')
				uniname[j] = '\0';
		}

		// skip invalid uniforms
		if (loc == -1)
			continue;

		outmap.insert(std::make_pair(std::string(uniname), loc));
	}
}
