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

// Tasks:
// (1) complete the 'lightprobe.frag' shader using the provided irradiance and BRDF LUT textures
// (2) exchange the three spheres for a series of spheres which demonstrate varying roughness values

// (+) create your own preintegrated irradiance cubemaps (look through the code for URLs)

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

	// locations of active uniforms
	CUniformTable	skyUniTable;
	CUniformTable	lightProbeUniTable;
	CUniformTable	tonemapUniTable;

	// GL objects
	GLuint			framebuffer;	// to render in HDR
	GLuint			renderTarget0;	// RGBA16F
	GLuint			depthTarget;	// depth-stencil surface
	
	GLuint			skyTexture;		// for sky
	GLuint			skyDiffIrrad;	// preintegrated diffuse irradiance
	GLuint			skySpecIrrad;	// preintegrated specular irradiance
	GLuint			brdfLUT;		// preintegrated BRDF lookup texture

	GLuint			sphereVBO;		// sphere vertex data
	GLuint			sphereIBO;		// sphere index data
	GLuint			sphereVAO;		// sphere input layout
	GLuint			screenQuadVAO;	// empty, but needed
	
	GLuint			skyCubePO;		// for sky
	GLuint			lightProbePO;	// for preintegrated light probe
	GLuint			tonemapPO;		// for tone mapping
};

#endif
