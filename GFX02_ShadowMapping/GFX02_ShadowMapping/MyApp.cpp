
#include <cmath>
#include <cstdio>
#include <vector>

#include <imgui/imgui.h>

#include "MyApp.h"

#define SHADOWMAP_SIZE	256

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
	shadowFramebuffer	= 0;

	renderTarget0		= 0;
	depthTarget			= 0;
	shadowMap			= 0;
	shadowDepth			= 0;
	blurredShadowMap	= 0;

	objectsVBO			= 0;
	objectsIBO			= 0;
	objectsVAO			= 0;
	
	screenQuadVAO		= 0;

	shadowMapPO			= 0;
	pointLightPO		= 0;
	blurPO				= 0;
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
	shadowMapPO = CShaderUtils::AssembleProgram(shadowMapTable, L"shadowmap.vert", 0, L"shadowmap.frag");
	pointLightPO = CShaderUtils::AssembleProgram(pointLightTable, L"pointlight_shadow.vert", 0, L"pointlight_shadow.frag");
	blurPO = CShaderUtils::AssembleProgram(blurTable, L"screenquad.vert", 0, L"boxblur.frag");
	tonemapPO = CShaderUtils::AssembleProgram(tonemapTable, L"screenquad.vert", 0, L"tonemap.frag");
	debugPO = CShaderUtils::AssembleProgram(debugTable, L"screenquad.vert", 0, L"screenquad.frag");

	// create shadow map and frame buffer (want to write linear depth so we waste some memory)
	glGenTextures(1, &shadowMap);
	glBindTexture(GL_TEXTURE_2D, shadowMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, SHADOWMAP_SIZE, SHADOWMAP_SIZE, 0, GL_RG, GL_FLOAT, NULL);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	float white[] = { 1, 1, 1, 1 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, white);

	glGenTextures(1, &blurredShadowMap);
	glBindTexture(GL_TEXTURE_2D, blurredShadowMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, SHADOWMAP_SIZE, SHADOWMAP_SIZE, 0, GL_RG, GL_FLOAT, NULL);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// same as a texture with private storage
	glGenRenderbuffers(1, &shadowDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, shadowDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SHADOWMAP_SIZE, SHADOWMAP_SIZE);

	glGenFramebuffers(1, &shadowFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMap, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, shadowDepth);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		assert(status == GL_FRAMEBUFFER_COMPLETE);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

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
	camera.SetClipPlanes(0.1f, 50.0f);
	camera.SetFov(glm::radians(60.0f));
	camera.SetDistance(8);
	camera.SetPosition(glm::vec3(0, 0, 0));
	camera.SetOrientation(glm::vec3(glm::radians(-45.0f), glm::radians(30.0f), 0));

	return true;
}

void CMyApp::Clean()
{
	glDeleteFramebuffers(1, &framebuffer);
	glDeleteFramebuffers(1, &shadowFramebuffer);
	glDeleteRenderbuffers(1, &shadowDepth);

	glDeleteTextures(1, &renderTarget0);
	glDeleteTextures(1, &depthTarget);
	glDeleteTextures(1, &shadowMap);
	glDeleteTextures(1, &blurredShadowMap);

	glDeleteProgram(shadowMapPO);
	glDeleteProgram(pointLightPO);
	glDeleteProgram(blurPO);
	glDeleteProgram(tonemapPO);
	glDeleteProgram(debugPO);

	glDeleteVertexArrays(1, &objectsVAO);
	glDeleteVertexArrays(1, &screenQuadVAO);

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

void CMyApp::BlurShadowMap()
{
	glm::vec2 texelsize(1.0f / SHADOWMAP_SIZE, 1.0f / SHADOWMAP_SIZE);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurredShadowMap, 0);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	{
		// setup graphics pipeline
		glUseProgram(blurPO);
		glViewport(0, 0, SHADOWMAP_SIZE, SHADOWMAP_SIZE);
		glDisable(GL_DEPTH_TEST);

		// update uniforms
		blurTable.SetInt("sampler0", 0);
		blurTable.SetVector2fv("texelSize", 1, &texelsize.x);

		glBindTexture(GL_TEXTURE_2D, shadowMap);

		// draw screen quad
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CMyApp::RenderShadowMap(ShadowProjData& outdata, const glm::vec3& lightpos)
{
	// ok, so we are in a little trouble here because point lights cast shadow in every direction...
	// ...for which we need a cubemap...
	// for now let's be lazy and choose (0, 0, 0) as lookAt point

	glm::vec2 clipplanes(0.2f, 30.0f); // TODO: fit to scene bounding box

	glm::mat4 view = glm::lookAtRH(lightpos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 proj = glm::perspectiveFovRH(glm::half_pi<float>(), (float)SHADOWMAP_SIZE, (float)SHADOWMAP_SIZE, clipplanes.x, clipplanes.y);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMap, 0);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	{
		// setup graphics pipeline
		glUseProgram(shadowMapPO);
		glBindVertexArray(objectsVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objectsIBO);
		glViewport(0, 0, SHADOWMAP_SIZE, SHADOWMAP_SIZE);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		//glCullFace(GL_FRONT);

		shadowMapTable.SetMatrix4fv("matView", 1, GL_FALSE, &view[0][0]);
		shadowMapTable.SetMatrix4fv("matProj", 1, GL_FALSE, &proj[0][0]);
		shadowMapTable.SetVector2fv("clipPlanes", 1, &clipplanes.x);

		RenderObjects(shadowMapTable);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	outdata.lightView = view;
	outdata.lightProj = proj;
	outdata.lightClip = clipplanes;

	// reset
	//glCullFace(GL_BACK);
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
	glm::vec2 shadowtexelsize(1.0f / SHADOWMAP_SIZE, 1.0f / SHADOWMAP_SIZE);

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

	// render pass 1 (shadow map for first light)
	ShadowProjData shadowdata;

	RenderShadowMap(shadowdata, lightpos1);
	BlurShadowMap();

	// render pass 2 (first light)
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glClearColor(0.0f, 0.0103f, 0.0707f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	{
		// setup graphics pipeline
		glUseProgram(pointLightPO);
		glBindVertexArray(objectsVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objectsIBO);
		glViewport(0, 0, windowWidth, windowHeight);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		// update common uniforms
		pointLightTable.SetMatrix4fv("matViewProj", 1, GL_FALSE, &viewproj[0][0]);
		pointLightTable.SetMatrix4fv("matLightView", 1, GL_FALSE, &shadowdata.lightView[0][0]);
		pointLightTable.SetMatrix4fv("matLightProj", 1, GL_FALSE, &shadowdata.lightProj[0][0]);

		pointLightTable.SetVector3fv("eyePos", 1, &eyepos.x);
		pointLightTable.SetVector2fv("lightClipPlanes", 1, &shadowdata.lightClip.x);
		pointLightTable.SetVector2fv("shadowTexelSize", 1, &shadowtexelsize.x);
		pointLightTable.SetInt("shadowMap", 0);

		//glBindTexture(GL_TEXTURE_2D, shadowMap);
		glBindTexture(GL_TEXTURE_2D, blurredShadowMap);

		// render objects
		pointLightTable.SetVector3fv("lightPos", 1, &lightpos1.x);
		pointLightTable.SetFloat("luminousFlux", 3200);	// ~50 W (fluorescent)
		
		RenderObjects(pointLightTable);
	}

	// render pass 3 (shadow map for second light)
	RenderShadowMap(shadowdata, lightpos2);
	BlurShadowMap();

	// render pass 4 (second light)
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	{
		// setup graphics pipeline
		glUseProgram(pointLightPO);
		glBindVertexArray(objectsVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objectsIBO);
		glViewport(0, 0, windowWidth, windowHeight);

		// enable additive blending
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);

		// setup depth-stencil state
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LEQUAL);

		// update common uniforms
		pointLightTable.SetMatrix4fv("matLightView", 1, GL_FALSE, &shadowdata.lightView[0][0]);
		pointLightTable.SetMatrix4fv("matLightProj", 1, GL_FALSE, &shadowdata.lightProj[0][0]);
		pointLightTable.SetVector2fv("lightClipPlanes", 1, &shadowdata.lightClip.x);

		//glBindTexture(GL_TEXTURE_2D, shadowMap);
		glBindTexture(GL_TEXTURE_2D, blurredShadowMap);

		// render objects
		pointLightTable.SetVector3fv("lightPos", 1, &lightpos2.x);
		pointLightTable.SetFloat("luminousFlux", 1000);
		
		RenderObjects(pointLightTable);
	}

	// must enable it for glClear (HUGE difference with Metal/Vulkan)
	glDepthMask(GL_TRUE);

	// render pass 5 (tone mapping)
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

	// render pass 6 (debug)
	if (debugMode) {
		// setup graphics pipeline
		glUseProgram(debugPO);
		glBindVertexArray(screenQuadVAO);
		glViewport(0, 0, 512, 512);

		// update uniforms
		debugTable.SetInt("sampler0", 0);
		glBindTexture(GL_TEXTURE_2D, shadowMap);
		//glBindTexture(GL_TEXTURE_2D, blurredShadowMap);

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
	glBindTexture(GL_TEXTURE_2D, renderTarget0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_HALF_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, depthTarget);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, windowWidth, windowHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
}
