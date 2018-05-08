
#include "ShaderUtils.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <locale>
#include <codecvt>
#include <algorithm>

#ifdef _MSC_VER
#	include <Windows.h>
#	include <Shlobj.h>
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
	// TODO: platform specific code for Linux/macOS
	assert(false);
#endif

	// NOTE: defined in property sheet
	std::wstring shadersdir(MY_MEDIA_DIR);
	FILE* infile = nullptr;

	if (FALSE == PathResolve(&shadersdir[0], NULL, PRF_VERIFYEXISTS)) {
		printf("[ShaderUtils] Media directory not found\n");
		return 0;
	}

	shadersdir.resize(shadersdir.find_first_of(L'\0'));
	shadersdir += L"\\Shaders\\";
	
	std::wstring sourcefile = shadersdir + filename;
	_wfopen_s(&infile, sourcefile.c_str(), L"rb");

	if (infile == nullptr) {
		printf("[ShaderUtils] Could not open file '%S'\n", sourcefile.c_str());
		return 0;
	}

	fseek(infile, 0, SEEK_END);
	long length = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	std::string data(length, '\0');

	fread(&data[0], 1, length, infile);
	fclose(infile);

	// copy included files (non-recursive!!!)
	size_t pos = data.find("#include");

	while (pos != std::string::npos) {
		size_t start = data.find('\"', pos) + 1;
		size_t end = data.find('\"', start);

		std::string incfile(data.substr(start, end - start));

		// convert to wchar
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
		std::wstring wincfile = shadersdir + converter.from_bytes(incfile);
		std::string incdata;

		_wfopen_s(&infile, wincfile.c_str(), L"rb");

		if (infile == nullptr) {
			printf("[ShaderUtils] Could not open included file '%S'\n", wincfile.c_str());
			return 0;
		}

		fseek(infile, 0, SEEK_END);
		length = ftell(infile);
		fseek(infile, 0, SEEK_SET);

		incdata.resize(length, '\0');

		fread(&incdata[0], 1, length, infile);
		fclose(infile);

		data.replace(pos, end - pos + 1, incdata);
		pos = data.find("#include", end);
	}

	// create shader
	GLuint shader = glCreateShader(type);
	GLint success = GL_FALSE;
	const GLchar* sources[] = { data.c_str() };

	glShaderSource(shader, 1, sources, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (GL_TRUE != success) {
		char infolog[512];

		glGetShaderInfoLog(shader, 512, NULL, infolog);
		glDeleteShader(shader);

		printf("[ShaderUtils] Shader compilation failed:\n%s\n", infolog);
		return 0;
	}

	return shader;
}

GLuint CShaderUtils::AssembleProgram(CUniformTable& outtable, const wchar_t* vsfile, const wchar_t* gsfile, const wchar_t* fsfile)
{
	GLuint vertexshader = CShaderUtils::FindAndCompileShader(GL_VERTEX_SHADER, vsfile);
	GLuint geometryshader = 0;
	GLuint fragmentshader = CShaderUtils::FindAndCompileShader(GL_FRAGMENT_SHADER, fsfile);

	assert(vertexshader != 0);
	assert(fragmentshader != 0);

	if (gsfile) {
		geometryshader = CShaderUtils::FindAndCompileShader(GL_GEOMETRY_SHADER, gsfile);
		assert(geometryshader != 0);
	}

	GLuint program = glCreateProgram();

	glAttachShader(program, vertexshader);
	glAttachShader(program, fragmentshader);

	if (geometryshader != 0)
		glAttachShader(program, geometryshader);

	glLinkProgram(program);

	bool success = CShaderUtils::ValidateShaderProgram(program);
	assert(success);

	glBindFragDataLocation(program, 0, "my_FragColor0");
	glBindFragDataLocation(program, 1, "my_FragColor1");
	glBindFragDataLocation(program, 2, "my_FragColor2");
	glBindFragDataLocation(program, 3, "my_FragColor3");
	glLinkProgram(program);

	// delete shader objects
	glDetachShader(program, vertexshader);
	glDetachShader(program, fragmentshader);

	if (geometryshader != 0)
		glDetachShader(program, geometryshader);

	glDeleteShader(vertexshader);
	glDeleteShader(geometryshader);
	glDeleteShader(fragmentshader);

	CShaderUtils::QueryUniformLocations(outtable, program);
	return program;
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

void CShaderUtils::QueryUniformLocations(CUniformTable& outmap, GLuint program)
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

glm::vec4 CShaderUtils::sRGBToLinear(uint8_t red, uint8_t green, uint8_t blue)
{
	glm::vec4 ret;

	float lo_r = (float)red / 3294.6f;
	float lo_g = (float)green / 3294.6f;
	float lo_b = (float)blue / 3294.6f;

	float hi_r = powf((red / 255.0f + 0.055f) / 1.055f, 2.4f);
	float hi_g = powf((green / 255.0f + 0.055f) / 1.055f, 2.4f);
	float hi_b = powf((blue / 255.0f + 0.055f) / 1.055f, 2.4f);

	ret.r = (red < 10 ? lo_r : hi_r);
	ret.g = (green < 10 ? lo_g : hi_g);
	ret.b = (blue < 10 ? lo_b : hi_b);
	ret.a = 1;

	return ret;
}
