
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

	sphereVBO			= 0;
	sphereIBO			= 0;
	inputLayout			= 0;
	program				= 0;
}

CMyApp::~CMyApp(void)
{
}

bool CMyApp::Init()
{
	glClearColor(0.125f, 0.25f, 0.5f, 1.0f);
	glClearDepth(1.0f);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);

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

	// create input layout
	glGenVertexArrays(1, &inputLayout);
	glBindVertexArray(inputLayout);
	{
		// NOTE: VBO can't be decoupled from VAO before GL 4.4
		glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SCommonVertex), (const void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SCommonVertex), (const void*)12);

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SCommonVertex), (const void*)20);
	}
	glBindVertexArray(0);

	// create shader program
	GLuint vertexshader = CShaderUtils::FindAndCompileShader(GL_VERTEX_SHADER, L"pointlight.vert");
	GLuint fragmentshader = CShaderUtils::FindAndCompileShader(GL_FRAGMENT_SHADER, L"pointlight.frag");

	assert(vertexshader != 0);
	assert(fragmentshader != 0);

	program = glCreateProgram();

	glAttachShader(program, vertexshader);
	glAttachShader(program, fragmentshader);
	glLinkProgram(program);

	bool success = CShaderUtils::ValidateShaderProgram(program);
	assert(success);

	// bind fragment output and relink
	glBindFragDataLocation(program, 0, "my_FragColor0");
	glLinkProgram(program);

	// delete shader objects
	glDetachShader(program, vertexshader);
	glDetachShader(program, fragmentshader);

	glDeleteShader(vertexshader);
	glDeleteShader(fragmentshader);

	// query all uniform locations and print them
	CShaderUtils::QueryUniformLocations(uniformLocs, program);

	printf("\nList of active uniforms:\n");

	for (auto it : uniformLocs)
		printf("  %s (%d)\n", it.first.c_str(), it.second);

	return true;
}

void CMyApp::Clean()
{
	glDeleteProgram(program);
	glDeleteVertexArrays(1, &inputLayout);
	glDeleteBuffers(1, &sphereVBO);
	glDeleteBuffers(1, &sphereIBO);
}

void CMyApp::Update()
{
	static Uint32 last_time = SDL_GetTicks();
	float delta_time = (SDL_GetTicks() - last_time) / 1000.0f;

	// TODO: update with delta

	last_time = SDL_GetTicks();
}

void CMyApp::Render()
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, windowWidth, windowHeight);

	// TODO:
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
}
