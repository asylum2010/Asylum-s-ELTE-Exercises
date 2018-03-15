
#include <cmath>
#include <cstdio>
#include <vector>

#include <imgui/imgui.h>

#include "MyApp.h"

CMyApp::CMyApp(void)
{
	windowWidth			= 0;
	windowHeight		= 0;
	currSample			= 0;
	time				= 0;

	framebuffer			= 0;
	renderTargets[0]	= 0;
	renderTargets[1]	= 0;
	depthTarget			= 0;
	
	screenQuadVAO		= 0;

	pathTracerPO		= 0;
	tonemapPO			= 0;

	// Example:
	// Monte Carlo integrate sin(x) on [0, pi] with p(x) = 1 / pi (uniform distribution)

	auto randomFloat = []() -> float {
		return (rand() & 32767) / 32767.0f;
	};

	// P(x) = \int_0^x p(x) dx = \int_0^x dx / pi = x / pi
	// P^-1(x) = pi * x
	int N = 1024;
	float estimate = 0;

	for (int k = 0; k < N; ++k) {
		float xi = randomFloat();
		float x_k = xi * glm::pi<float>();

		// PDF = 1 / pi
		estimate += glm::pi<float>() * sinf(x_k);
	}

	estimate /= N;
	printf("Estimate: %.6f\n", estimate);
}

CMyApp::~CMyApp(void)
{
}

bool CMyApp::Init()
{
	glClearColor(0, 0, 0, 1);
	glClearDepth(1);

	glEnable(GL_CULL_FACE);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glCullFace(GL_BACK);

	// create input layout for screenquad
	glGenVertexArrays(1, &screenQuadVAO);
	glBindVertexArray(screenQuadVAO);
	{
		// no input; will be generated in vertex shader
	}
	glBindVertexArray(0);

	// create path tracer program
	GLuint vertexshader = CShaderUtils::FindAndCompileShader(GL_VERTEX_SHADER, L"screenquad.vert");
	GLuint fragmentshader = CShaderUtils::FindAndCompileShader(GL_FRAGMENT_SHADER, L"pathtracer.frag");

	assert(vertexshader != 0);
	assert(fragmentshader != 0);

	pathTracerPO = glCreateProgram();

	glAttachShader(pathTracerPO, vertexshader);
	glAttachShader(pathTracerPO, fragmentshader);
	glLinkProgram(pathTracerPO);

	bool success = CShaderUtils::ValidateShaderProgram(pathTracerPO);
	assert(success);

	glBindFragDataLocation(pathTracerPO, 0, "my_FragColor0");
	glLinkProgram(pathTracerPO);

	// delete shader objects
	glDetachShader(pathTracerPO, vertexshader);
	glDetachShader(pathTracerPO, fragmentshader);

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

	// query all uniform locations
	CShaderUtils::QueryUniformLocations(pathTracerUniTable, pathTracerPO);
	CShaderUtils::QueryUniformLocations(tonemapUniTable, tonemapPO);

	// create render targets (don't know size yet)
	glGenTextures(1, &renderTargets[0]);
	glBindTexture(GL_TEXTURE_2D, renderTargets[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 256, 256, 0, GL_RGBA, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &renderTargets[1]);
	glBindTexture(GL_TEXTURE_2D, renderTargets[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 256, 256, 0, GL_RGBA, GL_FLOAT, NULL);

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
		// NOTE: will be swapped every frame with the other one
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTargets[0], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTarget, 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		assert(status == GL_FRAMEBUFFER_COMPLETE);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// setup camera
	camera.SetClipPlanes(0.1f, 20.0f);
	camera.SetFov(glm::radians(45.0f));
	camera.SetDistance(6);
	camera.SetPosition(glm::vec3(0, 1.633f, 0));
	camera.SetOrientation(glm::vec3(glm::radians(135.0f), glm::radians(30.0f), 0));

	return true;
}

void CMyApp::Clean()
{
	glDeleteFramebuffers(1, &framebuffer);
	glDeleteTextures(1, &renderTargets[0]);
	glDeleteTextures(1, &renderTargets[1]);
	glDeleteTextures(1, &depthTarget);

	glDeleteProgram(pathTracerPO);
	glDeleteProgram(tonemapPO);
	glDeleteVertexArrays(1, &screenQuadVAO);
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

	static Uint32 last_time = SDL_GetTicks();
	float delta_time = (SDL_GetTicks() - last_time) / 1000.0f;

	time += delta_time;
	last_time = SDL_GetTicks();

	// tweakables
	glm::vec3 eyepos;
	glm::mat4 view, proj;
	glm::mat4 viewproj;
	glm::mat4 viewprojinv;

	proj = glm::perspectiveFovRH<float>(glm::radians(45.0f), (float)windowWidth, (float)windowHeight, 0.1f, 20.0f);

	camera.GetViewMatrixAndEyePosition(view, eyepos);

	viewproj = proj * view;
	viewprojinv = glm::inverse(viewproj);

	if (currSample == 0) {
		// clear it so we don't read junk
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTargets[0], 0);
		
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}

	if (currSample < 16384) {
		++currSample;

		// render pass 1 (path tracer)
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTargets[currSample % 2], 0);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		{
			// setup graphics pipeline
			glUseProgram(pathTracerPO);
			glBindVertexArray(screenQuadVAO);
			glDisable(GL_DEPTH_TEST);
			glViewport(0, 0, windowWidth, windowHeight);

			// update uniforms
			pathTracerUniTable.SetMatrix4fv("matViewProjInv", 1, GL_FALSE, &viewprojinv[0][0]);
			pathTracerUniTable.SetVector3fv("eyePos", 1, &eyepos.x);
			pathTracerUniTable.SetFloat("time", time);
			pathTracerUniTable.SetFloat("currSample", (float)currSample);
			pathTracerUniTable.SetInt("prevIteration", 0);

			glBindTexture(GL_TEXTURE_2D, renderTargets[1 - currSample % 2]);

			// draw screen quad
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
	} else {
		printf("Convergence finished\n");
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
		tonemapUniTable.SetInt("sampler0", 0);
		glBindTexture(GL_TEXTURE_2D, renderTargets[currSample % 2]);

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

		currSample = 0;
		time = 0;
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
	glBindTexture(GL_TEXTURE_2D, renderTargets[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, renderTargets[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, depthTarget);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, windowWidth, windowHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	currSample = 0;
	time = 0;
}
