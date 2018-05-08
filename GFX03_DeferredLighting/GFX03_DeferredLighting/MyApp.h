#ifndef _MYAPP_H_
#define _MYAPP_H_

#include <memory>
#include <unordered_map>

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>

#include "../../Framework/GeometryUtils.h"
#include "../../Framework/ShaderUtils.h"
#include "../../Framework/TextureUtils.h"
#include "../../Framework/ArcballCamera.h"

// Tasks:
// (1)
// (2)

// (+)

class CMyApp
{
public:
	CMyApp(void);
	~CMyApp(void);

	bool Init();
	void Clean();

	void Update();
	void Render();

	void KeyboardDown(SDL_KeyboardEvent&);
	void KeyboardUp(SDL_KeyboardEvent&);
	void MouseMove(SDL_MouseMotionEvent&);
	void MouseDown(SDL_MouseButtonEvent&);
	void MouseUp(SDL_MouseButtonEvent&);
	void MouseWheel(SDL_MouseWheelEvent&);
	void Resize(int, int);

private:
	struct ShadowProjData {
		glm::mat4 lightView;
		glm::mat4 lightProj;
		glm::vec2 lightClip;
	};

	void CreateAttachment(GLuint& target, GLint internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, bool reallocate);
	void RenderObjects(CUniformTable& table);

	int				windowWidth;
	int				windowHeight;
	bool			debugMode;

	uint32_t		numSphereVertices;
	uint32_t		numSphereIndices;

	CArcballCamera	camera;

	// locations of active uniforms
	CUniformTable	gBufferTable;
	CUniformTable	tonemapTable;
	CUniformTable	debugTable;

	// GL objects
	GLuint			framebuffer;	// to render in HDR
	GLuint			gBuffer;
	GLuint			accumBuffer;	// light accumulation targets (HDR)

	GLuint			gBufferNormals;	// RGBA8 (normal, roughness)
	GLuint			gBufferDepth;	// R32F
	GLuint			accumDiffuse;	// RGBA16F (diffuse illuminance)
	GLuint			accumSpecular;	// RGBA16F (specular illuminance)

	GLuint			renderTarget0;	// RGBA16F (combined output)
	GLuint			depthTarget;	// depth-stencil surface

	GLuint			objectsVBO;		// objects vertex data
	GLuint			objectsIBO;		// objects index data
	GLuint			objectsVAO;		// objects input layout

	GLuint			screenQuadVAO;	// empty, but needed

	GLuint			gBufferPO;		// to render g-buffer
	GLuint			tonemapPO;		// for tone mapping
	GLuint			debugPO;		// for debugging
};

#endif
