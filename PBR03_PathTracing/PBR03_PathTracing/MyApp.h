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
	int				windowWidth;
	int				windowHeight;
	int				currSample;
	float			time;

	CArcballCamera	camera;

	// locations of active uniforms
	CUniformTable	pathTracerUniTable;
	CUniformTable	tonemapUniTable;

	// GL objects
	GLuint			framebuffer;		// to render in HDR
	GLuint			renderTargets[2];	// RGBA16F
	GLuint			depthTarget;		// not needed, but hey...
	
	GLuint			screenQuadVAO;		// empty, but needed

	GLuint			pathTracerPO;		// path tracer
	GLuint			tonemapPO;			// for tone mapping
};

#endif
