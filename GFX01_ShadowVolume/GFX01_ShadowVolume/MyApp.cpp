
#include <cmath>
#include <cstdio>
#include <vector>

#include <imgui/imgui.h>

#include "MyApp.h"

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
	tonemapPO			= 0;
}

CMyApp::~CMyApp(void)
{
}

bool CMyApp::Init()
{
	glClearColor(0.0f, 0.0103f, 0.0707f, 1.0f);
	glClearDepth(1.0f);

	glEnable(GL_CULL_FACE);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glCullFace(GL_BACK);

	// create a box and a sphere
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

	// create input layout for screenquad
	glGenVertexArrays(1, &screenQuadVAO);
	glBindVertexArray(screenQuadVAO);
	{
		// no input; will be generated in vertex shader
	}
	glBindVertexArray(0);

	// create point light program
	GLuint vertexshader = CShaderUtils::FindAndCompileShader(GL_VERTEX_SHADER, L"pointlight.vert");
	GLuint fragmentshader = CShaderUtils::FindAndCompileShader(GL_FRAGMENT_SHADER, L"pointlight.frag");

	assert(vertexshader != 0);
	assert(fragmentshader != 0);

	pointLightPO = glCreateProgram();

	glAttachShader(pointLightPO, vertexshader);
	glAttachShader(pointLightPO, fragmentshader);
	glLinkProgram(pointLightPO);

	bool success = CShaderUtils::ValidateShaderProgram(pointLightPO);
	assert(success);

	glBindFragDataLocation(pointLightPO, 0, "my_FragColor0");
	glLinkProgram(pointLightPO);

	// delete shader objects
	glDetachShader(pointLightPO, vertexshader);
	glDetachShader(pointLightPO, fragmentshader);

	glDeleteShader(vertexshader);
	glDeleteShader(fragmentshader);

	// create tonemap program
	vertexshader = CShaderUtils::FindAndCompileShader(GL_VERTEX_SHADER, L"screenquad.vert");
	fragmentshader = CShaderUtils::FindAndCompileShader(GL_FRAGMENT_SHADER, L"tonemap.frag");

	assert(vertexshader != 0);
	assert(fragmentshader != 0);

	tonemapPO = glCreateProgram();

	glAttachShader(tonemapPO, vertexshader);
	glAttachShader(tonemapPO, fragmentshader);
	glLinkProgram(tonemapPO);

	success = CShaderUtils::ValidateShaderProgram(tonemapPO);
	assert(success);

	glBindFragDataLocation(tonemapPO, 0, "my_FragColor0");
	glLinkProgram(tonemapPO);

	// delete shader objects
	glDetachShader(tonemapPO, vertexshader);
	glDetachShader(tonemapPO, fragmentshader);

	glDeleteShader(vertexshader);
	glDeleteShader(fragmentshader);

	// query all uniform locations and print them
	CShaderUtils::QueryUniformLocations(pointLightTable, pointLightPO);
	CShaderUtils::QueryUniformLocations(tonemapTable, tonemapPO);

	printf("\nActive uniforms (pointlight):\n");

	for (auto it : pointLightTable)
		printf("  %s (location = %d)\n", it.first.c_str(), it.second);

	printf("\nActive uniforms (tonemap):\n");

	for (auto it : tonemapTable)
		printf("  %s (location = %d)\n", it.first.c_str(), it.second);

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

void CMyApp::Render()
{
	assert(windowWidth > 0);

	// tweakables
	glm::vec3 eyepos;
	glm::vec3 lightpos1, lightpos2;
	glm::vec4 basecolor1 = CShaderUtils::sRGBToLinear(0, 240, 240);
	glm::vec4 basecolor2 = CShaderUtils::sRGBToLinear(255, 106, 0);
	glm::vec4 basecolor3 = CShaderUtils::sRGBToLinear(0, 255, 0);

	glm::mat4 world, view, proj;
	glm::mat4 worldinv;
	glm::mat4 viewproj;

	float time = SDL_GetTicks() / 1000.0f;

	// strong moving point light (experiment with curves on https://www.desmos.com/calculator)
	lightpos1.x = cosf(time) * 5;
	lightpos1.y = 3.0f;
	lightpos1.z = sinf(time) * 5;

	// a weaker static point light so we don't look retarded...
	lightpos2 = eyepos;

	// setup transforms
	camera.GetViewMatrixAndEyePosition(view, eyepos);
	camera.GetProjectionMatrix(proj);

	viewproj = proj * view;

	// render pass 1 (lit pbjects)
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	{
		// setup graphics pipeline
		glUseProgram(pointLightPO);
		glBindVertexArray(objectsVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objectsIBO);
		glEnable(GL_DEPTH_TEST);
		glViewport(0, 0, windowWidth, windowHeight);

		// update common uniforms
		pointLightTable.SetMatrix4fv("matViewProj", 1, GL_FALSE, &viewproj[0][0]);
		pointLightTable.SetVector3fv("eyePos", 1, &eyepos.x);

		auto render_objects = [&]() {
			// object 1
			world = glm::translate(glm::vec3(0, -1, 0)) * glm::scale(glm::vec3(5.0f, 0.2f, 5.0f));
			worldinv = glm::inverse(world);

			pointLightTable.SetMatrix4fv("matWorld", 1, GL_FALSE, &world[0][0]);
			pointLightTable.SetMatrix4fv("matWorldInv", 1, GL_FALSE, &worldinv[0][0]);
			pointLightTable.SetVector4fv("baseColor", 1, &basecolor1.x);
			pointLightTable.SetFloat("roughness", 0.45f);

			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);

			// object 2
			world = glm::translate(glm::vec3(-2.0f, 0, 2.5f));
			worldinv = glm::inverse(world);

			pointLightTable.SetMatrix4fv("matWorld", 1, GL_FALSE, &world[0][0]);
			pointLightTable.SetMatrix4fv("matWorldInv", 1, GL_FALSE, &worldinv[0][0]);
			pointLightTable.SetVector4fv("baseColor", 1, &basecolor2.x);
			pointLightTable.SetFloat("roughness", 0.35f);

			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);

			// object 3
			world = glm::translate(glm::vec3(1.5f, 0, -2.0f));
			worldinv = glm::inverse(world);

			pointLightTable.SetMatrix4fv("matWorld", 1, GL_FALSE, &world[0][0]);
			pointLightTable.SetMatrix4fv("matWorldInv", 1, GL_FALSE, &worldinv[0][0]);
			pointLightTable.SetVector4fv("baseColor", 1, &basecolor3.x);
			pointLightTable.SetFloat("roughness", 0.15f);

			glDrawElementsBaseVertex(GL_TRIANGLES, numSphereIndices, GL_UNSIGNED_INT, (void*)(36 * sizeof(uint32_t)), 24);
		};

		// render objects with first light
		glDepthFunc(GL_LESS);

		pointLightTable.SetVector3fv("lightPos", 1, &lightpos1.x);
		pointLightTable.SetFloat("luminousFlux", 3200);	// ~50 W (fluorescent)
		
		render_objects();

		// render objects with second light (additive blending)
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);

		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LEQUAL);

		pointLightTable.SetVector3fv("lightPos", 1, &lightpos2.x);
		pointLightTable.SetFloat("luminousFlux", 200);		// much weaker
		
		render_objects();

		glDisable(GL_BLEND);

		// must enable it for glClear (HUGE difference with Metal/Vulkan)
		glDepthMask(GL_TRUE);
	}

	// render pass 2 (tone mapping)
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	{
		// setup graphics pipeline
		glUseProgram(tonemapPO);
		glBindVertexArray(screenQuadVAO);
		glDisable(GL_DEPTH_TEST);
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
