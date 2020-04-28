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

class LBitmapFont
{
public:
  LBitmapFont()
    : bitmap(nullptr)
    , newLine(0)
    , space(0) {}

  bool buildFont(LTexture* bitmap)
  {
    bool buildSuccess = true;

    if (!bitmap->lockTexture()) {
      std::cout << "Unable to lock bitmap font texture." << std::endl;
      buildSuccess = false;
    } else {
      uint32_t bgColour = bitmap->getPixel32(0, 0);

      int cellW = bitmap->getWidth() / 16;
      int cellH = bitmap->getHeight() / 16;

      int top = cellH;
      int baseA = cellH;

      int currentChar = 0;

      for (int rows = 0; rows < 16; rows++) {
        for (int cols = 0; cols < 16; cols++) {
          this->chars[currentChar] = SDL_Rect {cellW * cols, cellH * rows,
                                               cellW, cellH};

          for (int pCol = 0; pCol < cellW; pCol++) {
            for (int pRow = 0; pRow < cellH; pRow++) {
              int pX = (cellW * cols) + pCol;
              int pY = (cellH * rows) + pRow;

              if (bitmap->getPixel32(pX, pY) != bgColour) {
                this->chars[currentChar].x = pX;

                // Break the loops.
                pCol = cellW;
                pRow = cellH;
              }
            }
          }

          for (int pColW = cellW - 1; pColW >= 0; pColW--) {
            for (int pRowW = 0; pRowW < cellH; pRowW++) {
              int pX = (cellW * cols) + pColW;
              int pY = (cellH * rows) + pRowW;

              if (bitmap->getPixel32(pX, pY) != bgColour) {
                this->chars[currentChar].w = (pX - this->chars[currentChar].x)
                                             + 1;

                pColW = -1;
                pRowW = cellH;
              }
            }
          }

          for (int pRow = 0; pRow < cellH; pRow++) {
            for (int pCol = 0; pCol < cellW; pCol++) {
              int pX = (cellW * cols) + pCol;
              int pY = (cellH * rows) + pRow;

              if (bitmap->getPixel32(pX, pY) != bgColour) {
                if (pRow < top) {
                  top = pRow;
                }

                pCol = cellW;
                pRow = cellH;
              }
            }
          }

          if (currentChar == 'A') {
            for (int pRow = cellH - 1; pRow >= 0; pRow--) {
              for (int pCol = 0; pCol < cellW; pCol++) {
                int pX = (cellW * cols) + pCol;
                int pY = (cellH * rows) + pRow;

                if (bitmap->getPixel32(pX, pY) != bgColour) {
                  baseA = pRow;

                  pCol = cellW;
                  pRow = -1;
                }
              }
            }
          }

          currentChar++;
        }
      }

      this->space = cellW / 2;
      this->newLine = baseA - top;

      for (int i = 0; i < 256; i++) {
        this->chars[i].y += top;
        this->chars[i].h -= top;
      }

      bitmap->unlockTexture();
      this->bitmap = bitmap;
    }

    return buildSuccess;
  }

  void renderText(int x, int y, std::string text)
  {
    if (this->bitmap != nullptr) {
      int curX = x;
      int curY = y;

      for (size_t i = 0; i < text.length(); i++) {
        if (text[i] == ' ') {
          curX += this->space;
        } else if (text[i] == '\n') {
          curY += this->newLine;

          curX = x;
        } else {
          int ascii = (unsigned char) text[i];

          this->bitmap->render(curX, curY, &(this->chars[ascii]));

          curX += this->chars[ascii].w + 1;
        }
      }
    }
  }

private:
  LTexture* bitmap;
  SDL_Rect chars[256];

  int newLine;
  int space;
};

LTexture lazyFontTexture;

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

      LBitmapFont lazyFont;
      lazyFont.buildFont(&lazyFontTexture);

      SDL_Event event;
      while (!shouldAppQuit) {
        while (SDL_PollEvent(&event) != 0) {
          if (event.type == SDL_QUIT) {
            shouldAppQuit = true;
          }
        }

        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(gRenderer);

        lazyFont.renderText(45, 45, "Hello\nSDL2!");

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

  if (!lazyFontTexture.loadFromFile("data/textures/lazyfont.png")) {
    std::cout << "Unable to load Lazy Font texture." << std::endl;
    loadingSuccessState = false;
  }

  return loadingSuccessState;
}

void closeApp()
{
  lazyFontTexture.free();

  SDL_DestroyRenderer(gRenderer);
  SDL_DestroyWindow(gWindow);
  
  gRenderer = nullptr;
  gWindow = nullptr;

  IMG_Quit();
  SDL_Quit();
}
