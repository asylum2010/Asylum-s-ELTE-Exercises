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

// Tasks:
// (0) [don't forget to give value to uniforms when you start using them]
// (1) modify 'pointlight.frag', so that it solves the radiance equation for point lights (BRDF can be Lambert)
// (2) add the specular term using the Cook-Torrance microfacet model (GGX, Shlick, Smith-Shlick are ok)

// (+) optimize the Cook-Torrance calculation (the Smith-Shlick function drops out its denominator)
// (+) optimize everything else that you can

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
	uint32_t		numSphereVertices;
	uint32_t		numSphereIndices;
	CUniformTable	uniformTable;	// locations of active uniforms

	// GL objects
	GLuint			framebuffer;	// to render in HDR
	GLuint			renderTarget0;	// RGBA16F
	GLuint			depthTarget;	// depth-stencil surface

	GLuint			sphereVBO;		// sphere vertex data
	GLuint			sphereIBO;		// sphere index data
	GLuint			sphereVAO;		// sphere input layout
	GLuint			screenQuadVAO;	// empty, but needed
	
	GLuint			pointlightPO;	// for spheres
	GLuint			tonemapPO;		// for tone mapping
};

#endif
