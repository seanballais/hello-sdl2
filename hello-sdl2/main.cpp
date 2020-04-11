#include <iostream>

#include <SDL2/SDL.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

enum KeyPressSurfaces
{
  KEY_PRESS_SURFACE_DEFAULT,
  KEY_PRESS_SURFACE_UP,
  KEY_PRESS_SURFACE_DOWN,
  KEY_PRESS_SURFACE_LEFT,
  KEY_PRESS_SURFACE_RIGHT,
  KEY_PRESS_SURFACE_TOTAL
};

SDL_Window* gWindow = nullptr;
SDL_Surface* gScreenSurface = nullptr;
SDL_Surface* gCurrentSurface = nullptr;

SDL_Surface* gKeyPressSurfaces[KEY_PRESS_SURFACE_TOTAL];

bool initApp();
bool loadMedia();
void closeApp();

SDL_Surface* loadSurface(std::string path);

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
      gCurrentSurface = gKeyPressSurfaces[KEY_PRESS_SURFACE_DEFAULT];
      while (!shouldAppQuit) {
        while (SDL_PollEvent(&event) != 0) {
          switch (event.type) {
            case SDL_QUIT:
              shouldAppQuit = true;
              break;

            case SDL_KEYDOWN:
              switch (event.key.keysym.sym) {
                case SDLK_UP:
                  gCurrentSurface = gKeyPressSurfaces[KEY_PRESS_SURFACE_UP];
                  break;

                case SDLK_DOWN:
                  gCurrentSurface = gKeyPressSurfaces[KEY_PRESS_SURFACE_DOWN];
                  break;

                case SDLK_LEFT:
                  gCurrentSurface = gKeyPressSurfaces[KEY_PRESS_SURFACE_LEFT];
                  break;

                case SDLK_RIGHT:
                  gCurrentSurface = gKeyPressSurfaces[KEY_PRESS_SURFACE_RIGHT];
                  break;

                default:
                  gCurrentSurface = gKeyPressSurfaces[
                                      KEY_PRESS_SURFACE_DEFAULT];
                  break;
              }
              break;
            default:
              continue;
          }
        }

        SDL_Rect stretchRect;
        stretchRect.x = 0;
        stretchRect.y = 0;
        stretchRect.w = SCREEN_WIDTH;
        stretchRect.h = SCREEN_HEIGHT;
        SDL_BlitScaled(gCurrentSurface, NULL, gScreenSurface, &stretchRect);
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

  const char* defaultImagePath = "data/hello-sdl2.bmp";
  gKeyPressSurfaces[KEY_PRESS_SURFACE_DEFAULT] = loadSurface(defaultImagePath);

  const char* upImagePath = "data/key_presses/up.bmp";
  gKeyPressSurfaces[KEY_PRESS_SURFACE_UP] = loadSurface(upImagePath);

  const char* downImagePath = "data/key_presses/down.bmp";
  gKeyPressSurfaces[KEY_PRESS_SURFACE_DOWN] = loadSurface(downImagePath);

  const char* leftImagePath = "data/key_presses/left.bmp";
  gKeyPressSurfaces[KEY_PRESS_SURFACE_LEFT] = loadSurface(leftImagePath);

  const char* rightImagePath = "data/key_presses/right.bmp";
  gKeyPressSurfaces[KEY_PRESS_SURFACE_RIGHT] = loadSurface(rightImagePath);

  for (auto*& surface : gKeyPressSurfaces) {
    if (surface == nullptr) {
      loadingSuccessState = false;
      break;
    }
  }

  return loadingSuccessState;
}

void closeApp()
{
  for (auto*& surface : gKeyPressSurfaces) {
    SDL_FreeSurface(surface);
    surface = nullptr;
  }

  SDL_DestroyWindow(gWindow);
  gWindow = nullptr;

  SDL_Quit();
}

SDL_Surface* loadSurface(std::string path)
{
  SDL_Surface* optimizedSurface = nullptr;
  SDL_Surface* loadedSurface = SDL_LoadBMP(path.c_str());
  if (loadedSurface == nullptr) {
    std::cout << "Unable to load BMP image, " << path << ". SDL Error: "
              << SDL_GetError() << std::endl;
  } else {
    optimizedSurface = SDL_ConvertSurface(loadedSurface,
                                          gScreenSurface->format,
                                          0);
    if (optimizedSurface == nullptr) {
      std::cout << "Unable to optimize BMP image, " << path << ". SDL Error: "
                << SDL_GetError() << std::endl;
    }

    SDL_FreeSurface(loadedSurface);
  }

  return optimizedSurface;
}
