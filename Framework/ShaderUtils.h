
#ifndef _SHADERUTILS_H_
#define _SHADERUTILS_H_

#include <GL/glew.h>
#include <unordered_map>
#include <exception>

// NOTE: will throw exception if trying to access an invalid key
class UniformTable : private std::unordered_map<std::string, GLint>
{
	typedef std::unordered_map<std::string, GLint> base_type;

public:
	using base_type::insert;
	using base_type::begin;
	using base_type::end;
	using base_type::count;
	using base_type::find;

	mapped_type& operator[](key_type&& _Keyval) {
		// disable auto creation
		if (count(_Keyval) == 0)
			throw std::invalid_argument("Invalid uniform name!");

		return base_type::operator[](_Keyval);
	}
};

class CShaderUtils
{
private:
	CShaderUtils();
	~CShaderUtils();

public:
	static GLuint FindAndCompileShader(GLenum type, const wchar_t* filename);
	static bool ValidateShaderProgram(GLuint program);
	static void QueryUniformLocations(UniformTable& outmap, GLuint program);
};

#endif
