#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;
TTF_Font* gFont = nullptr;

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
      SDL_SetColorKey(surface, SDL_TRUE,
                      SDL_MapRGB(surface->format, 0, 0xFF, 0xFF));

      texture = SDL_CreateTextureFromSurface(gRenderer, surface);
      if (texture == nullptr) {
        std::cout << "Unable to create texture from image, " << path << "."
                  << "SDL Error: " << SDL_GetError() << std::endl;
      } else {
        this->width = surface->w;
        this->height = surface->h;
      }

      SDL_FreeSurface(surface);
    }

    this->texture = texture;

    return texture != nullptr;
  }

  bool loadFromRenderedText(std::string textureText, SDL_Color textColor)
  {
    free();

    SDL_Surface* textSurface = TTF_RenderText_Solid(gFont, textureText.c_str(),
                                                    textColor);
    if (textSurface == nullptr) {
      std::cout << "Unable to render text surface. SDL_ttf Error: "
                << TTF_GetError() << std::endl;
    } else {
      this->texture = SDL_CreateTextureFromSurface(gRenderer, textSurface);
      if (this->texture == nullptr) {
        std::cout << "Unable to create texture from rendered text. SDL Error: "
                  << SDL_GetError() << std::endl;
      } else {
        this->width = textSurface->w;
        this->height = textSurface->h;
      }

      SDL_FreeSurface(textSurface);
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

  int getWidth()
  {
    return this->width;
  }

  int getHeight()
  {
    return this->height;
  }

private:
  SDL_Texture* texture;

  int width;
  int height;
};

LTexture gTextTexture;

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

        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(gRenderer);

        gTextTexture.render((SCREEN_WIDTH - gTextTexture.getWidth()) / 2,
                            (SCREEN_HEIGHT - gTextTexture.getHeight()) / 2);

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

  if (TTF_Init() == -1) {
    std::cout << "SDL_ttf could not initialize. SDL_ttf Error: "
              << TTF_GetError() << std::endl;
    return false;
  }

  return true;
}

bool loadMedia()
{
  bool loadingSuccessState = true;

  const std::string fontPath = "data/fonts/Raleway-Light.ttf";
  gFont = TTF_OpenFont(fontPath.c_str(), 28);
  if (gFont == nullptr) {
    std::cout << "Failed to load font. SDL_ttf Error: " << TTF_GetError()
              << std::endl;
    loadingSuccessState = false;
  } else {
    SDL_Color textColor {0, 0, 0};
    if (!gTextTexture.loadFromRenderedText("Brown fox jump over good boi.",
                                           textColor)) {
      std::cout << "Failed to render text texture." << std::endl;
      loadingSuccessState = false;
    }
  }

  return loadingSuccessState;
}

void closeApp()
{
  gTextTexture.free();

  TTF_CloseFont(gFont);
  gFont = nullptr;

  SDL_DestroyRenderer(gRenderer);
  SDL_DestroyWindow(gWindow);
  gWindow = nullptr;
  gRenderer = nullptr;

  TTF_Quit();
  IMG_Quit();
  SDL_Quit();
}

SDL_Texture* loadTexture(std::string path)
{
  SDL_Texture* texture = nullptr;

  SDL_Surface* surface = IMG_Load(path.c_str());
  if (surface == nullptr) {
    std::cout << "Unable to load image, " << path << ". SDL Error: "
              << SDL_GetError() << std::endl;
  } else {
    texture = SDL_CreateTextureFromSurface(gRenderer, surface);
    if (texture == nullptr) {
      std::cout << "Unable to create texture from image, " << path
                << ". SDL Error: " << SDL_GetError() << std::endl;
    }

    SDL_FreeSurface(surface);
  }

  return texture;
}
