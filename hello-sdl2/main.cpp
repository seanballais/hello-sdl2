#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

class LTexture
{
public:
  LTexture()
    : texture(nullptr)
    , width(0)
    , height(0) {}
  
  ~LTexture()
  {
    free();
  }

  bool loadFromFile(std::string path)
  {
    free();

    SDL_Texture* texture = nullptr;

    SDL_Surface* surface = IMG_Load(path.c_str());
    if (surface == nullptr) {
      std::cout << "Unable to load image, " << path << ". SDL_image Error: "
                << IMG_GetError() << std::endl;
    } else {
      SDL_Surface* formattedSurface = SDL_ConvertSurfaceFormat(
        surface, SDL_PIXELFORMAT_RGBA8888, 0);
      if (formattedSurface == nullptr) {
        std::cout << "Unable to convert loaded surface to display format. "
                  << "SDL Error: " << SDL_GetError() << std::endl;
        return false;
      }

      texture = SDL_CreateTexture(gRenderer,
                                  SDL_PIXELFORMAT_RGBA8888,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  formattedSurface->w,
                                  formattedSurface->h);
      if (texture == nullptr) {
        std::cout << "Unable to create a blank texture. "
                  << "SDL Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(formattedSurface);
        return false;
      }

      SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

      SDL_LockTexture(texture, nullptr, &(this->pixels), &(this->pitch));

      memcpy(this->pixels, formattedSurface->pixels,
             formattedSurface->pitch * formattedSurface->h);

      this->width = formattedSurface->w;
      this->height = formattedSurface->h;

      uint32_t* pixels = (uint32_t*) this->pixels;
      int pixelCount = (this->pitch / 4) * this->height;

      uint32_t colourKey = SDL_MapRGB(formattedSurface->format, 0, 0xFF, 0xFF);
      uint32_t transparent = SDL_MapRGBA(formattedSurface->format,
                                         0x00, 0xFF, 0xFF, 0x00);

      for (int i = 0; i < pixelCount; i++) {
        if (pixels[i] == colourKey) {
          pixels[i] = transparent;
        }
      }

      SDL_UnlockTexture(texture);

      this->pixels = nullptr;
      this->width = formattedSurface->w;
      this->height = formattedSurface->h;

      SDL_FreeSurface(formattedSurface);
    }

    SDL_FreeSurface(surface);

    this->texture = texture;

    return texture != nullptr;
  }

  bool createBlank(int width, int height,
                   SDL_TextureAccess access = SDL_TEXTUREACCESS_STREAMING)
  {
    this->texture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_RGBA8888,
                                      access, width, height);
    if (this->texture == nullptr) {
      std::cout << "Unable to create blank texture. SDL Error: "
                << SDL_GetError() << std::endl;
    } else {
      this->width = width;
      this->height = height;
    }

    return this->texture != nullptr;
  }

  void free()
  {
    if (this->texture != nullptr) {
      SDL_DestroyTexture(this->texture);

      this->texture = nullptr;
      this->width = 0;
      this->height = 0;
    }
  }

  void setColor(uint8_t r, uint8_t g, uint8_t b)
  {
    SDL_SetTextureColorMod(this->texture, r, g, b);
  }

  void setBlendMode(SDL_BlendMode blending)
  {
    SDL_SetTextureBlendMode(this->texture, blending);
  }

  void setAlpha(uint8_t alpha)
  {
    SDL_SetTextureAlphaMod(this->texture, alpha);
  }

  void render(int x, int y, SDL_Rect* clip = nullptr,
              double angle = 0.0, SDL_Point* center = nullptr,
              SDL_RendererFlip flip = SDL_FLIP_NONE)
  {
    SDL_Rect renderQuad{x, y, this->width, this->height};

    if (clip != nullptr) {
      renderQuad.w = clip->w;
      renderQuad.h = clip->h;
    }

    SDL_RenderCopyEx(gRenderer, this->texture, clip, &renderQuad,
                     angle, center, flip);
  }

  void setAsRenderTarget()
  {
    SDL_SetRenderTarget(gRenderer, this->texture);
  }

  int getWidth()
  {
    return this->width;
  }

  int getHeight()
  {
    return this->height;
  }

  bool lockTexture()
  {
    bool lockingSuccessful = true;

    if (this->pixels != nullptr) {
      std::cout << "Texture is already locked." << std::endl;
      lockingSuccessful = false;
    } else {
      if (SDL_LockTexture(this->texture,
                          nullptr,
                          &(this->pixels),
                          &(this->pitch)) != 0) {
        std::cout << "Unable to lock texture. SDL Error: " << SDL_GetError()
                  << std::endl;
        lockingSuccessful = false;
      }
    }

    return lockingSuccessful;
  }

  bool unlockTexture()
  {
    bool unlockingSuccess = true;

    if (this->pixels == nullptr) {
      std::cout << "Texture is not locked." << std::endl;
      unlockingSuccess = false;
    } else {
      SDL_UnlockTexture(this->texture);
      this->pixels = nullptr;
      this->pitch = 0;
    }

    return unlockingSuccess;
  }

  void* getPixels()
  {
    return this->pixels;
  }

  void copyPixels(void* pixels)
  {
    if (this->pixels != nullptr) {
      memcpy(this->pixels, pixels, this->pitch * this->height);
    }
  }

  int getPitch()
  {
    return this->pitch;
  }

  uint32_t getPixel32(unsigned int x, unsigned int y)
  {
    uint32_t* pixels = (uint32_t*) this->pixels;

    return pixels[(y * (this->pitch / 4)) + x];
  }

private:
  SDL_Texture* texture;
  void* pixels;
  int pitch;

  int width;
  int height;
};

LTexture gDotTexture;

class Dot
{
public:
  static const int DOT_WIDTH = 20;
  static const int DOT_HEIGHT = 20;

  static const int DOT_VEL = 640;

  Dot()
      : posX(0)
      , posY(0)
      , velX(0)
      , velY(0) {}

  void handleEvent(SDL_Event* event)
  {
    if (event->type == SDL_KEYDOWN && event->key.repeat == 0) {
      switch (event->key.keysym.sym) {
        case SDLK_w: this->velY -= DOT_VEL; break;
        case SDLK_s: this->velY += DOT_VEL; break;
        case SDLK_a: this->velX -= DOT_VEL; break;
        case SDLK_d: this->velX += DOT_VEL; break;
      }
    } else if (event->type == SDL_KEYUP && event->key.repeat == 0) {
      switch (event->key.keysym.sym) {
        case SDLK_w: this->velY += DOT_VEL; break;
        case SDLK_s: this->velY -= DOT_VEL; break;
        case SDLK_a: this->velX += DOT_VEL; break;
        case SDLK_d: this->velX -= DOT_VEL; break;
      }
    }
  }

  void move(float timeStep)
  {
    this->posX += this->velX * timeStep;
    if (this->posX < 0) {
      this->posX = 0;
    } else if (this->posX > SCREEN_WIDTH - DOT_WIDTH) {
      this->posX = SCREEN_WIDTH - DOT_WIDTH;
    }

    this->posY += this->velY * timeStep;
    if (this->posY < 0) {
      this->posY = 0;
    } else if (this->posY > SCREEN_HEIGHT - DOT_HEIGHT) {
      this->posY = SCREEN_HEIGHT - DOT_HEIGHT;
    }
  }

  void render()
  {
    gDotTexture.render(this->posX, this->posY);
  }

private:
  int posX;
  int posY;

  int velX;
  int velY;
};

class LTimer
{
public:
  LTimer()
    : startTicks(0)
    , pausedTicks(0)
    , isPausedState(false)
    , isStartedState(false) {}

  bool isStarted()
  {
    return this->isStartedState;
  }

  bool isPaused()
  {
    return this->isPausedState && this->isStartedState;
  }

  void start()
  {
    this->isStartedState = true;
    this->isPausedState = false;

    this->startTicks = SDL_GetTicks();
    this->pausedTicks = 0;
  }

  void stop()
  {
    this->isStartedState = false;
    this->isPausedState = false;

    this->startTicks = 0;
    this->pausedTicks = 0;
  }

  void pause()
  {
    if (this->isStarted() && !this->isPaused()) {
      this->isPausedState = true;

      this->pausedTicks = SDL_GetTicks() - this->startTicks;
      this->startTicks = 0;
    }
  }

  void resume()
  {
    if (this->isStarted() && this->isPaused()) {
      this->isPausedState = false;

      this->startTicks = SDL_GetTicks() - this->pausedTicks;

      this->pausedTicks = 0;
    }
  }

  uint32_t getTicks()
  {
    uint32_t time = 0;
    if (this->isStarted()) {
      if (this->isPaused()) {
        time = this->pausedTicks;
      } else {
        time = SDL_GetTicks() - this->startTicks;
      }
    }

    return time;
  }

private:
  uint32_t startTicks;
  uint32_t pausedTicks;

  bool isPausedState;
  bool isStartedState;
};

bool initApp();
bool loadMedia();
void closeApp();

SDL_Surface* loadSurface(std::string path);
SDL_Texture* loadTexture(std::string path);

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

      Dot dot;
      LTimer stepTimer;
      while (!shouldAppQuit) {
        while (SDL_PollEvent(&event) != 0) {
          if (event.type == SDL_QUIT) {
            shouldAppQuit = true;
          }

          dot.handleEvent(&event);
        }

        float timeStep = stepTimer.getTicks() / 1000.f;

        dot.move(timeStep);

        stepTimer.start();

        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(gRenderer);

        dot.render();

        SDL_RenderPresent(gRenderer);
      }
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

  gWindow = SDL_CreateWindow("Hello SDL 2",
                             SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED,
                             SCREEN_WIDTH,
                             SCREEN_HEIGHT,
                             SDL_WINDOW_SHOWN);
  if (gWindow == nullptr) {
    std::cout << "Window could not be created. SDL Error: " << SDL_GetError()
              << std::endl;
    return false;
  }

  gRenderer = SDL_CreateRenderer(gWindow, -1,
                                 SDL_RENDERER_ACCELERATED
                                 | SDL_RENDERER_PRESENTVSYNC);
  if (gRenderer == nullptr) {
    std::cout << "Renderer could not be created. SDL Error: " << SDL_GetError()
              << std::endl;
    return false;
  }

  SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
  
  const int imgFlags = IMG_INIT_PNG;
  if (!(IMG_Init(imgFlags) & imgFlags)) {
    std::cout << "SDL_image could not initialize. SDL_image Error: "
              << IMG_GetError() << std::endl;
    return false;
  }

  return true;
}

bool loadMedia()
{
  bool loadingSuccessState = true;

  if (!gDotTexture.loadFromFile("data/textures/dot.bmp")) {
    std::cout << "Unable to load dot texture." << std::endl;
    loadingSuccessState = false;
  }

  return loadingSuccessState;
}

void closeApp()
{
  gDotTexture.free();

  SDL_DestroyRenderer(gRenderer);
  SDL_DestroyWindow(gWindow);
  
  gRenderer = nullptr;
  gWindow = nullptr;

  IMG_Quit();
  SDL_Quit();
}
