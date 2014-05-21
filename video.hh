#ifndef VIDEOINITHH
#define VIDEOINITHH

#include "teg.hh"

namespace Video {
  /* This value will change every time a new OpenGL context is made. */
  extern uint32_t opengl_context_cookie;
  extern int32_t fullscreen_width, fullscreen_height;
  extern int32_t windowed_width, windowed_height;
  extern bool fullscreen_mode, vsync;
  void Init(); // can be called repeatedly, if you want
  void ReadConfig(); // should be called once, when the program starts
  void WriteConfig(); // should be called if changed values are to be saved
  double GetAspect(); // return aspect ratio of screen
  void Swap(); // swap buffers
};

#ifndef TEG_PIXEL_PACK
#define TEG_PIXEL_PACK 8
#endif

#endif
