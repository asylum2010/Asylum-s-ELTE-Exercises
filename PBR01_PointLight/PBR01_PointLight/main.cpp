
#include <iostream>
#include <sstream>

#ifdef _MSC_VER
#	include <crtdbg.h>
#endif

// library headers
#include <GL/glew.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl_gl3.h>

// user headers
#include "MyApp.h"

// defines
#define WINDOW_WIDTH	1360
#define WINDOW_HEIGHT	768

void exitProgram()
{
	SDL_Quit();

#ifdef _MSC_VER
#	ifdef _DEBUG
	system("pause");
#	endif

	_CrtDumpMemoryLeaks();
#endif
}

int main(int argc, char* args[])
{
	atexit(exitProgram);

	// initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		std::cout << "[SDL] Initialization error: " << SDL_GetError() << std::endl;
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// setup swapchain
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,			32);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,			8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,			8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,			8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,			8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,		1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,			24);

	// create window
	SDL_Window* win = SDL_CreateWindow(
		"PBR exercise 01: Point lights",
		100,
		100,
		640,
		480,
		SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);

	if (win == nullptr) {
		std::cout << "[SDL] Could not create window: " << SDL_GetError() << std::endl;
		return 1;
	}

	// resize it before we start doing anything...
	SDL_SetWindowSize(win, WINDOW_WIDTH, WINDOW_HEIGHT);

	// create GL context
	SDL_GLContext context = SDL_GL_CreateContext(win);

	if (context == nullptr) {
		std::cout << "[SDL] Could not create GL context: " << SDL_GetError() << std::endl;
		return 1;
	}

	// enable vsync
	SDL_GL_SetSwapInterval(1);

	GLenum error = glewInit();
	if (error != GLEW_OK) {
		std::cout << "[GLEW] Initialization error!" << std::endl;
		return 1;
	}

	// query GL version
	GLint major = -1;
	GLint minor = -1;

	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);

	// we support GL 3.3+ only (7+ years old...)
	assert(major >= 3 || (major == 3 && minor >= 3));
	std::cout << "OpenGL version is GL " << major << "." << minor << std::endl;

	// initialize UI
	ImGui_ImplSdlGL3_Init(win);

	// main loop
	{
		bool quit = false;
		SDL_Event ev;
		CMyApp app;

		if (!app.Init()) {
			SDL_GL_DeleteContext(context);
			SDL_DestroyWindow(win);

			std::cout << "[CMyApp] Initialization error!" << std::endl;
			return 1;
		}

		while (!quit) {
			// poll SDL events
			while (SDL_PollEvent(&ev)) {
				ImGui_ImplSdlGL3_ProcessEvent(&ev);
				
				bool is_mouse_captured = ImGui::GetIO().WantCaptureMouse;
				bool is_keyboard_captured = ImGui::GetIO().WantCaptureKeyboard;
				
				switch (ev.type) {
				case SDL_QUIT:
					quit = true;
					break;

				case SDL_KEYDOWN:
					if (ev.key.keysym.sym == SDLK_ESCAPE)
						quit = true;

					if (!is_keyboard_captured)
						app.KeyboardDown(ev.key);

					break;

				case SDL_KEYUP:
					if (!is_keyboard_captured)
						app.KeyboardUp(ev.key);

					break;

				case SDL_MOUSEBUTTONDOWN:
					if (!is_mouse_captured)
						app.MouseDown(ev.button);

					break;

				case SDL_MOUSEBUTTONUP:
					if (!is_mouse_captured)
						app.MouseUp(ev.button);

					break;

				case SDL_MOUSEWHEEL:
					if (!is_mouse_captured)
						app.MouseWheel(ev.wheel);

					break;

				case SDL_MOUSEMOTION:
					if (!is_mouse_captured)
						app.MouseMove(ev.motion);
					break;

				case SDL_WINDOWEVENT:
					if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
						app.Resize(ev.window.data1, ev.window.data2);

					break;
				}
			}

			ImGui_ImplSdlGL3_NewFrame(win);
			{
				app.Update();
				app.Render();
			}
			ImGui::Render();

			SDL_GL_SwapWindow(win);
		}

		// release GL objects
		app.Clean();
	}

	// clean up and exit
	ImGui_ImplSdlGL3_Shutdown();
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);

	return 0;
}
