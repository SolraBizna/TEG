#define NO_MASK_MALLOC 1
#include "teg.h"

#include <stdarg.h>

extern void* safe_malloc(size_t size) {
  if(size == 0) return NULL;
  else {
    void* ret = malloc(size);
    if(ret == NULL) die("couldn't malloc %zu bytes", size);
    return ret;
  }
}

extern void safe_free(void* ptr) {
  if(ptr != NULL) free(ptr);
}

extern void* safe_calloc(size_t nmemb, size_t size) {
  if(nmemb == 0 || size == 0) return NULL;
  else {
    void* ret = calloc(nmemb, size);
    if(ret == NULL) die("couldn't calloc %zu x %zu bytes", nmemb, size);
    return ret;
  }
}

extern void* safe_realloc(void* ptr, size_t size) {
  if(size == 0) {
    safe_free(ptr);
    return NULL;
  }
  else if(ptr == NULL) return safe_malloc(size);
  else {
    void* ret = realloc(ptr, size);
    if(ret == NULL) die("couldn't realloc %zu bytes", size);
    return ret;
  }
}

extern void _assertgl(const char* file, int line, const char* name) {
  GLenum error;
  int had_error = 0;
  while((error = glGetError()) != GL_NO_ERROR) {
    if(!had_error)
      fprintf(stderr, "%s:%i: OpenGL errors detected %s:\n", file, line, name);
    had_error = 1;
    fprintf(stderr, "%s:%i: %s\n", file, line, gluErrorString(error));
  }
#if DEBUG
  if(had_error)
    die("OpenGL errors occurred, see stderr.");
#endif
}

extern char* teg_strdup(const char* src) {
  /* Very annoyed at Microsoft right now. */
  if(src == NULL) return NULL;
  size_t len = strlen(src) + 1;
  char* ret = (char*)safe_malloc(len);
  memcpy(ret, src, len);
  return ret;
}

extern void die(const char* format, ...) {
  char error[1920]; // enough to fill up an 80x24 terminal
  va_list arg;
  va_start(arg, format);
  vsnprintf(error, sizeof(error), format, arg);
  va_end(arg);
  /* TODO: replace fourth parameter with game window */
  /* TODO: exit fullscreen mode before displaying error */
  if(SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                              GAME_PRETTY_NAME" encountered a fatal error.",
                              error, NULL)) {
    fprintf(stderr,
            "%s encountered a fatal error:\n%s\n"
            "Additionally, there was an error when attempting to use SDL to"
            " display this\nerror message: %s\n",
            GAME_PRETTY_NAME, error, SDL_GetError());
  }
  exit(1);
}
