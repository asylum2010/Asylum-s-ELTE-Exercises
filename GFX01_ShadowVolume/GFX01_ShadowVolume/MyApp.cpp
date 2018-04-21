
#include <cmath>
#include <cstdio>
#include <vector>

#include <imgui/imgui.h>

#include "MyApp.h"

// this is cheating but otherwise we get nasty zfights
#define MANIFOLD_EPSILON	8e-3f

// tweakables
glm::vec4 basecolor1 = CShaderUtils::sRGBToLinear(0, 240, 240);
glm::vec4 basecolor2 = CShaderUtils::sRGBToLinear(255, 106, 0);
glm::vec4 basecolor3 = CShaderUtils::sRGBToLinear(0, 255, 0);

CMyApp::CMyApp(void)
{
	windowWidth			= 0;
	windowHeight		= 0;
	numSphereVertices	= 0;
	numSphereIndices	= 0;

	framebuffer			= 0;
	renderTarget0		= 0;
	depthTarget			= 0;
	
	manifoldsVBO		= 0;
	manifoldsIBO		= 0;
	manifoldsVAO		= 0;

	objectsVBO			= 0;
	objectsIBO			= 0;
	objectsVAO			= 0;
	
	screenQuadVAO		= 0;

	pointLightPO		= 0;
	zOnlyPO				= 0;
	extrudePO			= 0;
	tonemapPO			= 0;
}

CMyApp::~CMyApp(void)
{
}

bool CMyApp::Init()
{
	glClearColor(0.0f, 0.0103f, 0.0707f, 1.0f);
	glClearDepth(1.0f);
	glClearStencil(0);

	glEnable(GL_CULL_FACE);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glCullFace(GL_BACK);

	// create a box and a sphere
	{
		glGenBuffers(1, &objectsVBO);
		glGenBuffers(1, &objectsIBO);

		CGeometryUtils::NumVerticesIndicesSphere(numSphereVertices, numSphereIndices, 32, 32);

		glBindBuffer(GL_ARRAY_BUFFER, objectsVBO);
		glBufferData(GL_ARRAY_BUFFER, (24 + numSphereVertices) * sizeof(SCommonVertex), NULL, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objectsIBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (36 + numSphereIndices) * sizeof(uint32_t), NULL, GL_STATIC_DRAW);

		SCommonVertex* vdata = (SCommonVertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		uint32_t* idata = (uint32_t*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
		{
			CGeometryUtils::CreateBox(vdata, idata, 2, 2, 2);
			CGeometryUtils::CreateSphere(vdata + 24, idata + 36, 1.0f, 32, 32);
		}
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		glUnmapBuffer(GL_ARRAY_BUFFER);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		// create input layout
		glGenVertexArrays(1, &objectsVAO);
		glBindVertexArray(objectsVAO);
		{
			// NOTE: VBO can't be decoupled from VAO before GL 4.4
			glBindBuffer(GL_ARRAY_BUFFER, objectsVBO);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SCommonVertex), (const void*)0);

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SCommonVertex), (const void*)12);

			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(SCommonVertex), (const void*)20);
		}
		glBindVertexArray(0);
	}

	// do the same, but objects are now 2-manifolds
	{
		glGenBuffers(1, &manifoldsVBO);
		glGenBuffers(1, &manifoldsIBO);

		CManifoldUtils::NumVerticesIndices2MSphere(num2MSphereVertices, num2MSphereIndices, 32, 32);

		glBindBuffer(GL_ARRAY_BUFFER, manifoldsVBO);
		glBufferData(GL_ARRAY_BUFFER, (8 + num2MSphereVertices) * sizeof(SCommonVertex), NULL, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, manifoldsIBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (36 + num2MSphereIndices) * 3 * sizeof(uint32_t), NULL, GL_STATIC_DRAW);

		SPositionVertex* vdata = (SPositionVertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		uint32_t* idata = (uint32_t*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
		{
			uint32_t* baseindices = new uint32_t[36];

			CManifoldUtils::Create2MBox(vdata, baseindices, (2 - MANIFOLD_EPSILON), (2 - MANIFOLD_EPSILON), (2 - MANIFOLD_EPSILON));
			CManifoldUtils::GenerateGSAdjacency(idata, baseindices, 36);

			delete[] baseindices;
			baseindices = new uint32_t[num2MSphereIndices];

			CManifoldUtils::Create2MSphere(vdata + 8, baseindices, 1.0f - MANIFOLD_EPSILON, 32, 32);
			CManifoldUtils::GenerateGSAdjacency(idata + 36 * 3, baseindices, num2MSphereIndices);
	
			delete[] baseindices;
		}
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		glUnmapBuffer(GL_ARRAY_BUFFER);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		// create input layout
		glGenVertexArrays(1, &manifoldsVAO);
		glBindVertexArray(manifoldsVAO);
		{
			// NOTE: VBO can't be decoupled from VAO before GL 4.4
			glBindBuffer(GL_ARRAY_BUFFER, manifoldsVBO);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SPositionVertex), (const void*)0);
		}
		glBindVertexArray(0);
	}

	// create input layout for screenquad
	glGenVertexArrays(1, &screenQuadVAO);
	glBindVertexArray(screenQuadVAO);
	{
		// no input; will be generated in vertex shader
	}
	glBindVertexArray(0);

	// create programs
	pointLightPO = CShaderUtils::AssembleProgram(pointLightTable, L"pointlight.vert", 0, L"pointlight.frag");
	zOnlyPO = CShaderUtils::AssembleProgram(zOnlyTable, L"zonly.vert", 0, L"zonly.frag");
	extrudePO = CShaderUtils::AssembleProgram(extrudeTable, L"extrude.vert", L"extrude.geom", L"extrude.frag");
	tonemapPO = CShaderUtils::AssembleProgram(tonemapTable, L"screenquad.vert", 0, L"tonemap.frag");

	// create render targets (don't know size yet)
	glGenTextures(1, &renderTarget0);
	glBindTexture(GL_TEXTURE_2D, renderTarget0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 256, 256, 0, GL_RGBA, GL_HALF_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &depthTarget);
	glBindTexture(GL_TEXTURE_2D, depthTarget);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 256, 256, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// create framebuffer
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTarget0, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTarget, 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		assert(status == GL_FRAMEBUFFER_COMPLETE);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// setup camera
	camera.SetClipPlanes(0.1f, 30.0f);
	camera.SetFov(glm::radians(60.0f));
	camera.SetDistance(8);
	camera.SetPosition(glm::vec3(0, 0, 0));
	camera.SetOrientation(glm::vec3(glm::radians(-45.0f), glm::radians(30.0f), 0));

	return true;
}

void CMyApp::Clean()
{
	glDeleteFramebuffers(1, &framebuffer);
	glDeleteTextures(1, &renderTarget0);
	glDeleteTextures(1, &depthTarget);

	glDeleteProgram(pointLightPO);
	glDeleteProgram(zOnlyPO);
	glDeleteProgram(extrudePO);
	glDeleteProgram(tonemapPO);

	glDeleteVertexArrays(1, &manifoldsVAO);
	glDeleteVertexArrays(1, &objectsVAO);
	glDeleteVertexArrays(1, &screenQuadVAO);

	glDeleteBuffers(1, &manifoldsVBO);
	glDeleteBuffers(1, &manifoldsIBO);
	glDeleteBuffers(1, &objectsVBO);
	glDeleteBuffers(1, &objectsIBO);
}

void CMyApp::Update()
{
	static Uint32 last_time = SDL_GetTicks();
	float delta_time = (SDL_GetTicks() - last_time) / 1000.0f;

	// TODO: update physics with delta

	last_time = SDL_GetTicks();
}

void CMyApp::FillStencilBuffer(const glm::vec3& lightpos, const glm::mat4& viewproj)
{
	glm::mat4 world;

	glEnable(GL_DEPTH_CLAMP);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDisable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_STENCIL_TEST);
	glStencilMask(0xff);
	glStencilFunc(GL_ALWAYS, 0, 0xff);

	// fill the stencil buffer with Carmack's reverse
	glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
	glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);

	glUseProgram(extrudePO);
	glBindVertexArray(manifoldsVAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, manifoldsIBO);

	// NOTE: this must be in 'sync' with RenderObjects()
	// FYI: refactor it into a class [clean code exercise]

	extrudeTable.SetMatrix4fv("matViewProj", 1, GL_FALSE, &viewproj[0][0]);
	extrudeTable.SetVector3fv("lightPos", 1, &lightpos.x);

	// object 2
	world = glm::translate(glm::vec3(-2.0f, 0.2f, 2.5f));
	extrudeTable.SetMatrix4fv("matWorld", 1, GL_FALSE, &world[0][0]);

	glDrawElements(GL_TRIANGLES_ADJACENCY, 36 * 3, GL_UNSIGNED_INT, NULL);

	// object 3
	world = glm::translate(glm::vec3(1.5f, 0.2f, -2.0f));
	extrudeTable.SetMatrix4fv("matWorld", 1, GL_FALSE, &world[0][0]);

	glDrawElementsBaseVertex(GL_TRIANGLES_ADJACENCY, num2MSphereIndices * 3, GL_UNSIGNED_INT, (void*)(36 * 3 * sizeof(uint32_t)), 8);

	// reset
	glEnable(GL_CULL_FACE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_CLAMP);
}

void CMyApp::RenderObjects(CUniformTable& table)
{
	glm::mat4 world, worldinv;

	// object 1
	world = glm::translate(glm::vec3(0, -1, 0)) * glm::scale(glm::vec3(5.0f, 0.2f, 5.0f));
	worldinv = glm::inverse(world);

	table.SetMatrix4fv("matWorld", 1, GL_FALSE, &world[0][0]);
	table.SetMatrix4fv("matWorldInv", 1, GL_FALSE, &worldinv[0][0]);
	table.SetVector4fv("baseColor", 1, &basecolor1.x);
	table.SetFloat("roughness", 0.45f);

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);

	// object 2
	world = glm::translate(glm::vec3(-2.0f, 0.2f, 2.5f));
	worldinv = glm::inverse(world);

	table.SetMatrix4fv("matWorld", 1, GL_FALSE, &world[0][0]);
	table.SetMatrix4fv("matWorldInv", 1, GL_FALSE, &worldinv[0][0]);
	table.SetVector4fv("baseColor", 1, &basecolor2.x);
	table.SetFloat("roughness", 0.35f);

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);

	// object 3
	world = glm::translate(glm::vec3(1.5f, 0.2f, -2.0f));
	worldinv = glm::inverse(world);

	table.SetMatrix4fv("matWorld", 1, GL_FALSE, &world[0][0]);
	table.SetMatrix4fv("matWorldInv", 1, GL_FALSE, &worldinv[0][0]);
	table.SetVector4fv("baseColor", 1, &basecolor3.x);
	table.SetFloat("roughness", 0.15f);

	glDrawElementsBaseVertex(GL_TRIANGLES, numSphereIndices, GL_UNSIGNED_INT, (void*)(36 * sizeof(uint32_t)), 24);
};

void CMyApp::Render()
{
	assert(windowWidth > 0);

	glm::vec3 eyepos;
	glm::vec3 lightpos1, lightpos2;

	glm::mat4 world, view, proj;
	glm::mat4 worldinv;
	glm::mat4 viewproj;

	float time = SDL_GetTicks() / 1000.0f;

	// setup transforms
	camera.GetViewMatrixAndEyePosition(view, eyepos);
	camera.GetProjectionMatrix(proj);

	viewproj = proj * view;

	// moving light
	lightpos1.x = cosf(time) * 5;
	lightpos1.y = 4.0f;
	lightpos1.z = sinf(time) * 5;

	// static light
	lightpos2.x = -5;
	lightpos2.y = 8;
	lightpos2.z = 5;

	// render (sub)pass 1 (z-only, black)
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	{
		// setup graphics pipeline
		glUseProgram(zOnlyPO);
		glBindVertexArray(objectsVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objectsIBO);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glViewport(0, 0, windowWidth, windowHeight);

		zOnlyTable.SetMatrix4fv("matViewProj", 1, GL_FALSE, &viewproj[0][0]);
		RenderObjects(zOnlyTable);
	}

	// render (sub)pass 2 (first light)
	glClear(GL_STENCIL_BUFFER_BIT);
	{
		FillStencilBuffer(lightpos1, viewproj);

		// setup graphics pipeline
		glUseProgram(pointLightPO);
		glBindVertexArray(objectsVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objectsIBO);
		glViewport(0, 0, windowWidth, windowHeight);

		// enable additive blending
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);

		// setup depth-stencil state (pass where stencil is zero)
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LEQUAL);

		glEnable(GL_STENCIL_TEST);
		glStencilMask(0xff);
		glStencilFunc(GL_EQUAL, 0, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		// update common uniforms
		pointLightTable.SetMatrix4fv("matViewProj", 1, GL_FALSE, &viewproj[0][0]);
		pointLightTable.SetVector3fv("eyePos", 1, &eyepos.x);

		// render objects
		pointLightTable.SetVector3fv("lightPos", 1, &lightpos1.x);
		pointLightTable.SetFloat("luminousFlux", 3200);	// ~50 W (fluorescent)
		
		RenderObjects(pointLightTable);
	}

	// render (sub)pass 3 (second light)
	glClear(GL_STENCIL_BUFFER_BIT);
	{
		FillStencilBuffer(lightpos2, viewproj);

		// setup graphics pipeline
		glUseProgram(pointLightPO);
		glBindVertexArray(objectsVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objectsIBO);
		glViewport(0, 0, windowWidth, windowHeight);

		// enable additive blending
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);

		// setup depth-stencil state (pass where stencil is zero)
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LEQUAL);

		glEnable(GL_STENCIL_TEST);
		glStencilMask(0xff);
		glStencilFunc(GL_EQUAL, 0, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		// render objects
		pointLightTable.SetVector3fv("lightPos", 1, &lightpos2.x);
		pointLightTable.SetFloat("luminousFlux", 1000);
		
		RenderObjects(pointLightTable);
	}

	// must enable it for glClear (HUGE difference with Metal/Vulkan)
	glDepthMask(GL_TRUE);

	// render pass 4 (tone mapping)
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	{
		// setup graphics pipeline
		glUseProgram(tonemapPO);
		glBindVertexArray(screenQuadVAO);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_BLEND);
		glViewport(0, 0, windowWidth, windowHeight);

		// update uniforms
		tonemapTable.SetInt("sampler0", 0);
		glBindTexture(GL_TEXTURE_2D, renderTarget0);

		// draw screen quad
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
}

void CMyApp::KeyboardDown(SDL_KeyboardEvent& key)
{
}

void CMyApp::KeyboardUp(SDL_KeyboardEvent& key)
{
}

void CMyApp::MouseMove(SDL_MouseMotionEvent& mouse)
{
	if (mouse.state & SDL_BUTTON_LMASK) {
		camera.OrbitRight(glm::radians((float)mouse.xrel));
		camera.OrbitUp(glm::radians((float)mouse.yrel));
	} else if (mouse.state & SDL_BUTTON_MMASK) {
		float scale = camera.GetDistance() / 10.0f;
		float amount = 1e-3f + scale * (0.1f - 1e-3f);

		// NOTE: inverting on x is more natural (phones/tablets)
		camera.PanRight(mouse.xrel * -amount);
		camera.PanUp(mouse.yrel * amount);
	}
}

void CMyApp::MouseDown(SDL_MouseButtonEvent& mouse)
{
}

void CMyApp::MouseUp(SDL_MouseButtonEvent& mouse)
{
}

void CMyApp::MouseWheel(SDL_MouseWheelEvent& wheel)
{
}

void CMyApp::Resize(int newwidth, int newheight)
{
	windowWidth = newwidth;
	windowHeight = newheight;

	camera.SetAspect((float)windowWidth / (float)windowHeight);

	// reallocate render target storages
	glBindTexture(GL_TEXTURE_2D, renderTarget0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_HALF_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, depthTarget);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, windowWidth, windowHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
}
