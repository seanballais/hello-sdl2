#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int BUTTON_WIDTH = 300;
const int BUTTON_HEIGHT = 200;
const int TOTAL_BUTTONS = 4;

SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;
TTF_Font* gFont = nullptr;

enum LButtonSprite
{
  BUTTON_SPRITE_MOUSE_OUT = 0,
  BUTTON_SPRITE_MOUSE_OVER_MOTION = 1,
  BUTTON_SPRITE_MOUSE_DOWN = 2,
  BUTTON_SPRITE_MOUSE_UP = 3,
  BUTTON_SPRITE_TOTAL = 4
};

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

SDL_Rect gSpriteClips[BUTTON_SPRITE_TOTAL];
LTexture gButtonSpriteSheetTexture;

class LButton
{
public:
  LButton()
    : position({0, 0})
    , currentSprite(BUTTON_SPRITE_MOUSE_OUT) {}

  void setPosition(int x, int y)
  {
    this->position.x = x;
    this->position.y = y;
  }

  void handleEvent(SDL_Event* event)
  {
    if (event->type == SDL_MOUSEMOTION
        || event->type == SDL_MOUSEBUTTONDOWN
        || event->type == SDL_MOUSEBUTTONUP) {
      int x;
      int y;
      SDL_GetMouseState(&x, &y);

      bool isMouseInsideButton = true;

      if (x < this->position.x) {
        isMouseInsideButton = false;
      } else if (x > (this->position.x + BUTTON_WIDTH)) {
        isMouseInsideButton = false;
      } else if (y < this->position.y) {
        isMouseInsideButton = false;
      } else if (y > (this->position.y + BUTTON_HEIGHT)) {
        isMouseInsideButton = false;
      }

      if (!isMouseInsideButton) {
        this->currentSprite = BUTTON_SPRITE_MOUSE_OUT;
      } else {
        switch (event->type) {
          case SDL_MOUSEMOTION:
            this->currentSprite = BUTTON_SPRITE_MOUSE_OVER_MOTION;
            break;
          case SDL_MOUSEBUTTONDOWN:
            this->currentSprite = BUTTON_SPRITE_MOUSE_DOWN;
            break;
          case SDL_MOUSEBUTTONUP:
            this->currentSprite = BUTTON_SPRITE_MOUSE_UP;
            break;
        }
      }
    }
  }

  void render()
  {
    gButtonSpriteSheetTexture.render(this->position.x, this->position.y,
                                     &gSpriteClips[this->currentSprite]);
  }

private:
  SDL_Point position;

  LButtonSprite currentSprite;
};

LButton gButtons[TOTAL_BUTTONS];

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
          if (event.type == SDL_QUIT) {
            shouldAppQuit = true;
          }

          for (size_t i = 0; i < TOTAL_BUTTONS; i++) {
            gButtons[i].handleEvent(&event);
          }
        }

        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(gRenderer);

        for (size_t i = 0; i < TOTAL_BUTTONS; i++) {
          gButtons[i].render();
        }

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

  const std::string spriteSheetTexturePath = "data/textures/button.png";
  if (!gButtonSpriteSheetTexture.loadFromFile(spriteSheetTexturePath)) {
    std::cout << "Failed to load texture. SDL_image Error: " << IMG_GetError()
              << std::endl;
    loadingSuccessState = false;
  }

  for (int i = 0; i < BUTTON_SPRITE_TOTAL; i++) {
    gSpriteClips[i] = {0, BUTTON_HEIGHT * i, BUTTON_WIDTH, BUTTON_HEIGHT};
  }

  std::array<int, 2> xPositions{0, 340};
  std::array<int, 2> yPositions{0, 280};
  for (size_t i = 0; i < TOTAL_BUTTONS; i++) {
    gButtons[i] = {};
    gButtons[i].setPosition(xPositions[i % 2], yPositions[i / 2]);
  }

  return loadingSuccessState;
}

void closeApp()
{
  gButtonSpriteSheetTexture.free();

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
