#include <cstdint>
#include <iostream>
#include <string>

#include <GL/glut.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

SDL_Window* gWindow = nullptr;
SDL_GLContext gContext;
bool gRenderQuad = true;

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

bool initApp();
bool initGL();
void handleKeys(unsigned char key, int x, int);
void update();
void render();
bool loadMedia();
void closeApp();

int main(int argc, char* args[])
{
  if (!initApp()) {
    std::cout << "Failed to initialize." << std::endl;
  } else {
    if (!loadMedia()) {
      std::cout << "Failed to load media." << std::endl;
    } else {
      bool shouldAppQuit = false;
      SDL_Event event;
      
      SDL_StartTextInput();

      while (!shouldAppQuit) {
        while (SDL_PollEvent(&event) != 0) {
          if (event.type == SDL_QUIT) {
            shouldAppQuit = true;
          } else if (event.type == SDL_TEXTINPUT) {
            int x = 0;
            int y = 0;

            SDL_GetMouseState(&x, &y);
            handleKeys(event.text.text[0], x, y);
          }
        }

        render();

        SDL_GL_SwapWindow(gWindow);
      }

      SDL_StopTextInput();
    }
  }

  closeApp();

  return 0;
}

bool initApp()
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cout << "SDL could not be initialized. SDL Error: " << SDL_GetError()
              << std::endl;
    return false;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

  gWindow = SDL_CreateWindow("Hello SDL 2",
                             SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED,
                             SCREEN_WIDTH,
                             SCREEN_HEIGHT,
                             SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  if (gWindow == nullptr) {
    std::cout << "Window could not be created. SDL Error: " << SDL_GetError()
              << std::endl;
    return false;
  }

  gContext = SDL_GL_CreateContext(gWindow);
  if (gContext == nullptr) {
    std::cout << "OpenGL context could not be created. SDL Error: "
              << SDL_GetError() << std::endl;
    return false;
  }

  if (SDL_GL_SetSwapInterval(1) < 0) {
    std::cout << "Warning: Unable to set VSync! SDL Error: " << SDL_GetError()
              << std::endl;
  }

  if (!initGL()) {
    std::cout << "Unable to initialize OpenGL." << std::endl;
    return false;
  }

  return true;
}

bool initGL()
{
  GLenum error = GL_NO_ERROR;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  error = glGetError();
  if (error != GL_NO_ERROR) {
    std::cout << "Error initializing OpenGL. " << gluErrorString(error)
              << std::endl;
    return false;
  }

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  error = glGetError();
  if (error != GL_NO_ERROR) {
    std::cout << "Error initializing OpenGL. " << gluErrorString(error)
              << std::endl;
    return false;
  }

  glClearColor(0.f, 0.f, 0.f, 1.f);

  error = glGetError();
  if (error != GL_NO_ERROR) {
    std::cout << "Error initializing OpenGL. " << gluErrorString(error)
              << std::endl;
    return false;
  }

  return true;
}

void handleKeys(unsigned char key, int x, int)
{
  if (key == 'q') {
    gRenderQuad = !gRenderQuad;
  }
}

void update()
{

}

void render()
{
  glClear(GL_COLOR_BUFFER_BIT);

  if (gRenderQuad) {
    glBegin(GL_QUADS);
      glVertex2f(-0.5f, -0.5f);
      glVertex2f(0.5f, -0.5f);
      glVertex2f(0.5f, 0.5f);
      glVertex2f(-0.5f, 0.5f);
    glEnd();
  }
}

bool loadMedia()
{
  bool loadingSuccessState = true;
  return loadingSuccessState;
}

void closeApp()
{
  SDL_DestroyWindow(gWindow);
  gWindow = nullptr;

  SDL_Quit();
}
