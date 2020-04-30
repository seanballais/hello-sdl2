#include <cstdint>
#include <iostream>
#include <string>

#include <GL/glew.h>
#include <GL/glut.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

SDL_Window* gWindow = nullptr;
SDL_GLContext gContext;
bool gRenderQuad = true;

GLuint gProgramID = 0;
GLint gVertexPos2DLocation = -1;
GLuint gVBO = 0;
GLuint gIBO = 0;

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

bool initApp();
bool initGL();
void handleKeys(unsigned char key, int x, int);
void update();
void render();
bool loadMedia();
void closeApp();
void printProgramLog(GLuint program);
void printShaderLog(GLuint shader);

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

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

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

  glewExperimental = GL_TRUE;
  GLenum glewError = glewInit();
  if (glewError != GLEW_OK) {
    std::cout << "Error initializing GLEW. " << glewGetErrorString(glewError)
              << std::endl;
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
  gProgramID = glCreateProgram();

  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

  const GLchar* vertexShaderSource[] = {
    "#version 140\n"
    "in vec2 LVertexPos2D;\n"
    "\n"
    "void main() {\n"
    "  gl_Position = vec4(LVertexPos2D.x, LVertexPos2D.y, 0, 1);\n"
    "}"
  };

  glShaderSource(vertexShader, 1, vertexShaderSource, nullptr);

  glCompileShader(vertexShader);

  GLint vShaderCompiled = GL_FALSE;
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vShaderCompiled);
  if (vShaderCompiled != GL_TRUE) {
    std::cout << "Unable to compile vertex shader, " << vertexShader
              << "." << std::endl;
    return false;
  }

  glAttachShader(gProgramID, vertexShader);

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

  const GLchar* fragmetnShaderSource[] = {
    "#version 140\n"
    "out vec4 LFragment;\n"
    "\n"
    "void main() {\n"
    "  LFragment = vec4(1.0, 1.0, 1.0, 1.0);\n"
    "}\n"
  };

  glShaderSource(fragmentShader, 1, fragmetnShaderSource, nullptr);

  glCompileShader(fragmentShader);

  GLint fShaderCompiled = GL_FALSE;
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);
  if (fShaderCompiled != GL_TRUE) {
    std::cout << "Unable to compile fragment shader, " << fragmentShader
              << "." << std::endl;
    return false;
  }

  glAttachShader(gProgramID, fragmentShader);

  glLinkProgram(gProgramID);


  GLint programSuccess = GL_TRUE;
  glGetProgramiv(gProgramID, GL_LINK_STATUS, &programSuccess);
  if (programSuccess != GL_TRUE) {
    std::cout << "Error linking program, " << gProgramID << "." << std::endl;
    printProgramLog(gProgramID);
    return false;
  }

  gVertexPos2DLocation = glGetAttribLocation(gProgramID, "LVertexPos2D");
  if (gVertexPos2DLocation == -1) {
    std::cout << "LVertexPos2D is not a valid GLSL program variable."
              << std::endl;
    return false;
  }

  glClearColor(0.f, 0.f, 0.f, 1.f);

  GLfloat vertexData[] = {
    -0.5f, -0.5f,
     0.5f, -0.5f,
     0.5f,  0.5f,
    -0.5f,  0.5f
  };

  GLuint indexData[] = { 0, 1, 2, 3 };

  glGenBuffers(1, &gVBO);
  glBindBuffer(GL_ARRAY_BUFFER, gVBO);
  glBufferData(GL_ARRAY_BUFFER, 2 * 4 * sizeof(GLfloat), vertexData,
               GL_STATIC_DRAW);

  glGenBuffers(1, &gIBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), indexData,
               GL_STATIC_DRAW);

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
    glUseProgram(gProgramID);

    glEnableVertexAttribArray(gVertexPos2DLocation);

    glBindBuffer(GL_ARRAY_BUFFER, gVBO);
    glVertexAttribPointer(gVertexPos2DLocation, 2, GL_FLOAT, GL_FALSE,
                          2 * sizeof(GLfloat), nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
    glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, nullptr);

    glDisableVertexAttribArray(gVertexPos2DLocation);

    glUseProgram(0);
  }
}

void printProgramLog(GLuint program)
{
  if (glIsProgram(program)) {
    int infoLogLength = 0;
    int maxLength = infoLogLength;

    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

    char* infoLog = new char[maxLength];

    glGetProgramInfoLog(program, maxLength, &infoLogLength, infoLog);
    if (infoLogLength > 0) {
      std::cout << infoLog << std::endl;
    }

    delete[] infoLog;
  } else {
    std::cout << "Name " << program << " is not a program." << std::endl;
  }
}

void printShaderLog(GLuint shader)
{
  if (glIsShader(shader)) {
    int infoLogLength = 0;
    int maxLength = infoLogLength;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

    char* infoLog = new char[maxLength];

    glGetShaderInfoLog(shader, maxLength, &infoLogLength, infoLog);
    if (infoLogLength > 0) {
      std::cout << infoLog << std::endl;
    }

    delete[] infoLog;
  } else {
    std::cout << "Name " << shader << " is not a shader." << std::endl;
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
