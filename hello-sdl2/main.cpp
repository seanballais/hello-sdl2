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

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const int LEVEL_WIDTH = 1280;
const int LEVEL_HEIGHT = 960;

const int TILE_WIDTH = 80;
const int TILE_HEIGHT = 80;
const int TOTAL_TILES = 192;
const int TOTAL_TILE_SPRITES = 12;

const int TILE_RED = 0;
const int TILE_GREEN = 1;
const int TILE_BLUE = 2;
const int TILE_CENTER = 3;
const int TILE_TOP = 4;
const int TILE_TOPRIGHT = 5;
const int TILE_RIGHT = 6;
const int TILE_BOTTOMRIGHT = 7;
const int TILE_BOTTOM = 8;
const int TILE_BOTTOMLEFT = 9;
const int TILE_LEFT = 10;
const int TILE_TOPLEFT = 11;

SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;

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

LTexture gTileTexture;
SDL_Rect gTileClips[12];

bool checkCollision(SDL_Rect a, SDL_Rect b);

class Tile
{
public:
  Tile(int x, int y, int tileType)
    : box({x, y, TILE_WIDTH, TILE_HEIGHT})
    , type(tileType) {}

  void render(SDL_Rect& camera)
  {
    if (checkCollision(camera, this->box)) {
      gTileTexture.render(this->box.x - camera.x,
                          this->box.y - camera.y,
                          &gTileClips[this->type]);
    }
  }

  int getType()
  {
    return this->type;
  }

  SDL_Rect getBox()
  {
    return this->box;
  }

private:
  SDL_Rect box;
  int type;
};

bool touchesWall(SDL_Rect box, Tile* tiles[]);
bool setTiles(Tile* tiles[]);

LTexture gDotTexture;

class Dot
{
public:
  static const int DOT_WIDTH = 20;
  static const int DOT_HEIGHT = 20;

  static const int DOT_VEL = 10;

  Dot()
    : box({0, 0, DOT_WIDTH, DOT_HEIGHT})
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

  void move(Tile* tiles[])
  {
    this->box.x += this->velX;

    if ((this->box.x < 0)
        || (this->box.x + DOT_WIDTH > LEVEL_WIDTH)
        || touchesWall(this->box, tiles)) {
      this->box.x -= this->velX;
    }

    this->box.y += this->velY;

    if ((this->box.y < 0)
        || (this->box.y + DOT_HEIGHT > LEVEL_HEIGHT)
        || touchesWall(this->box, tiles)) {
      this->box.y -= this->velY;
    }
  }

  void setCamera(SDL_Rect& camera)
  {
    camera.x = (this->box.x + DOT_WIDTH / 2) - (SCREEN_WIDTH / 2);
    camera.y = (this->box.y + DOT_HEIGHT / 2) - (SCREEN_HEIGHT / 2);

    if (camera.x < 0) {
      camera.x = 0;
    }

    if (camera.y < 0) {
      camera.y = 0;
    }

    if (camera.x > LEVEL_WIDTH - camera.w) {
      camera.x = LEVEL_WIDTH - camera.w;
    }

    if (camera.y > LEVEL_HEIGHT - camera.h) {
      camera.y = LEVEL_HEIGHT - camera.h;
    }
  }

  void render(SDL_Rect& camera)
  {
    gDotTexture.render(this->box.x - camera.x, this->box.y - camera.y);
  }

private:
  SDL_Rect box;

  int velX;
  int velY;
};

bool initApp();
bool loadMedia(Tile* tiles[]);
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
    Tile* tileSet[TOTAL_TILES];
    if (!loadMedia(tileSet)) {
      std::cout << "Failed to load media." << std::endl;
    } else {
      bool shouldAppQuit = false;
      SDL_Event event;

      SDL_Rect camera {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

      Dot dot;
      while (!shouldAppQuit) {
        while (SDL_PollEvent(&event) != 0) {
          if (event.type == SDL_QUIT) {
            shouldAppQuit = true;
          }

          dot.handleEvent(&event);
        }

        dot.move(tileSet);
        dot.setCamera(camera);

        SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(gRenderer);

        for (int i = 0; i < TOTAL_TILES; i++) {
          tileSet[i]->render(camera);
        }

        dot.render(camera);

        SDL_RenderPresent(gRenderer);
      }
    }

    for (int i = 0; i < TOTAL_TILES; i++) {
      if (tileSet[i] != nullptr) {
        delete tileSet[i];
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

bool loadMedia(Tile* tiles[])
{
  bool loadingSuccessState = true;

  if (!gDotTexture.loadFromFile("data/textures/dot.bmp")) {
    std::cout << "Failed to load dot texture." << std::endl;
    loadingSuccessState = false;
  }

  if (!gTileTexture.loadFromFile("data/textures/tiles.png")) {
    std::cout << "Failed to load tile set texture." << std::endl;
    loadingSuccessState = false;
  }

  if (!setTiles(tiles)) {
    std::cout << "Failed to load tile set." << std::endl;
    loadingSuccessState = false;
  }

  return loadingSuccessState;
}

void closeApp()
{
  SDL_DestroyRenderer(gRenderer);
  SDL_DestroyWindow(gWindow);
  
  gRenderer = nullptr;
  gWindow = nullptr;

  IMG_Quit();
  SDL_Quit();
}

bool setTiles(Tile* tiles[])
{
  bool tilesLoaded = true;

  int x = 0;
  int y = 0;

  std::ifstream map {"data/maps/lazy.map"};
  if (map.fail()) {
    std::cout << "Unable to load map file." << std::endl;
    tilesLoaded = false;
  } else {
    for (int i = 0; i < TOTAL_TILES; i++) {
      int tileType = -1;

      map >> tileType;

      if (map.fail()) {
        std::cout << "Error loading map: Unexpected EOF." << std::endl;
        tilesLoaded = false;
        break;
      }

      if ((tileType >= 0) && (tileType < TOTAL_TILE_SPRITES)) {
        tiles[i] = new Tile(x, y, tileType);
      } else {
        std::cout << "Error loading map: Invalid tile type at "
                  << i << "." << std::endl;
        tilesLoaded = false;
        break;
      }

      x += TILE_WIDTH;

      if (x >= LEVEL_WIDTH) {
        x = 0;
        y += TILE_HEIGHT;
      }
    }
  }

  if (tilesLoaded) {
    gTileClips[TILE_RED] = {0, 0, TILE_WIDTH, TILE_HEIGHT};
    gTileClips[TILE_GREEN] = {0, 80, TILE_WIDTH, TILE_HEIGHT};
    gTileClips[TILE_BLUE] = {0, 160, TILE_WIDTH, TILE_HEIGHT};
    gTileClips[TILE_TOPLEFT] = {80, 0, TILE_WIDTH, TILE_HEIGHT};
    gTileClips[TILE_LEFT] = {80, 80, TILE_WIDTH, TILE_HEIGHT};
    gTileClips[TILE_BOTTOMLEFT] = {80, 160, TILE_WIDTH, TILE_HEIGHT};
    gTileClips[TILE_TOP] = {160, 0, TILE_WIDTH, TILE_HEIGHT};
    gTileClips[TILE_CENTER] = {160, 80, TILE_WIDTH, TILE_HEIGHT};
    gTileClips[TILE_BOTTOM] = {160, 160, TILE_WIDTH, TILE_HEIGHT};
    gTileClips[TILE_TOPRIGHT] = {240, 0, TILE_WIDTH, TILE_HEIGHT};
    gTileClips[TILE_RIGHT] = {240, 80, TILE_WIDTH, TILE_HEIGHT};
    gTileClips[TILE_BOTTOMRIGHT] = {240, 160, TILE_WIDTH, TILE_HEIGHT};
  }

  map.close();

  return tilesLoaded;
}

bool touchesWall(SDL_Rect box, Tile* tiles[])
{
  for (int i = 0; i < TOTAL_TILES; i++) {
    if ((tiles[i]->getType() >= TILE_CENTER)
        && (tiles[i]->getType() <= TILE_TOPLEFT)) {
      if (checkCollision(box, tiles[i]->getBox())) {
        return true;
      }
    }
  }

  return false;
}

bool checkCollision(SDL_Rect a, SDL_Rect b)
{
  int leftA;
  int leftB;
  int rightA;
  int rightB;
  int topA;
  int topB;
  int bottomA;
  int bottomB;

  leftA = a.x;
  rightA = a.x + a.w;
  topA = a.y;
  bottomA = a.y + a.h;

  leftB = b.x;
  rightB = b.x + b.w;
  topB = b.y;
  bottomB = b.y + b.h;

  if ((bottomA <= topB)
      || (topA >= bottomB)
      || (rightA <= leftB)
      || (leftA >= rightB)) {
    return false;
  }

  return true;
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
