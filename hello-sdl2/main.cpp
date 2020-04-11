#include <iostream>

#include <SDL2/SDL.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

SDL_Window* gWindow = nullptr;
SDL_Surface* gScreenSurface = nullptr;
SDL_Surface* gHelloSDL2Img = nullptr;

bool initApp();
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
      while (!shouldAppQuit) {
        while (SDL_PollEvent(&event) != 0) {
          switch (event.type) {
            case SDL_QUIT:
              shouldAppQuit = true;
              break;
            default:
              continue;
          }
        }

        SDL_BlitSurface(gHelloSDL2Img, NULL, gScreenSurface, NULL);
        SDL_UpdateWindowSurface(gWindow);
      }
    }
  }

  closeApp();

  return 0;
}

bool initApp()
{
  bool initSuccessState = true;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cout << "SDL could not be initialized. SDL Error: " << SDL_GetError()
              << std::endl;
    initSuccessState = false;
  } else {
    gWindow = SDL_CreateWindow("Hello SDL 2",
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               SCREEN_WIDTH,
                               SCREEN_HEIGHT,
                               SDL_WINDOW_SHOWN);
    if (gWindow == nullptr) {
      std::cout << "Window could not be created. SDL Error: " << SDL_GetError()
                << std::endl;
      initSuccessState = false;
    } else {
      gScreenSurface = SDL_GetWindowSurface(gWindow);
    }
  }

  return initSuccessState;
}

bool loadMedia()
{
  bool loadingSuccessState = true;

  const char* imagePath = "data/hello-sdl2.bmp";
  gHelloSDL2Img = SDL_LoadBMP(imagePath);
  if (gHelloSDL2Img == nullptr) {
    std::cout << "Unable to load image, " << imagePath << ". SDL Error: "
              << SDL_GetError() << std::endl;
    loadingSuccessState = false;
  }

  return loadingSuccessState;
}

void closeApp()
{
  SDL_FreeSurface(gHelloSDL2Img);
  gHelloSDL2Img = nullptr;

  SDL_DestroyWindow(gWindow);
  gWindow = nullptr;

  SDL_Quit();
}
