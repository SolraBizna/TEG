#include "video.hh"
#include "config.hh"

#include "xgl.hh"

using namespace Video;

const uint32_t Video::uninitialized_context_cookie = 0;
uint32_t Video::opengl_context_cookie = uninitialized_context_cookie;

#ifndef DEFAULT_WINDOWED_WIDTH
#define DEFAULT_WINDOWED_WIDTH 640
#endif

#ifndef DEFAULT_WINDOWED_HEIGHT
#define DEFAULT_WINDOWED_HEIGHT 480
#endif

int32_t Video::fullscreen_width = 0, Video::fullscreen_height = 0;
int32_t Video::windowed_width = DEFAULT_WINDOWED_WIDTH,
  Video::windowed_height = DEFAULT_WINDOWED_HEIGHT;
bool Video::fullscreen_mode = true;
bool Video::vsync = false;

static const char* video_config_file = "Video Configuration";
static const Config::Element video_config_elements[] = {
  Config::Element("fullscreen_width", fullscreen_width),
  Config::Element("fullscreen_height", fullscreen_height),
  Config::Element("windowed_width", windowed_width),
  Config::Element("windowed_height", windowed_height),
  Config::Element("fullscreen_mode", fullscreen_mode),
  Config::Element("vsync", vsync),
};

static SDL_Window* window = NULL;
static SDL_GLContext glcontext = NULL;
static bool inited = false;
static bool window_visible = true, window_minimized = false;

void Video::Kill() {
  if(inited) {
    SDL_Quit();
    inited = false;
  }
}

void Video::Init() {
  if(!inited) {
    if(SDL_Init(SDL_INIT_VIDEO))
      die("Couldn't initialize SDL!");
    inited = true;
  }
  if(glcontext) SDL_GL_DeleteContext(glcontext);
  if(window) SDL_DestroyWindow(window);
  int target_w, target_h;
  Uint32 flags = SDL_WINDOW_OPENGL;
  if(fullscreen_mode) {
    target_w = fullscreen_width;
    target_h = fullscreen_height;
    if(target_w == 0 || target_h == 0)
      flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    else
      flags |= SDL_WINDOW_FULLSCREEN;
  }
  else {
    target_w = windowed_width;
    target_h = windowed_height;
  }
  do {
    /* sanity */
    if(target_w < 320) target_w = 320;
    else if(target_h < 240) target_h = 240;
    window = SDL_CreateWindow(GAME_WINDOW_TITLE,
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              target_w, target_h, flags);
    if(window) break;
    else if(flags & SDL_WINDOW_FULLSCREEN) {
      fprintf(stderr, "Fullscreen mode %i x %i failed. Trying a window.\n",
              target_w, target_h);
      flags &= ~(SDL_WINDOW_FULLSCREEN_DESKTOP);
      target_w = windowed_width;
      target_h = windowed_height;
    }
    else
      die("Could not create a window no matter how hard we tried. The last reason SDL gave was: %s", SDL_GetError());
  } while(1);
  /* TODO: set minimized/visible state? */
  dprintf("SDL_CreateWindow(..., %i, %i, 0x%x) succeeded.\n",
          target_w, target_h, flags);
  glcontext = SDL_GL_CreateContext(window);
  if(!glcontext)
    die("Could not create an OpenGL context. The reason SDL gave was: %s", SDL_GetError());
  ++opengl_context_cookie;
  dprintf("OpenGL initialized. Context cookie is %i.\n",
          opengl_context_cookie);
  xgl::Initialize();
  /* OpenGL context setup */
  glPixelStorei(GL_PACK_ALIGNMENT, TEG_PIXEL_PACK);
  glPixelStorei(GL_UNPACK_ALIGNMENT, TEG_PIXEL_PACK);
}

void Video::ReadConfig() {
  Config::Read(video_config_file, video_config_elements, elementcount(video_config_elements));
}

void Video::WriteConfig() {
  Config::Write(video_config_file, video_config_elements, elementcount(video_config_elements));
}

uint32_t Video::GetScreenWidth() {
  int w, h;
  SDL_GL_GetDrawableSize(window, &w, &h);
  return w;
}

uint32_t Video::GetScreenHeight() {
  int w, h;
  SDL_GL_GetDrawableSize(window, &w, &h);
  return h;
}

double Video::GetAspect() {
  int w, h;
  SDL_GL_GetDrawableSize(window, &w, &h);
  return (double)w / h;
}

void Video::Swap() {
  SDL_GL_SwapWindow(window);
}

bool Video::IsScreenActive() {
  return window_visible && !window_minimized;
}

bool Video::HandleEvent(SDL_Event& evt) {
  switch(evt.type) {
  case SDL_WINDOWEVENT:
    switch(evt.window.event) {
    case SDL_WINDOWEVENT_SHOWN:
      window_visible = true;
      break;
    case SDL_WINDOWEVENT_HIDDEN:
      window_visible = false;
      break;
    case SDL_WINDOWEVENT_MINIMIZED:
      window_minimized = true;
      break;
    case SDL_WINDOWEVENT_RESTORED:
      window_minimized = false;
      break;
    }
    return true;
  }
  return false;
}
