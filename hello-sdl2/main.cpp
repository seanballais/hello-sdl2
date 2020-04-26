#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int LEVEL_WIDTH = 1280;
const int LEVEL_HEIGHT = 960;
const int BUTTON_WIDTH = 300;
const int BUTTON_HEIGHT = 200;
const int TOTAL_BUTTONS = 4;
const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

SDL_Renderer* gRenderer = nullptr;
TTF_Font* gFont = nullptr;
Mix_Music* gMusic = nullptr;

Mix_Chunk* gScratch = nullptr;
Mix_Chunk* gHigh = nullptr;
Mix_Chunk* gMedium = nullptr;
Mix_Chunk* gLow = nullptr;

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

LTexture gBGTexture;
LTexture gInputTextTexture;
LTexture gPromptTextTexture;

struct Circle
{
  int x;
  int y;
  int r;
};

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

LTexture gDotTexture;

bool checkCollision(Circle& a, Circle& b);
bool checkCollision(Circle& a, SDL_Rect& b);

class Dot
{
public:
  static const int DOT_WIDTH = 20;
  static const int DOT_HEIGHT = 20;

  static const int DOT_VEL = 10;

  Dot(int x, int y)
      : posX(x)
      , posY(y)
      , velX(0)
      , velY(0)
      , collider({0, 0, DOT_WIDTH / 2}) {}

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

  void move()
  {
    this->posX += this->velX;
    if ((this->posX < 0) || (this->posX + DOT_WIDTH > SCREEN_WIDTH)) {
      this->posX -= this->velX;
    }

    this->posY += this->velY;
    if ((this->posY < 0) || (this->posY + DOT_HEIGHT > SCREEN_HEIGHT)) {
      this->posY -= this->velY;
    }
  }

  void render()
  {
    gDotTexture.render(this->posX, this->posY);
  }

  Circle& getCollider()
  {
    return this->collider;
  }

  int getPosX()
  {
    return this->posX;
  }

  int getPosY()
  {
    return this->posY;
  }

private:
  int posX;
  int posY;

  int velX;
  int velY;

  Circle collider;
};

const int TOTAL_WINDOWS = 3;

class LWindow
{
public:
  LWindow()
    : window(nullptr)
    , width(0)
    , height(0)
    , isMouseFocused(false)
    , isKeyboardFocused(false)
    , isFullscreen(false)
    , isMinimized_(false) {}

  bool init()
  {
    this->window = SDL_CreateWindow("SDL 2 Tutorial",
                                    SDL_WINDOWPOS_UNDEFINED,
                                    SDL_WINDOWPOS_UNDEFINED,
                                    SCREEN_WIDTH,
                                    SCREEN_HEIGHT,
                                    SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (this->window != nullptr) {
      this->isMouseFocused = true;
      this->isKeyboardFocused = true;
      this->width = SCREEN_WIDTH;
      this->height = SCREEN_HEIGHT;

      this->renderer = SDL_CreateRenderer(this->window,
                                          -1,
                                          SDL_RENDERER_ACCELERATED
                                          | SDL_RENDERER_PRESENTVSYNC);
      if (this->renderer == nullptr) {
        std::cout << "Renderer could not be created. SDL Error: "
                  << SDL_GetError() << std::endl;
        SDL_DestroyWindow(this->window);
        this->window = nullptr;
      } else {
        SDL_SetRenderDrawColor(this->renderer, 0xFF, 0xFF, 0xFF, 0xFF);

        this->windowID = SDL_GetWindowID(this->window);

        this->isShown_ = true;
      }
    } else {
      std::cout << "Window could not be created. SDL Error: "
                << SDL_GetError() << std::endl;
    }

    return this->window != nullptr && this->renderer != nullptr;
  }

  void handleEvent(SDL_Event& event)
  {
    if (event.type == SDL_WINDOWEVENT
        && event.window.windowID == this->windowID) {
      bool updateCaption = false;

      switch (event.window.event) {
        case SDL_WINDOWEVENT_SHOWN:
          this->isShown_ = true;
          break;
        case SDL_WINDOWEVENT_HIDDEN:
          this->isShown_ = false;
          break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
          this->width = event.window.data1;
          this->height = event.window.data2;
          SDL_RenderPresent(this->renderer);
          break;
        case SDL_WINDOWEVENT_EXPOSED:
          SDL_RenderPresent(this->renderer);
          break;
        case SDL_WINDOWEVENT_ENTER:
          this->isMouseFocused = true;
          updateCaption = true;
          break;
        case SDL_WINDOWEVENT_LEAVE:
          this->isMouseFocused = false;
          updateCaption = true;
          break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
          this->isKeyboardFocused = true;
          updateCaption = true;
          break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
          this->isKeyboardFocused = false;
          updateCaption = true;
          break;
        case SDL_WINDOWEVENT_MINIMIZED:
          this->isMinimized_ = true;
          break;
        case SDL_WINDOWEVENT_MAXIMIZED:
          this->isMinimized_ = false;
          break;
        case SDL_WINDOWEVENT_RESTORED:
          this->isMinimized_ = false;
          break;
        case SDL_WINDOWEVENT_CLOSE:
          SDL_HideWindow(this->window);
          break;
      }

      if (updateCaption) {
        std::stringstream caption;
        caption << "SDL 2 Tutoral -"
                << " Mouse Focus: " << (this->isMouseFocused ? "On" : "Off")
                << " Keyboard Focus: "
                << ((this->isKeyboardFocused) ? "On" : "Off");
        SDL_SetWindowTitle(this->window, caption.str().c_str());
      }
    } else if (event.type == SDL_KEYDOWN
               && event.key.keysym.sym == SDLK_RETURN) {
      if (this->isFullscreen) {
        SDL_SetWindowFullscreen(this->window, SDL_FALSE);
        this->isFullscreen = false;
      } else {
        SDL_SetWindowFullscreen(this->window, SDL_TRUE);
        this->isFullscreen = true;
        this->isMinimized_ = false;
      }
    }
  }

  void focus()
  {
    if (!this->isShown_) {
      SDL_ShowWindow(this->window);
    }

    SDL_RaiseWindow(this->window);
  }

  void render()
  {
    if (!this->isMinimized_) {
      SDL_SetRenderDrawColor(this->renderer, 0xFF, 0xFF, 0xFF, 0xFF);
      SDL_RenderClear(this->renderer);
      SDL_RenderPresent(this->renderer);
    }
  }
  
  void free()
  {
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(this->window);

    this->renderer = nullptr;
    this->window = nullptr;
  }

  int getWidth()
  {
    return this->width;
  }
  
  int getHeight()
  {
    return this->height;
  }

  bool hasMouseFocus()
  {
    return this->isMouseFocused;
  }

  bool hasKeyboardFocus()
  {
    return this->isKeyboardFocused;
  }

  bool isMinimized()
  {
    return this->isMinimized_;
  }

  bool isShown()
  {
    return this->isShown_;
  }

private:
  SDL_Window* window;
  SDL_Renderer* renderer;
  uint32_t windowID;

  int width;
  int height;

  bool isMouseFocused;
  bool isKeyboardFocused;
  bool isFullscreen;
  bool isMinimized_;
  bool isShown_;
};

LWindow gWindows[TOTAL_WINDOWS];
LTexture gSceneTexture;

LButton gButtons[TOTAL_BUTTONS];
LTexture gTimeTextTexture;

bool initApp();
bool loadMedia();
void closeApp();

SDL_Surface* loadSurface(std::string path);
SDL_Texture* loadTexture(std::string path);

const int TOTAL_DATA = 10;
int32_t gData[TOTAL_DATA];
LTexture gDataTextures[TOTAL_DATA];

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

      for (int i = 1; i < TOTAL_WINDOWS; i++) {
        gWindows[i].init();
      }

      while (!shouldAppQuit) {
        while (SDL_PollEvent(&event) != 0) {
          if (event.type == SDL_QUIT) {
            shouldAppQuit = true;
          }

          for (int i = 0; i < TOTAL_WINDOWS; i++) {
            gWindows[i].handleEvent(event);
          }

          if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
              case SDLK_1:
                gWindows[0].focus();
                break;
              case SDLK_2:
                gWindows[1].focus();
                break;
              case SDLK_3:
                gWindows[2].focus();
                break;
            }
          }
        }

        for (int i = 0; i < TOTAL_WINDOWS; i++) {
          gWindows[i].render();
        }

        bool allWindowsClosed = true;
        for (int i = 0; i < TOTAL_WINDOWS; i++) {
          if (gWindows[i].isShown()) {
            allWindowsClosed = false;
            break;
          }
        }

        if (allWindowsClosed) {
          shouldAppQuit = true;
        }
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

  if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
    std::cout << "Warning: Linear texture filtering not enabled." << std::endl;
  }

  if (!gWindows[0].init()) {
    std::cout << "Window 0 could not be created." << std::endl;
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

  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    std::cout << "SDL_mixer could not initialize. SDL_mixer Error: "
              << Mix_GetError() << std::endl;
    return false;
  }

  return true;
}

bool loadMedia()
{
  bool loadingSuccessState = true;
  return loadingSuccessState;
}

void closeApp()
{
  gSceneTexture.free();
  gDotTexture.free();
  gBGTexture.free();

  for (int i = 0; i < TOTAL_WINDOWS; i++) {
    gWindows[i].free();
  }

  gPromptTextTexture.free();
  gInputTextTexture.free();

  TTF_CloseFont(gFont);
  gFont = nullptr;

  Mix_Quit();
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();
}

double distanceSquared(int x1, int y1, int x2, int y2)
{
  return ((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2));
}

bool checkCollision(Circle& a, Circle& b)
{
  int totalRadiusSquared = a.r + b.r;
  totalRadiusSquared = totalRadiusSquared * totalRadiusSquared;
  return (distanceSquared(a.x, a.y, b.x, b.y) < totalRadiusSquared);
}

bool checkCollision(Circle& a, SDL_Rect& b)
{
  int closestX;
  int closestY;

  if (a.x < b.x) {
    closestX = b.x;
  } else if (a.x > b.x + b.w) {
    closestX = b.x + b.w;
  } else {
    closestX = a.x;
  }

  if (a.y < b.y) {
    closestY = b.y;
  } else if (a.y > b.y + b.h) {
    closestY = b.y + b.h;
  } else {
    closestY = a.y;
  }

  return (distanceSquared(a.x, a.y, closestX, closestY) < ((a.r) * (a.r)));
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
