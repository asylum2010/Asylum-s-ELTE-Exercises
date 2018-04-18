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
#include "../../Framework/ManifoldUtils.h"
#include "../../Framework/ShaderUtils.h"
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
	GLuint AssembleProgram(CUniformTable& outtable, const wchar_t* vsfile, const wchar_t* gsfile, const wchar_t* fsfile);
	void FillStencilBuffer(const glm::vec3& lightpos, const glm::mat4& viewproj);
	void RenderObjects(CUniformTable& table);

	int				windowWidth;
	int				windowHeight;

	uint32_t		numSphereVertices;
	uint32_t		numSphereIndices;
	uint32_t		num2MSphereVertices;
	uint32_t		num2MSphereIndices;

	CArcballCamera	camera;

	// locations of active uniforms
	CUniformTable	pointLightTable;
	CUniformTable	zOnlyTable;
	CUniformTable	extrudeTable;
	CUniformTable	tonemapTable;

	// GL objects
	GLuint			framebuffer;	// to render in HDR
	GLuint			renderTarget0;	// RGBA16F
	GLuint			depthTarget;	// depth-stencil surface

	GLuint			manifoldsVBO;	// 2-manifolds vertex data
	GLuint			manifoldsIBO;	// 2-manifolds index data
	GLuint			manifoldsVAO;	// 2-manifolds input layout

	GLuint			objectsVBO;		// objects vertex data
	GLuint			objectsIBO;		// objects index data
	GLuint			objectsVAO;		// objects input layout

	GLuint			screenQuadVAO;	// empty, but needed

	GLuint			pointLightPO;	// for objects
	GLuint			zOnlyPO;		// for objects
	GLuint			extrudePO;		// for shadow volumes
	GLuint			tonemapPO;		// for tone mapping
};

#endif
