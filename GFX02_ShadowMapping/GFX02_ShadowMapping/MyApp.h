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
// (1) modify 'shadowmap.frag' so that it outputs linear depth
// (2) modify 'pointlight_shadow.frag' so that it implements the basic shadow test
// (3) implement Chabychev's inequality for variance shadows

// (+) implement irregular PCF using the provided 'pcfnoise.bmp' texture

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

	void BlurShadowMap();
	void RenderShadowMap(ShadowProjData& outdata, const glm::vec3& lightpos);
	void RenderObjects(CUniformTable& table);

	int				windowWidth;
	int				windowHeight;
	bool			debugMode;

	uint32_t		numSphereVertices;
	uint32_t		numSphereIndices;
	uint32_t		num2MSphereVertices;
	uint32_t		num2MSphereIndices;

	CArcballCamera	camera;

	// locations of active uniforms
	CUniformTable	shadowMapTable;
	CUniformTable	pointLightTable;
	CUniformTable	blurTable;
	CUniformTable	tonemapTable;
	CUniformTable	debugTable;

	// GL objects
	GLuint			framebuffer;	// to render in HDR
	GLuint			shadowFramebuffer;

	GLuint			renderTarget0;	// RGBA16F
	GLuint			depthTarget;	// depth-stencil surface
	GLuint			shadowMap;		// RG32F
	GLuint			shadowDepth;	// lazy
	GLuint			blurredShadowMap;

	GLuint			objectsVBO;		// objects vertex data
	GLuint			objectsIBO;		// objects index data
	GLuint			objectsVAO;		// objects input layout

	GLuint			screenQuadVAO;	// empty, but needed

	GLuint			shadowMapPO;	// for shadow map
	GLuint			pointLightPO;	// for objects
	GLuint			blurPO;			// for blur
	GLuint			tonemapPO;		// for tone mapping
	GLuint			debugPO;		// for debugging
};

#endif
