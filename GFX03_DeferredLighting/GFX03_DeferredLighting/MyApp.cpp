
#include <cmath>
#include <cstdio>
#include <vector>

#include <imgui/imgui.h>

#include "MyApp.h"

// tweakables
glm::vec4 basecolor1 = CShaderUtils::sRGBToLinear(0, 240, 240);
glm::vec4 basecolor2 = CShaderUtils::sRGBToLinear(255, 106, 0);
glm::vec4 basecolor3 = CShaderUtils::sRGBToLinear(0, 255, 0);

CMyApp::CMyApp(void)
{
	windowWidth			= 0;
	windowHeight		= 0;
	debugMode			= false;
	numSphereVertices	= 0;
	numSphereIndices	= 0;

	framebuffer			= 0;
	gBuffer				= 0;
	accumBuffer			= 0;

	gBufferNormals		= 0;
	gBufferDepth		= 0;
	accumDiffuse		= 0;
	accumSpecular		= 0;

	renderTarget0		= 0;
	depthTarget			= 0;

	objectsVBO			= 0;
	objectsIBO			= 0;
	objectsVAO			= 0;
	
	screenQuadVAO		= 0;

	gBufferPO			= 0;
	tonemapPO			= 0;
	debugPO				= 0;
}

CMyApp::~CMyApp(void)
{
}

bool CMyApp::Init()
{
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

	// create input layout for screenquad
	glGenVertexArrays(1, &screenQuadVAO);
	glBindVertexArray(screenQuadVAO);
	{
		// no input; will be generated in vertex shader
	}
	glBindVertexArray(0);

	// create programs
	gBufferPO = CShaderUtils::AssembleProgram(gBufferTable, L"gbuffer.vert", 0, L"gbuffer.frag");
	tonemapPO = CShaderUtils::AssembleProgram(tonemapTable, L"screenquad.vert", 0, L"tonemap.frag");
	debugPO = CShaderUtils::AssembleProgram(debugTable, L"screenquad.vert", 0, L"screenquad.frag");

	// create g-buffer FBO (don't know size yet)
	CreateAttachment(gBufferNormals, GL_RGBA8, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, false);
	CreateAttachment(gBufferDepth, GL_R32F, 256, 256, GL_RED, GL_FLOAT, false);

	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gBufferNormals, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gBufferDepth, 0);

		// NOTE: depth will be attached later
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		assert(status == GL_FRAMEBUFFER_COMPLETE);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// create accumulation FBO (don't know size yet)

	// create combined output FBO (don't know size yet)
	CreateAttachment(renderTarget0, GL_RGBA16F, 256, 256, GL_RGBA, GL_HALF_FLOAT, false);
	CreateAttachment(depthTarget, GL_DEPTH24_STENCIL8, 256, 256, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, false);

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
	camera.SetClipPlanes(0.1f, 50.0f);
	camera.SetFov(glm::radians(60.0f));
	camera.SetDistance(8);
	camera.SetPosition(glm::vec3(0, 0, 0));
	camera.SetOrientation(glm::vec3(glm::radians(-45.0f), glm::radians(30.0f), 0));

	return true;
}

void CMyApp::CreateAttachment(GLuint& target, GLint internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, bool reallocate)
{
	if (reallocate) {
		assert(target != 0);

		glBindTexture(GL_TEXTURE_2D, target);
		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, NULL);
	} else {
		assert(target == 0);

		glGenTextures(1, &target);
		glBindTexture(GL_TEXTURE_2D, target);
		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
}

void CMyApp::Clean()
{
	glDeleteFramebuffers(1, &framebuffer);
	glDeleteFramebuffers(1, &gBuffer);

	glDeleteTextures(1, &gBufferNormals);
	glDeleteTextures(1, &gBufferDepth);
	glDeleteTextures(1, &accumDiffuse);
	glDeleteTextures(1, &accumSpecular);

	glDeleteTextures(1, &renderTarget0);
	glDeleteTextures(1, &depthTarget);

	glDeleteProgram(gBufferPO);
	glDeleteProgram(tonemapPO);
	glDeleteProgram(debugPO);

	glDeleteVertexArrays(1, &objectsVAO);
	glDeleteVertexArrays(1, &screenQuadVAO);

	glDeleteBuffers(1, &objectsVBO);
	glDeleteBuffers(1, &objectsIBO);

	CTextureUtils::Shutdown();
}

void CMyApp::Update()
{
	static Uint32 last_time = SDL_GetTicks();
	float delta_time = (SDL_GetTicks() - last_time) / 1000.0f;

	// TODO: update physics with delta

	last_time = SDL_GetTicks();
}

void CMyApp::RenderObjects(CUniformTable& table)
{
	glm::mat4 world, worldinv;

	// object 1
	world = glm::translate(glm::vec3(0, -1, 0)) * glm::scale(glm::vec3(15.0f, 0.2f, 15.0f));
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

	// render pass 1 (fill g-buffer)
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTarget, 0);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	{
		// setup graphics pipeline
		glUseProgram(gBufferPO);
		glBindVertexArray(objectsVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objectsIBO);
		
		glViewport(0, 0, windowWidth, windowHeight);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		// update global uniforms
		gBufferTable.SetMatrix4fv("matViewProj", 1, GL_FALSE, &viewproj[0][0]);

		// render
		RenderObjects(gBufferTable);
	}

	// render pass 2 (light accumulation)
	glBindFramebuffer(GL_FRAMEBUFFER, accumBuffer);
	{
		// ...
	}

	// render pass 3 (material pass)
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTarget, 0);
	{
		// ...
	}

	// must enable it for glClear (HUGE difference with Metal/Vulkan)
	glDepthMask(GL_TRUE);

	// render pass 4 (tone mapping)
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.0f, 0.0103f, 0.0707f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	{
		// setup graphics pipeline
		glUseProgram(tonemapPO);
		glBindVertexArray(screenQuadVAO);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glViewport(0, 0, windowWidth, windowHeight);

		// update uniforms
		tonemapTable.SetInt("sampler0", 0);
		glBindTexture(GL_TEXTURE_2D, renderTarget0);

		// draw screen quad
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	// render pass 5 (debug)
	if (debugMode) {
		GLsizei height = 256;
		GLsizei width = (GLsizei)((height * windowWidth) / windowHeight);

		// setup graphics pipeline
		glUseProgram(debugPO);
		glBindVertexArray(screenQuadVAO);
		glViewport(0, 0, width, height);

		// update uniforms
		debugTable.SetInt("sampler0", 0);
		glBindTexture(GL_TEXTURE_2D, gBufferNormals);

		// draw screen quad
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
}

void CMyApp::KeyboardDown(SDL_KeyboardEvent& key)
{
}

void CMyApp::KeyboardUp(SDL_KeyboardEvent& key)
{
	if (key.keysym.sym == SDLK_d)
		debugMode = !debugMode;
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
	CreateAttachment(gBufferNormals, GL_RGBA8, windowWidth, windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, true);
	CreateAttachment(gBufferDepth, GL_R32F, windowWidth, windowHeight, GL_RED, GL_FLOAT, true);

	CreateAttachment(renderTarget0, GL_RGBA16F, windowWidth, windowHeight, GL_RGBA, GL_HALF_FLOAT, true);
	CreateAttachment(depthTarget, GL_DEPTH24_STENCIL8, windowWidth, windowHeight, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, true);
}
