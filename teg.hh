/* All C++ headers or source files using TEG should include this file. C files
   should include "teg.h" instead. */
#ifndef TEG_HH
#define TEG_HH

#include <stdarg.h>
#include <stdio.h>
#include <string>

#include "teg.h"
#undef lerp
template<class T, class S> T lerp(T a, T b, S i) { return a + (b-a) * i; }

namespace TEG {
  std::string format(const char* format, ...)
    __attribute__((format(printf, 1, 2)));
  std::string format(const std::string format, ...);
  std::string vformat(const char* format, va_list arg)
    __attribute__((format(printf, 1, 0)));
  std::string vformat(const std::string format, va_list arg);
}

#endif
