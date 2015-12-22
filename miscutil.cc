#define NO_MASK_MALLOC 1
#include "teg.hh"
#if !NO_OPENGL
#include "video.hh"
#endif

#include <string.h>
#include <stdarg.h>

#if __WIN32__
/*
Copyright (C) 2014 insane coder (http://insanecoding.blogspot.com/,
http://asprintf.insanecoding.org/)

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#include <limits.h>
int vasprintf(char **strp, const char *fmt, va_list ap)
{
  int r = -1, size = _vscprintf(fmt, ap);

  if ((size >= 0) && (size < INT_MAX))
    {
      *strp = (char *)safe_malloc(size+1); //+1 for null
      if (*strp)
        {
          r = vsnprintf(*strp, size+1, fmt, ap);  //+1 for null
          if ((r < 0) || (r > size))
            {
              safe_free(*strp);
              r = -1;
            }
        }
    }
  else { *strp = 0; }

  return(r);
}
#endif

extern void* safe_malloc(size_t size) {
  if(size == 0) return NULL;
  else {
    void* ret = malloc(size);
    if(ret == NULL) die("couldn't malloc %lu bytes", (unsigned long)size);
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
    if(ret == NULL) die("couldn't calloc %lu x %lu bytes",
                        (unsigned long)nmemb, (unsigned long)size);
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
    if(ret == NULL) die("couldn't realloc %lu bytes", (unsigned long)size);
    return ret;
  }
}

#if !NO_OPENGL
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
#endif

extern char* teg_strdup(const char* src) {
  /* Very annoyed at Microsoft right now. */
  if(src == NULL) return NULL;
  size_t len = strlen(src) + 1;
  char* ret = (char*)safe_malloc(len);
  memcpy(ret, src, len);
  return ret;
}

#ifndef TEG_NO_DIE_IMPLEMENTATION
extern void die(const char* format, ...) {
  char error[1920]; // enough to fill up an 80x24 terminal
  va_list arg;
  va_start(arg, format);
  vsnprintf(error, sizeof(error), format, arg);
  va_end(arg);
  Video::Kill();
  fprintf(stderr, "%s encountered a fatal error:\n%s\n", GAME_PRETTY_NAME,
          error);
  if(SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                              GAME_PRETTY_NAME" encountered a fatal error.",
                              error, NULL)) {
    fprintf(stderr,
            "Additionally, there was an error when attempting to use SDL to"
            " display this\nerror message: %s\n",
            SDL_GetError());
  }
  exit(1);
}
#endif

std::string TEG::format(const char* format, ...) {
  va_list arg;
  va_start(arg, format);
  std::string ret = TEG::vformat(format, arg);
  va_end(arg);
  return ret;
}

std::string TEG::vformat(const char* format, va_list arg) {
  char* new_p = NULL;
  vasprintf(&new_p, format, arg);
  assert(new_p);
  std::string ret = std::string(new_p);
  safe_free(new_p);
  return ret;
}

std::string TEG::format(const std::string& format, ...) {
  va_list arg;
  va_start(arg, format);
  std::string ret = TEG::vformat(format.c_str(), arg);
  va_end(arg);
  return ret;
}

std::string TEG::vformat(const std::string& format, va_list arg) {
  return TEG::vformat(format.c_str(), arg);
}
