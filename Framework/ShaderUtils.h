
#ifndef _SHADERUTILS_H_
#define _SHADERUTILS_H_

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <exception>
#include <cstdarg>

class CUniformTable : private std::unordered_map<std::string, GLint>
{
	typedef std::unordered_map<std::string, GLint> base_type;

public:
	using base_type::insert;
	using base_type::begin;
	using base_type::end;
	using base_type::count;
	using base_type::find;

	// NOTE: will throw exception when trying to access an invalid key
	mapped_type& operator[](key_type&& _Keyval) {
		// disable auto creation
		auto it = base_type::find(_Keyval);

		if (it == base_type::end())
			throw std::invalid_argument("Invalid uniform name!");

		return it->second;
	}

	// NOTE: these will silently fail
	void SetMatrix4fv(key_type&& _Keyval, GLsizei count, GLboolean transpose, const GLfloat* value) {
		auto it = base_type::find(_Keyval);

		if (it != base_type::end())
			glUniformMatrix4fv(it->second, count, transpose, value);
	}

	void SetVector3fv(key_type&& _Keyval, GLsizei count, const GLfloat* value) {
		auto it = base_type::find(_Keyval);

		if (it != base_type::end())
			glUniform3fv(it->second, count, value);
	}

	void SetVector4fv(key_type&& _Keyval, GLsizei count, const GLfloat* value) {
		auto it = base_type::find(_Keyval);

		if (it != base_type::end())
			glUniform4fv(it->second, count, value);
	}

	void SetFloat(key_type&& _Keyval, GLfloat v0) {
		auto it = base_type::find(_Keyval);

		if (it != base_type::end())
			glUniform1f(it->second, v0);
	}

	void SetInt(key_type&& _Keyval, GLint v0) {
		auto it = base_type::find(_Keyval);

		if (it != base_type::end())
			glUniform1i(it->second, v0);
	}
};

class CShaderUtils
{
private:
	CShaderUtils();
	~CShaderUtils();

public:
	static GLuint FindAndCompileShader(GLenum type, const wchar_t* filename);
	static GLuint AssembleProgram(CUniformTable& outtable, const wchar_t* vsfile, const wchar_t* gsfile, const wchar_t* fsfile);
	static bool ValidateShaderProgram(GLuint program);
	static void QueryUniformLocations(CUniformTable& outmap, GLuint program);
	static glm::vec4 sRGBToLinear(uint8_t red, uint8_t green, uint8_t blue);
};

#endif
