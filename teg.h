/* All C-language source or header files should include this file. It ensures
   that the basic types are available and all required headers to interact with
   the various libraries we depend on are included portably.
   C++ files should include teg.hh instead, as it includes this file and also
   sets up any C++-specific stuff. */
#ifndef TEG_H
#define TEG_H

#if defined(__cplusplus) && !defined(TEG_HH)
#error Please include "teg.hh" rather than "teg.h" from C++ files.
#endif

#ifndef GAME_PRETTY_NAME
#error Please compile with -DGAME_PRETTY_NAME="Game Name Here"
#endif

#ifndef GAME_WINDOW_TITLE
#define GAME_WINDOW_TITLE GAME_PRETTY_NAME
#endif

#define __STDC_FORMAT_MACROS 1

#if __WIN32__
// Why? Dunno
#include <winsock2.h>
#endif

#include <stdint.h>
#include <inttypes.h>

#ifdef __WIN32__
# define DIR_SEP "\\"
# include <windows.h>
# undef KEY_EXECUTE
#else
# define DIR_SEP "/"
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
#endif

#if !NO_SDL
# include "SDL.h"
# if !NO_OPENGL
#  include "SDL_opengl.h"
#  include <GL/glu.h> // yuck
//#include OPENAL_H
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !NO_LUA
#include <lua.h>
#include <lauxlib.h>
#endif

extern int g_argc;
extern const char** g_argv;

#ifndef NO_MASK_MALLOC
extern void* malloc(size_t size) __attribute__((error("Use safe_malloc (or new) instead.")));
extern void free(void* ptr) __attribute__((error("Use safe_free (or delete) instead.")));
extern void* calloc(size_t nmemb, size_t size) __attribute__((error("Use safe_calloc (or new) instead.")));
extern void* realloc(void* ptr, size_t size) __attribute__((error("Use safe_realloc instead.")));
#endif

extern char* strdup(const char* src)
#if __cplusplus
  throw()
#endif
  __attribute__((error("Use teg_strdup instead.")));

/* Versions of the standard C allocation functions that are guaranteed to
   conform to our expectations. */
/* safe_malloc(0) returns NULL, otherwise it always succeeds. */
extern void* safe_malloc(size_t size) __attribute__((malloc));
/* safe_free(NULL) does nothing. */
extern void safe_free(void* ptr);
/* safe_calloc(0,x) or (x,0) returns NULL, otherwise it always succeeds. */
extern void* safe_calloc(size_t nmemb, size_t size)  __attribute__((malloc));
/* safe_realloc(NULL,x) returns safe_malloc(x).
   safe_realloc(x,0) frees x and returns NULL.
   otherwise, it always succeeds (and returns the new pointer). */
extern void* safe_realloc(void* ptr, size_t size);

/* Portable version of strdup. */
extern char* teg_strdup(const char* src);

/* Display an error message to the user and then EXPLODE! */
extern void die(const char* format, ...) __attribute__((noreturn,
                                                        format(printf,1,2)));

#if DEBUG
#define assert(expr) if(!(expr)) die("%s:%i: Assertion failed (%s)", __FILE__, __LINE__, #expr)
#else
#define assert(expr) (void)0
#endif

/* Arguments are encoded with UTF-8. */
int teg_main(int argc, char* argv[]);

#if DEBUG
#define dprintf(format, ...) fprintf(stderr, "DEBUG:%s:%i: " format, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define dprintf(format, ...) (void)0
#endif

void _assertgl(const char* file, int line, const char* name);
#define assertgl(name) _assertgl(__FILE__, __LINE__, name)

#define elementcount(array) (sizeof(array) / sizeof(*(array)))

#define lerp(a,b,i) ((a) + ((b)-(a)) * (i))

#ifdef __cplusplus
}
#endif

#endif
