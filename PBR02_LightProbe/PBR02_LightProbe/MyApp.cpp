
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
	sphereVBO			= 0;
	sphereIBO			= 0;
	sphereVAO			= 0;
	screenQuadVAO		= 0;
	skyCubePO			= 0;
	lightProbePO		= 0;
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

	// create one sphere
	glGenBuffers(1, &sphereVBO);
	glGenBuffers(1, &sphereIBO);

	CGeometryUtils::NumVerticesIndicesSphere(numSphereVertices, numSphereIndices, 32, 32);

	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
	glBufferData(GL_ARRAY_BUFFER, numSphereVertices * sizeof(SCommonVertex), NULL, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numSphereIndices * sizeof(uint32_t), NULL, GL_STATIC_DRAW);

	SCommonVertex* vdata = (SCommonVertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	uint32_t* idata = (uint32_t*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
	{
		CGeometryUtils::CreateSphere(vdata, idata, 1.0f, 32, 32);
	}
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
	glUnmapBuffer(GL_ARRAY_BUFFER);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// create input layout for sphere
	glGenVertexArrays(1, &sphereVAO);
	glBindVertexArray(sphereVAO);
	{
		// NOTE: VBO can't be decoupled from VAO before GL 4.4
		glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);

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

	// create light probe program
	GLuint vertexshader = CShaderUtils::FindAndCompileShader(GL_VERTEX_SHADER, L"lightprobe.vert");
	GLuint fragmentshader = CShaderUtils::FindAndCompileShader(GL_FRAGMENT_SHADER, L"lightprobe.frag");

	assert(vertexshader != 0);
	assert(fragmentshader != 0);

	lightProbePO = glCreateProgram();

	glAttachShader(lightProbePO, vertexshader);
	glAttachShader(lightProbePO, fragmentshader);
	glLinkProgram(lightProbePO);

	bool success = CShaderUtils::ValidateShaderProgram(lightProbePO);
	assert(success);

	glBindFragDataLocation(lightProbePO, 0, "my_FragColor0");
	glLinkProgram(lightProbePO);

	// delete shader objects
	glDetachShader(lightProbePO, vertexshader);
	glDetachShader(lightProbePO, fragmentshader);

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
	//CShaderUtils::QueryUniformLocations(uniformTable, skyCubePO);
	CShaderUtils::QueryUniformLocations(uniformTable, lightProbePO);
	CShaderUtils::QueryUniformLocations(uniformTable, tonemapPO);

	printf("\nList of active uniforms:\n");

	for (auto it : uniformTable)
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

	return true;
}

void CMyApp::Clean()
{
	glDeleteFramebuffers(1, &framebuffer);
	glDeleteTextures(1, &renderTarget0);
	glDeleteTextures(1, &depthTarget);
	glDeleteProgram(skyCubePO);
	glDeleteProgram(lightProbePO);
	glDeleteProgram(tonemapPO);
	glDeleteVertexArrays(1, &sphereVAO);
	glDeleteVertexArrays(1, &screenQuadVAO);
	glDeleteBuffers(1, &sphereVBO);
	glDeleteBuffers(1, &sphereIBO);
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
	glm::vec3 eyepos(0, 0, 5);
	glm::vec4 basecolor1 = CShaderUtils::sRGBToLinear(0, 240, 240);
	glm::vec4 basecolor2 = CShaderUtils::sRGBToLinear(255, 106, 0);
	glm::vec4 basecolor3 = CShaderUtils::sRGBToLinear(0, 255, 0);

	glm::mat4 world, view, proj;
	glm::mat4 worldinv;
	glm::mat4 viewproj;

	float time = SDL_GetTicks() / 1000.0f;

	// setup transforms
	view = glm::lookAtRH(eyepos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	proj = glm::perspectiveFovRH<float>(glm::radians(80.0f), (float)windowWidth, (float)windowHeight, 0.1f, 30.0f);

	viewproj = proj * view;
	worldinv = glm::inverse(world);

	// render pass 1 (lit spheres)
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	{
		// setup graphics pipeline
		glUseProgram(lightProbePO);
		glBindVertexArray(sphereVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereIBO);
		glEnable(GL_DEPTH_TEST);
		glViewport(0, 0, windowWidth, windowHeight);

		// update common uniforms
		uniformTable.SetMatrix4fv("matViewProj", 1, GL_FALSE, &viewproj[0][0]);
		uniformTable.SetVector3fv("eyePos", 1, &eyepos.x);

		// sphere 1
		world = glm::translate(glm::vec3(3, 0, -2));

		uniformTable.SetMatrix4fv("matWorld", 1, GL_FALSE, &world[0][0]);
		uniformTable.SetMatrix4fv("matWorldInv", 1, GL_FALSE, &worldinv[0][0]);
		uniformTable.SetVector4fv("baseColor", 1, &basecolor1.x);
		uniformTable.SetFloat("roughness", 0.2f);

		glDrawElements(GL_TRIANGLES, numSphereIndices, GL_UNSIGNED_INT, NULL);

		// sphere 2
		world = glm::translate(glm::vec3(-3, -1, 0));

		uniformTable.SetMatrix4fv("matWorld", 1, GL_FALSE, &world[0][0]);
		uniformTable.SetMatrix4fv("matWorldInv", 1, GL_FALSE, &worldinv[0][0]);
		uniformTable.SetVector4fv("baseColor", 1, &basecolor2.x);
		uniformTable.SetFloat("roughness", 0.5f);

		glDrawElements(GL_TRIANGLES, numSphereIndices, GL_UNSIGNED_INT, NULL);

		// sphere 3
		world = glm::translate(glm::vec3(-1, 1, -2));

		uniformTable.SetMatrix4fv("matWorld", 1, GL_FALSE, &world[0][0]);
		uniformTable.SetMatrix4fv("matWorldInv", 1, GL_FALSE, &worldinv[0][0]);
		uniformTable.SetVector4fv("baseColor", 1, &basecolor3.x);
		uniformTable.SetFloat("roughness", 0.7f);

		glDrawElements(GL_TRIANGLES, numSphereIndices, GL_UNSIGNED_INT, NULL);
	}

	// render pass 2 (sky dome)


	// render pass 3 (tone mapping)
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	{
		// setup graphics pipeline
		glUseProgram(tonemapPO);
		glBindVertexArray(screenQuadVAO);
		glDisable(GL_DEPTH_TEST);
		glViewport(0, 0, windowWidth, windowHeight);

		// update uniforms
		uniformTable.SetInt("sampler0", 0);
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

	// reallocate render target storages
	glBindTexture(GL_TEXTURE_2D, renderTarget0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowWidth, windowHeight, 0, GL_RGBA, GL_HALF_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, depthTarget);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, windowWidth, windowHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
}